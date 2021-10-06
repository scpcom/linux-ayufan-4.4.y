/*
################################################################################
#
# r8168 is the Linux device driver released for Realtek Gigabit Ethernet
# controllers with PCI-Express interface.
#
# Copyright(c) 2017 Realtek Semiconductor Corp. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
#
# Author:
# Realtek NIC software team <nicfae@realtek.com>
# No. 2, Innovation Road II, Hsinchu Science Park, Hsinchu 300, Taiwan
#
################################################################################
*/

/************************************************************************************
 *  This product is covered by one or more of the following patents:
 *  US6,570,884, US6,115,776, and US6,327,625.
 ***********************************************************************************/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/completion.h>

#include <asm/uaccess.h>

#include "r8168.h"
#include "r8168_dash.h"
#include "rtl_eeprom.h"

int AllocateDashShareMemory(struct net_device *dev)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        struct pci_dev *pdev = tp->pci_dev;
        u64 PhyAddr;
        u64 NewPhyAddr;
        u16 Offset;
        int ret = 0;

        if (!tp->DASH) return -EFAULT;

        do {
                //allocate tx desc
                tp->NumTxDashSendFwDesc = 4 ;    //init 4 dash tx desc
                tp->SizeOfTxDashSendFwDesc = sizeof(TX_DASH_SEND_FW_DESC) ;
                tp->SizeOfTxDashSendFwDescMemAlloc = tp->NumTxDashSendFwDesc * tp->SizeOfTxDashSendFwDesc + ALIGN_256;

                tp->UnalignedTxDashSendFwDescVa = pci_alloc_consistent(pdev, tp->SizeOfTxDashSendFwDescMemAlloc,
                                                  &tp->UnalignedTxDashSendFwDescPa);
                if ( !tp->UnalignedTxDashSendFwDescVa ) {
                        ret = -ENOMEM ;
                        break;
                }

                memset(tp->UnalignedTxDashSendFwDescVa, 0, tp->SizeOfTxDashSendFwDescMemAlloc);

                PhyAddr = tp->UnalignedTxDashSendFwDescPa;
                NewPhyAddr = ( ( PhyAddr + ALIGN_256 ) & ( ~ALIGN_256 ) );
                Offset = NewPhyAddr - PhyAddr;
                tp->TxDashSendFwDesc = ( PTX_DASH_SEND_FW_DESC ) ( tp->UnalignedTxDashSendFwDescVa + Offset );
                tp->TxDashSendFwDescPhy = tp->UnalignedTxDashSendFwDescPa + Offset;

                // allocate tx buffer
                tp->NumOfSendToFwBuffer = tp->NumTxDashSendFwDesc ;
                tp->SizeOfSendToFwBuffer = SEND_TO_FW_BUF_SIZE ;
                tp->SizeOfSendToFwBufferMemAlloc = tp->NumOfSendToFwBuffer * tp->SizeOfSendToFwBuffer + ALIGN_256;

                tp->UnalignedSendToFwBufferVa = pci_alloc_consistent(pdev, tp->SizeOfSendToFwBufferMemAlloc,
                                                &tp->UnalignedSendToFwBufferPa);

                if ( !tp->UnalignedSendToFwBufferVa ) {
                        ret = -ENOMEM ;
                        break;
                }

                memset(tp->UnalignedSendToFwBufferVa, 0, tp->SizeOfSendToFwBufferMemAlloc);

                //tx buffer must be aligned 256 bytes
                PhyAddr = tp->UnalignedSendToFwBufferPa;
                NewPhyAddr = ( ( PhyAddr + ALIGN_256 ) & ( ~ALIGN_256 ) );
                Offset = NewPhyAddr - PhyAddr;
                tp->SendToFwBuffer = (u8*) (tp->UnalignedSendToFwBufferVa) + Offset ;
                tp->SendToFwBufferPhy = tp->UnalignedSendToFwBufferPa + Offset ;

                //allocate rx desc
                tp->NumRxDashRecvFwDesc = 4 ;
                tp->SizeOfRxDashRecvFwDesc = sizeof (RX_DASH_FROM_FW_DESC) ;
                tp->SizeOfRxDashRecvFwDescMemAlloc = tp->NumRxDashRecvFwDesc * tp->SizeOfRxDashRecvFwDesc + ALIGN_256;

                tp->UnalignedRxDashRecvFwDescVa = pci_alloc_consistent(pdev, tp->SizeOfRxDashRecvFwDescMemAlloc,
                                                  &tp->UnalignedRxDashRecvFwDescPa);

                if ( !tp->UnalignedRxDashRecvFwDescVa ) {
                        ret = -ENOMEM ;
                        break;
                }

                //rx desc must be aligned 256 bytes
                memset(tp->UnalignedRxDashRecvFwDescVa, 0, tp->SizeOfRxDashRecvFwDescMemAlloc);

                PhyAddr = tp->UnalignedRxDashRecvFwDescPa;
                NewPhyAddr = ( ( PhyAddr + ALIGN_256 ) & ( ~ALIGN_256 ) );
                Offset = NewPhyAddr - PhyAddr;
                tp->RxDashRecvFwDesc = ( PRX_DASH_FROM_FW_DESC ) ( tp->UnalignedRxDashRecvFwDescVa + Offset );
                tp->RxDashRecvFwDescPhy = tp->UnalignedRxDashRecvFwDescPa + Offset;

                //allocate rx buffer
                tp->NumRecvFromFwBuffer = tp->NumRxDashRecvFwDesc ;
                tp->SizeOfRecvFromFwBuffer = RECV_FROM_FW_BUF_SIZE ;
                tp->SizeOfRecvFromFwBufferMemAlloc = tp->NumRecvFromFwBuffer * tp->SizeOfRecvFromFwBuffer + ALIGN_256;
                tp->UnalignedRecvFromFwBufferVa = pci_alloc_consistent(pdev, tp->SizeOfRecvFromFwBufferMemAlloc,
                                                  &tp->UnalignedRecvFromFwBufferPa);

                if ( !tp->UnalignedRecvFromFwBufferVa ) {
                        ret = -ENOMEM ;
                        break;
                }

                memset(tp->UnalignedRecvFromFwBufferVa, 0, tp->SizeOfRecvFromFwBufferMemAlloc);

                PhyAddr = tp->UnalignedRecvFromFwBufferPa;
                NewPhyAddr = ( ( PhyAddr + ALIGN_256 ) & ( ~ALIGN_256 ) );
                Offset = NewPhyAddr - PhyAddr;
                tp->RecvFromFwBuffer = (u8*)tp->UnalignedRecvFromFwBufferVa + Offset ;
                tp->RecvFromFwBufferPhy = tp->UnalignedRecvFromFwBufferPa + Offset ;
        } while(FALSE);

        return ret;
}

void FreeAllocatedDashShareMemory(struct net_device *dev)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        struct pci_dev *pdev = tp->pci_dev;

        if ( tp->UnalignedTxDashSendFwDescVa ) {
                pci_free_consistent(pdev, tp->SizeOfTxDashSendFwDescMemAlloc, tp->UnalignedTxDashSendFwDescVa,
                                    tp->UnalignedTxDashSendFwDescPa);

                tp->UnalignedTxDashSendFwDescVa = NULL;
        }

        if ( tp->UnalignedSendToFwBufferVa ) {
                pci_free_consistent(pdev, tp->SizeOfSendToFwBufferMemAlloc, tp->UnalignedSendToFwBufferVa,
                                    tp->UnalignedSendToFwBufferPa);

                tp->UnalignedSendToFwBufferVa = NULL;
        }

        if ( tp->UnalignedRxDashRecvFwDescVa ) {
                pci_free_consistent(pdev, tp->SizeOfRxDashRecvFwDescMemAlloc, tp->UnalignedRxDashRecvFwDescVa,
                                    tp->UnalignedRxDashRecvFwDescPa);

                tp->UnalignedRxDashRecvFwDescVa = NULL;
        }

        if ( tp->UnalignedRecvFromFwBufferVa ) {
                pci_free_consistent(pdev, tp->SizeOfRecvFromFwBufferMemAlloc, tp->UnalignedRecvFromFwBufferVa,
                                    tp->UnalignedRecvFromFwBufferPa);

                tp->UnalignedRecvFromFwBufferVa = NULL;
        }
}

static void Dash2ResetTx(struct rtl8168_private *tp)
{
        if (!tp->DASH) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                Dash2DisableTx( tp );
                Dash2EnableTx( tp );
        }
}

static void Dash2WriteTxPollingBit(struct rtl8168_private *tp)
{
        if (!tp->DASH) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                RTL_CMAC_W8(CMAC_IBCR2, RTL_CMAC_R8(CMAC_IBCR2) | BIT_1);
        }
}

static u8
IsDash2RxLastDesc(struct rtl8168_private *tp)
{
        if(tp->CurrNumRxDashRecvFwDesc == tp->NumRxDashRecvFwDesc -1)
                return TRUE;
        else
                return FALSE;
}

static u8
IsDash2TxLastDesc(struct rtl8168_private *tp)
{
        if(tp->CurrNumTxDashSendFwDesc == tp->NumTxDashSendFwDesc-1)
                return TRUE;
        else
                return FALSE;
}

static PRX_DASH_FROM_FW_DESC
GetDash2RxDesc(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*) tp->RxDashRecvFwDesc +
                  tp->CurrNumRxDashRecvFwDesc * tp->SizeOfRxDashRecvFwDesc;

        return (PRX_DASH_FROM_FW_DESC)tmpUchar;
}

static PTX_DASH_SEND_FW_DESC
GetDash2TxDesc(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*)tp->TxDashSendFwDesc +
                  tp->CurrNumTxDashSendFwDesc * tp->SizeOfTxDashSendFwDesc;

        return (PTX_DASH_SEND_FW_DESC)tmpUchar;
}

static PTX_DASH_SEND_FW_DESC
GetLastSendDash2TxDesc(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*)tp->TxDashSendFwDesc +
                  tp->LastSendNumTxDashSendFwDesc * tp->SizeOfTxDashSendFwDesc;

        return (PTX_DASH_SEND_FW_DESC)tmpUchar;
}

static u64
GetDash2RxBufferPhy(struct rtl8168_private *tp)
{
        u64 tmpPhyAddr;
        tmpPhyAddr = tp->RecvFromFwBufferPhy;
        tmpPhyAddr += tp->CurrNumRxDashRecvFwDesc * tp->SizeOfRecvFromFwBuffer;

        return tmpPhyAddr ;
}

static u64
GetDash2TxBufferPhy(struct rtl8168_private *tp)
{
        u64 tmpPhyAddr;
        tmpPhyAddr = tp->SendToFwBufferPhy;
        tmpPhyAddr += tp->CurrNumTxDashSendFwDesc * tp->SizeOfSendToFwBuffer;

        return tmpPhyAddr ;
}

static u8*
GetDash2RxBuffer(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*)tp->RecvFromFwBuffer;
        tmpUchar += tp->CurrNumRxDashRecvFwDesc * tp->SizeOfRecvFromFwBuffer;
        return tmpUchar;
}

static u8*
GetDash2TxBuffer(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*)tp->SendToFwBuffer;
        tmpUchar += tp->CurrNumTxDashSendFwDesc * tp->SizeOfSendToFwBuffer;

        return tmpUchar ;
}

static u8*
GetLastSendDash2TxBuffer(struct rtl8168_private *tp)
{
        u8 *tmpUchar;
        tmpUchar= (u8*)tp->SendToFwBuffer;
        tmpUchar += tp->LastSendNumTxDashSendFwDesc * tp->SizeOfSendToFwBuffer;

        return tmpUchar ;
}

static void
NextDash2RxDesc(struct rtl8168_private *tp)
{
        if(tp->CurrNumRxDashRecvFwDesc == tp->NumRxDashRecvFwDesc-1 )
                tp->CurrNumRxDashRecvFwDesc = 0;
        else
                tp->CurrNumRxDashRecvFwDesc++ ;
}

static void
NextDash2TxDesc(struct rtl8168_private *tp)
//increase the index of dash2 tx descriptor
{
        if(tp->CurrNumTxDashSendFwDesc == tp->NumTxDashSendFwDesc-1)
                tp->CurrNumTxDashSendFwDesc = 0;
        else
                tp->CurrNumTxDashSendFwDesc++;
}

static void NICInitDash2Send(struct rtl8168_private *tp)
{
        u32 index;
        PTX_DASH_SEND_FW_DESC pDescTxDashSendFw;
        u16 TmpStatus ;

        if (!tp->DASH) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                for(index=0; index<tp->NumTxDashSendFwDesc; index++) {
                        pDescTxDashSendFw = GetDash2TxDesc(tp);
                        pDescTxDashSendFw->BufferAddress = GetDash2TxBufferPhy(tp);
                        pDescTxDashSendFw->resv=0;
                        pDescTxDashSendFw->length = SEND_TO_FW_BUF_SIZE;

                        if(IsDash2TxLastDesc(tp)) {
                                TmpStatus = TXS_FS | TXS_LS | TXS_EOR;
                        } else {
                                TmpStatus = TXS_FS | TXS_LS ;
                        }

                        pDescTxDashSendFw->statusLowByte = (u8) TmpStatus ;
                        pDescTxDashSendFw->statusHighByte = (u8) (TmpStatus >> 8);

                        NextDash2TxDesc(tp) ;

                }
        }
}

static void NICInitDash2Recv(struct rtl8168_private *tp)
{
        PRX_DASH_FROM_FW_DESC pDescRxDashFromFw;
        u32 index;
        u16 TmpStatus;

        if (!tp->DASH) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                for(index=0; index<tp->NumRxDashRecvFwDesc; index++) {
                        pDescRxDashFromFw = GetDash2RxDesc(tp);
                        pDescRxDashFromFw->BufferAddress = GetDash2RxBufferPhy(tp);
                        pDescRxDashFromFw->resv = 0;
                        pDescRxDashFromFw->length = RECV_FROM_FW_BUF_SIZE ;

                        if(IsDash2RxLastDesc(tp)) {
                                TmpStatus = RX_DASH_FROM_FW_OWN | RXS_EOR;
                        } else {
                                TmpStatus = RX_DASH_FROM_FW_OWN ;
                        }
                        pDescRxDashFromFw->statusLowByte = (u8)TmpStatus;
                        pDescRxDashFromFw->statusHighByte = (u8)(TmpStatus>>8);

                        NextDash2RxDesc(tp);
                }

                tp->CurrNumRxDashRecvFwDesc = 0; //after init rx desc, reset current num of rx desc
        }
}

static void Dash2DrvInformOobToStopCmac(struct rtl8168_private *tp)
//inform oob to stop CMAC
{
        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                //ask oob to stop CMAC engine

                tp->CmacResetting = TRUE ;
                OCP_write(tp, 0x180, 4, CMAC_OOB_STOP);
                OCP_write(tp, 0x30, 4, 0x1);
        }
}

static void Dash2CmacHwReset(struct rtl8168_private *tp)
{
        u32 TmpUlong;

        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        //ocp reg 0xb2000150 bit5=1 //cmac reset
        //ocp reg 0xb2000150 bit5=0 //cmac reset done
        //mac 0xF8 bit24=1
        //ocp reg 0xb200080C bit8=1 //clear oob cmac reset ISR
        if (HW_DASH_SUPPORT_TYPE_2(tp)) {
                TmpUlong = OCP_read(tp, 0x150, 4);
                TmpUlong |= BIT_5;
                OCP_write(tp, 0x150, 4, TmpUlong);

                TmpUlong = OCP_read(tp, 0x150, 4);
                TmpUlong &= ~(BIT_5);
                OCP_write(tp, 0x150, 4, TmpUlong);
        } else if (HW_DASH_SUPPORT_TYPE_3(tp)) {
                TmpUlong = OCP_read(tp, 0x150, 4);
                TmpUlong |= BIT_5;
                OCP_write(tp, 0x150, 4, TmpUlong);
                udelay(1); //delay 1us
        }

        RTL_CMAC_W8(CMAC_IBISR0, RTL_CMAC_R8(CMAC_IBISR0) | BIT_0);

        TmpUlong = OCP_read(tp, 0x80c, 4);
        TmpUlong |= BIT_24 ;
        OCP_write(tp, 0x80c, 4, TmpUlong);
}

/*
static void Dash2CmacCheckOobInitOk(struct rtl8168_private *tp)
{
        u32 TmpUlong ;

        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp)) {
                TmpUlong = OCP_read(tp, 0x2C20, 4);
                if(TmpUlong == CMAC_OOB_INIT ) {
                        OCP_write(tp, 0x2C20, 4, 0x0);
                        tp->CmacResetting=FALSE ;
                }
        } else if (HW_DASH_SUPPORT_TYPE_3(tp)) {
                TmpUlong = OCP_read_with_oob_base_address(tp, CMAC_SYNC_REG, 4, RTL8168FP_KVM_BASE);
                if(TmpUlong == CMAC_OOB_INIT ) {
                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);
                        tp->CmacResetting=FALSE ;
                }
        }
}

static void Dash2CmacCheckOobIssueCmacReset(struct rtl8168_private *tp)
//check if oob issue CMAC_RESET request and reset the dummy register
{
        u32 TmpUlong;

        tp->CmacOobIssueCmacReset = FALSE ;

        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp)) {
                TmpUlong = OCP_read(tp, 0x2C20, 4);

                if(TmpUlong == CMAC_OOB_RESET) {
                        OCP_write(tp, 0x2C20, 4, 0x0);

                        tp->CmacOobIssueCmacReset = TRUE ;
                }
        } else if (HW_DASH_SUPPORT_TYPE_3(tp)) {
                TmpUlong = OCP_read_with_oob_base_address(tp, CMAC_SYNC_REG, 4, RTL8168FP_KVM_BASE);
                if(TmpUlong == CMAC_OOB_RESET ) {
                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);
                        tp->CmacOobIssueCmacReset = TRUE ;
                }
        }
}
*/

static void Dash2DrvInformOobReInit(struct rtl8168_private *tp)
//driver inform oob to reinit CMAC
{
        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        OCP_write(tp, 0x180, 4, CMAC_OOB_INIT);
        OCP_write(tp, 0x30, 4, 0x1);
}

static void Dash2CmacFillTxRxDescAddress(struct rtl8168_private *tp)
{
        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp)) {
                OCP_write(tp, 0x890, 4, (tp->RxDashRecvFwDescPhy & DMA_BIT_MASK(32)));
                OCP_write(tp, 0x894, 4, (tp->RxDashRecvFwDescPhy >> 32));

                OCP_write(tp, 0x898, 4, (tp->TxDashSendFwDescPhy & DMA_BIT_MASK(32)));
                OCP_write(tp, 0x89c, 4, (tp->TxDashSendFwDescPhy >> 32));
        } else if (HW_DASH_SUPPORT_TYPE_3(tp)) {
                OCP_write_with_oob_base_address(tp, CMAC_RXDESC_OFFSET, 4, (tp->RxDashRecvFwDescPhy & DMA_BIT_MASK(32)), RTL8168FP_CMAC_IOBASE);
                OCP_write_with_oob_base_address(tp, CMAC_RXDESC_OFFSET+4, 4, (tp->RxDashRecvFwDescPhy >> 32), RTL8168FP_CMAC_IOBASE);

                OCP_write_with_oob_base_address(tp, CMAC_TXDESC_OFFSET, 4, (tp->TxDashSendFwDescPhy & DMA_BIT_MASK(32)), RTL8168FP_CMAC_IOBASE);
                OCP_write_with_oob_base_address(tp, CMAC_TXDESC_OFFSET+4, 4, (tp->TxDashSendFwDescPhy >> 32), RTL8168FP_CMAC_IOBASE);
        }
}

/*
static void Dash2DumpTxDesc(struct rtl8168_private *tp)
{
    int i;
    PTX_DASH_SEND_FW_DESC pDescTxDashSendFw;

    for (i=0; i<4; i++) {
        pDescTxDashSendFw= tp->TxDashSendFwDesc + i;
        printk("CurrNumTxDashSendFwDesc=%x, tx desc=\n%02x%02x %x\n%x\n%lx \n",
               i,
               pDescTxDashSendFw->statusHighByte, pDescTxDashSendFw->statusLowByte, pDescTxDashSendFw->length,
               pDescTxDashSendFw->resv,
               pDescTxDashSendFw->BufferAddress
              );
    }
}
*/

static void Dash2DrvReInit(struct rtl8168_private *tp)
//re-init dash2 descriptors/buffers and re-fill the address
{
        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        memset(tp->UnalignedTxDashSendFwDescVa, 0, tp->SizeOfTxDashSendFwDescMemAlloc);
        memset(tp->UnalignedRxDashRecvFwDescVa, 0, tp->SizeOfRxDashRecvFwDescMemAlloc);
        memset(tp->UnalignedSendToFwBufferVa, 0, tp->SizeOfSendToFwBufferMemAlloc);
        memset(tp->UnalignedRecvFromFwBufferVa, 0, tp->SizeOfRecvFromFwBufferMemAlloc);

        tp->CurrNumTxDashSendFwDesc = 0;
        tp->CurrNumRxDashRecvFwDesc = 0;

        NICInitDash2Send(tp);
        NICInitDash2Recv(tp);

        //re-fill tx/rx desc address
        Dash2CmacFillTxRxDescAddress(tp);

        //Setting rxdma and max_payload
        //RTL_CMAC_W8(CMAC_IBCR0, ( BIT_3 | BIT_7 | BIT_5 ));
        RTL_CMAC_W8(CMAC_IBCR0, 0x01);

        //Setting txdma and max_rd_size
        //RTL_CMAC_W8(CMAC_IBCR2, ( BIT_3 | BIT_7 | BIT_5 ));
        RTL_CMAC_W8(CMAC_IBCR2, 0x01);

        Dash2EnableTx( tp );
        Dash2EnableRx( tp );
}

/*
static void Dash2CmacCheckOobCmacStop(struct rtl8168_private *tp)
//check oob cmac engine stopped
{
        u32 TmpUlong;

        if (!HW_DASH_SUPPORT_TYPE_2(tp) && !HW_DASH_SUPPORT_TYPE_3(tp)) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp)) {
                //Check OOB
                //if OOB[0xb2002c20]==CMAC_OOB_STOP, then OOB[0xb2002c20]=0
                TmpUlong = OCP_read(tp, 0x2C20, 4);

                if(TmpUlong == CMAC_OOB_STOP) {
                        OCP_write(tp, 0x2C20, 4, 0x0);
                        tp->CmacOobIssueCmacReset = TRUE;
                }
        } else if (HW_DASH_SUPPORT_TYPE_3(tp)) {
                TmpUlong = OCP_read_with_oob_base_address(tp, CMAC_SYNC_REG, 4, RTL8168FP_KVM_BASE);
                if(TmpUlong == CMAC_OOB_STOP ) {
                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);
                        tp->CmacOobIssueCmacReset = TRUE;
                }
        }
}
*/

void DashHwInit(struct net_device *dev)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        void __iomem *ioaddr = tp->mmio_addr;

        if (!tp->DASH) return;

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                Dash2DrvInformOobToStopCmac(tp); // reset CMAC
        } else {
                NICInitDash2Send(tp);

                NICInitDash2Recv(tp);

                rtl8168_eri_write( ioaddr, SystemSlaveDescStartAddrLow, 4, (tp->RxDashRecvFwDescPhy & DMA_BIT_MASK(32)), ERIAR_ExGMAC);
                rtl8168_eri_write( ioaddr, SystemSlaveDescStartAddrHigh, 4, (tp->RxDashRecvFwDescPhy >> 32), ERIAR_ExGMAC);

                rtl8168_eri_write( ioaddr, SystemMasterDescStartAddrLow, 4, (tp->TxDashSendFwDescPhy & DMA_BIT_MASK(32)), ERIAR_ExGMAC);
                rtl8168_eri_write( ioaddr, SystemMasterDescStartAddrHigh, 4, (tp->TxDashSendFwDescPhy >> 32), ERIAR_ExGMAC);
        }
}

static void DP_IssueSwInterrupt(
        struct rtl8168_private *tp,
        u8 type //interrupt type
)
{
        void __iomem *ioaddr = tp->mmio_addr;

        if( FALSE == HW_DASH_SUPPORT_TYPE_1(tp)) return;

        //software interrupt type
        rtl8168_eri_write(ioaddr, 0xE8, 1, type, ERIAR_ExGMAC);

        //issue software interrupt
        OCP_write(tp, 0x30, 1, 0x01);
}

static void RecvFromDashFw(struct rtl8168_private *tp)
{
        void __iomem *ioaddr = tp->mmio_addr;
        //u16 DashReqRegValue;

        if (!tp->DASH) return;

        if( HW_DASH_SUPPORT_TYPE_1( tp ) ) {
                PRX_DASH_FROM_FW_DESC pDescRxDashFromFw;
                u16 TmpStatus;

                tp->DashReqRegValue = OCP_read(tp, OCP_REG_DASH_REQ, 4);

                pDescRxDashFromFw = tp->RxDashRecvFwDesc;

                pDescRxDashFromFw->BufferAddress = tp->RecvFromFwBufferPhy;
                pDescRxDashFromFw->resv = 0;
                pDescRxDashFromFw->length = RECV_FROM_FW_BUF_SIZE;
                TmpStatus = RX_DASH_FROM_FW_OWN;
                pDescRxDashFromFw->statusLowByte = (u8)TmpStatus;
                pDescRxDashFromFw->statusHighByte = (u8)(TmpStatus>>8);
                RTL_W8(TxPoll, TPPool_HRDY);
        }
}

static void RecvFromDashFwComplete(struct rtl8168_private *tp)
{
        u16 index=0 ;
        u8 DPIssueSwInterrupt ;
        u8 OobReqEnable = FALSE;
        u8 OobAckEnable = FALSE;

        if (!tp->DASH) return;

        if( tp->OobReq ) {
                OobReqEnable = TRUE;
        }

        if( tp->OobAck ) {
                OobAckEnable = TRUE;
        }

        if (tp->dash_printer_enabled) {
                OobReqEnable = TRUE;
                OobAckEnable = TRUE;
        }

        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                for(index=0; index<tp->NumRxDashRecvFwDesc; index++) {
                        PRX_DASH_BUFFER_TYPE_2 pRxDashBufferType2;
                        PRX_DASH_FROM_FW_DESC pDescRxDashFromFw;
                        u16 PacketStatus;
                        u16 TmpStatus;

                        pDescRxDashFromFw = GetDash2RxDesc(tp) ;

                        PacketStatus = le16_to_cpu(pDescRxDashFromFw->statusLowByte+(pDescRxDashFromFw->statusHighByte<<8));
                        if ( PacketStatus & RXS_OWN ) {
                                break;
                        }

                        pRxDashBufferType2 = (PRX_DASH_BUFFER_TYPE_2)GetDash2RxBuffer(tp) ;

                        tp->DashReqRegValue = pRxDashBufferType2->oobhdr.hostReqV;

                        if( OobReqEnable ) {
                                if( tp->DashReqRegValue == 0x91 ) {
                                        tp->AfterRecvFromFwBufLen = pDescRxDashFromFw->length;
                                        memcpy(tp->AfterRecvFromFwBuf, pRxDashBufferType2 ,tp->AfterRecvFromFwBufLen);

                                        tp->OobReqComplete = TRUE;
#if defined(ENABLE_DASH_PRINTER_SUPPORT)
                                        if (tp->dash_printer_enabled)
                                                complete(&tp->fw_req);
#endif
                                }
                        }

                        if( OobAckEnable ) {
                                if( tp->DashReqRegValue == 0x92) {
                                        u8 *pByte;

                                        pByte = GetDash2RxBuffer(tp) ;
                                        if( pByte[4] == OSPUSHDATA ) {
                                                tp->OobAckComplete = TRUE;
                                        }
#if defined(ENABLE_DASH_PRINTER_SUPPORT)
                                        if (tp->dash_printer_enabled)
                                                complete(&tp->fw_ack);
#endif
                                }
                        }

                        pDescRxDashFromFw = GetDash2RxDesc(tp) ;

                        pDescRxDashFromFw->BufferAddress = GetDash2RxBufferPhy(tp) ;
                        pDescRxDashFromFw->resv = 0;
                        pDescRxDashFromFw->length = RECV_FROM_FW_BUF_SIZE;

                        if(IsDash2RxLastDesc(tp)) {
                                TmpStatus = RX_DASH_FROM_FW_OWN | RXS_EOR;
                        } else {
                                TmpStatus = RX_DASH_FROM_FW_OWN ;
                        }
                        pDescRxDashFromFw->statusLowByte = (u8)TmpStatus;
                        pDescRxDashFromFw->statusHighByte = (u8)(TmpStatus>>8);

                        NextDash2RxDesc(tp) ;
                }
        } else {
                if( OobReqEnable ) {
                        if( tp->DashReqRegValue == 0x91 ) {
                                u8 *pRxBuffer ;
                                PRX_DASH_FROM_FW_DESC pDescRxDashFromFw;

                                pDescRxDashFromFw = GetDash2RxDesc(tp) ;
                                pRxBuffer = GetDash2RxBuffer(tp) ;

                                tp->AfterRecvFromFwBufLen = pDescRxDashFromFw->length;
                                memcpy(tp->AfterRecvFromFwBuf, pRxBuffer ,tp->AfterRecvFromFwBufLen);

                                tp->OobReqComplete = TRUE;
                        }
                }

                if( OobAckEnable ) {
                        if( tp->DashReqRegValue == 0x92) {
                                u8 *pByte;

                                pByte = GetDash2RxBuffer(tp) ;
                                if( pByte[4] == OSPUSHDATA ) {
                                        tp->OobAckComplete = TRUE;
                                }

                                DPIssueSwInterrupt= TRUE ;

                        }
                }
        }

        if(DPIssueSwInterrupt) {
                DP_IssueSwInterrupt(tp, 0x07);
        }
}

static int SendToDashFw(struct rtl8168_private *tp, u8 *DataSrc, u16 DataLen, u16 HostReqValue)
{
        void __iomem *ioaddr = tp->mmio_addr;
        PTX_DASH_SEND_FW_DESC pDescTxDashSendFw;
        u16 TmpStatus;
        u16 OcpConfigRegValue;
        int ret = 0;

        if (!tp->DASH) return  -EFAULT;

        do {
                if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                        if (tp->SendingToFw == FALSE) {
                                if (DataLen > SEND_TO_FW_BUF_SIZE) {
                                        ret = -EFAULT;
                                } else {
                                        if (DataLen > 0) {
                                                u8 *tmpTxSendFwBuffer ;
                                                PTX_DASH_SEND_FW_DESC tmpTxSendFwDesc;

                                                tmpTxSendFwBuffer = GetDash2TxBuffer(tp) ;
                                                tmpTxSendFwDesc = GetDash2TxDesc(tp) ;

                                                if(tmpTxSendFwDesc->statusHighByte & TX_DASH_SEND_FW_OWN_HIGHBYTE) {
                                                        ret = -EFAULT ;
                                                } else {
                                                        tp->SendingToFw = TRUE;
                                                        tp->SendToFwBufferLen = DataLen;
                                                        memcpy(tmpTxSendFwBuffer, DataSrc, DataLen);

                                                        pDescTxDashSendFw = tmpTxSendFwDesc;

                                                        pDescTxDashSendFw->BufferAddress = GetDash2TxBufferPhy(tp);
                                                        pDescTxDashSendFw->resv = 0;
                                                        pDescTxDashSendFw->length = DataLen;
                                                        if(IsDash2TxLastDesc(tp)) {
                                                                TmpStatus = TX_DASH_SEND_FW_OWN | TXS_LS | TXS_FS | TXS_EOR;
                                                        } else {
                                                                TmpStatus = TX_DASH_SEND_FW_OWN | TXS_LS | TXS_FS ;
                                                        }
                                                        pDescTxDashSendFw->statusLowByte = (u8)TmpStatus;

                                                        wmb();

                                                        pDescTxDashSendFw->statusHighByte = (u8)(TmpStatus>>8);

                                                        wmb();

                                                        Dash2WriteTxPollingBit(tp);  //Write Tx Polling Bit

                                                        tp->LastSendNumTxDashSendFwDesc = tp->CurrNumTxDashSendFwDesc;

                                                        NextDash2TxDesc(tp) ;
                                                }
                                        } else {
                                                ret = -EFAULT;
                                        }
                                }
                        }
                } else {
                        u16 OcpRegConfig0;

                        //OcpRegConfig0
                        switch(tp->mcfg) {
                        case CFG_METHOD_11:
                        case CFG_METHOD_12:
                                OcpRegConfig0 = OCP_REG_CONFIG0;
                                break;
                        default:
                                OcpRegConfig0 = OCP_REG_CONFIG0_REV_F;
                                break;
                        }

                        OcpConfigRegValue = OCP_read(tp, OcpRegConfig0, 4);
                        if( !(OcpConfigRegValue & OCP_REG_CONFIG0_FIRMWARERDY) ) {
                                //Firmware not ready
                                ret = -EFAULT;
                                break;
                        }

                        if ( tp->SendingToFw == FALSE ) {
                                if (DataLen > SEND_TO_FW_BUF_SIZE) {
                                        ret = -EFAULT;
                                } else {
                                        if (DataLen > 0) {
                                                tp->SendingToFw = TRUE;

                                                tp->SendToFwBufferLen = DataLen;

                                                memcpy(tp->SendToFwBuffer, DataSrc, DataLen);

                                                pDescTxDashSendFw = tp->TxDashSendFwDesc;

                                                pDescTxDashSendFw->BufferAddress = tp->SendToFwBufferPhy;
                                                pDescTxDashSendFw->resv = 0;
                                                pDescTxDashSendFw->length = DataLen;
                                                TmpStatus = TX_DASH_SEND_FW_OWN;
                                                pDescTxDashSendFw->statusLowByte = (u8)TmpStatus;
                                                pDescTxDashSendFw->statusHighByte = (u8)(TmpStatus>>8);

                                                tp->HostReqValue = HostReqValue;

                                                rtl8168_eri_write(ioaddr, HostReqReg, 2, HostReqValue, ERIAR_ExGMAC);
                                        } else {
                                                ret = -EFAULT;
                                        }
                                }
                        } else {
                                ret = -EFAULT;
                        }
                }
        } while(FALSE);

        return ret;

}

static void SendToDashFwComplete(struct rtl8168_private *tp)
{
        if (!tp->DASH) return;

        do {
                if ( tp->SendingToFw ) {
                        PTX_DASH_SEND_FW_DESC pDescTxDashSendFw;
                        u8 *pTxDashSendFwBuffer ;

                        tp->SendingToFw = FALSE;

                        if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                                pDescTxDashSendFw = GetLastSendDash2TxDesc(tp);
                                pTxDashSendFwBuffer = GetLastSendDash2TxBuffer(tp);
                        } else {
                                pDescTxDashSendFw = tp->TxDashSendFwDesc;
                                pTxDashSendFwBuffer = tp->SendToFwBuffer ;
                        }

                        if (pDescTxDashSendFw->length < SEND_TO_FW_BUF_SIZE && pDescTxDashSendFw->length > 0) {
                                tp->AfterSendToFwBufLen = pDescTxDashSendFw->length;
                                memcpy(tp->AfterSendToFwBuf, pTxDashSendFwBuffer, tp->AfterSendToFwBufLen);
                        }
                }
        } while(FALSE);
}

void HandleDashInterrupt(struct net_device *dev)
{
        struct rtl8168_private *tp = netdev_priv(dev);

        if(tp->DASH) {
                if (tp->RcvFwDashOkEvt) {
                        tp->RcvFwDashOkEvt = FALSE;
                        RecvFromDashFwComplete(tp);
                }

                if (tp->RcvFwReqSysOkEvt) {
                        tp->RcvFwReqSysOkEvt = FALSE;
                        RecvFromDashFw(tp);
                }

                if (tp->SendFwHostOkEvt) {
                        tp->SendFwHostOkEvt = FALSE;
                        SendToDashFwComplete(tp);
#if defined(ENABLE_DASH_PRINTER_SUPPORT)
                        if (tp->dash_printer_enabled)
                                complete(&tp->fw_host_ok);
#endif
                }

                /*
                if (tp->CmacResetIntr) {
                tp->CmacResetIntr= FALSE ;

                Dash2CmacCheckOobIssueCmacReset(tp);

                if(tp->CmacOobIssueCmacReset) {
                tp->CmacResetIsrCounter = 1;
                Dash2DrvInformOobToStopCmac(tp);
                }

                Dash2CmacCheckOobCmacStop(tp);
                Dash2CmacHwReset(tp) ;
                Dash2DrvReInit(tp);
                Dash2DrvInformOobReInit(tp);
                }
                */

                if (tp->CmacResetIntr) {
                        u32 TmpUlong;

                        tp->CmacResetIntr = FALSE;

                        if(HW_DASH_SUPPORT_TYPE_2(tp)) {
                                // check oob
                                // if OOB[0xb2002c20] == CMAC_OOB_RESET, CMAC_OOB_STOP, CMAC_OOB_INIT
                                TmpUlong = OCP_read(tp, 0x2C20, 4);
                                if (TmpUlong == CMAC_OOB_RESET) {
                                        OCP_write(tp, 0x2C20, 4, 0x0);
                                        Dash2DrvInformOobToStopCmac(tp);
                                } else if (TmpUlong == CMAC_OOB_STOP) {
                                        OCP_write(tp, 0x2C20, 4, 0x0);
                                        Dash2CmacHwReset(tp);
                                        Dash2DrvReInit(tp);
                                        Dash2DrvInformOobReInit(tp);
                                } else if (TmpUlong == CMAC_OOB_INIT) {
                                        OCP_write(tp, 0x2C20, 4, 0x0);
                                        Dash2CmacFillTxRxDescAddress(tp);
                                        tp->CmacResetting = FALSE;
                                }
                        } else if(HW_DASH_SUPPORT_TYPE_3(tp)) {
                                TmpUlong = OCP_read_with_oob_base_address(tp, CMAC_SYNC_REG, 4, RTL8168FP_KVM_BASE);

                                if(TmpUlong == CMAC_OOB_RESET) {
                                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);

                                        tp->CmacResetbyFwCnt++;

                                        Dash2DrvInformOobToStopCmac(tp);
                                } else if (TmpUlong == CMAC_OOB_STOP) {
                                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);

                                        Dash2CmacHwReset(tp);
                                        Dash2DrvReInit(tp);
                                        Dash2DrvInformOobReInit(tp);
                                } else if (TmpUlong == CMAC_OOB_INIT) {
                                        OCP_write_with_oob_base_address(tp, CMAC_SYNC_REG, 4, 0x0, RTL8168FP_KVM_BASE);

                                        tp->CmacResetting = FALSE ;
                                        Dash2CmacFillTxRxDescAddress(tp);
                                }
                        }
                }

                if (HW_DASH_SUPPORT_TYPE_2(tp) || HW_DASH_SUPPORT_TYPE_3(tp)) {
                        if( tp->DashFwDisableRx ) {
                                tp->DashFwDisableRx = FALSE;
                                Dash2ResetTx( tp );
                                Dash2CmacFillTxRxDescAddress(tp);
                        }
                }
        }
}

#if !defined(ENABLE_DASH_PRINTER_SUPPORT)
static int DashIoctlGetRcvFromFwData(struct net_device *dev, struct rtl_dash_ioctl_struct *prtl_dash_usrdata)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        u32 ulInfoLen;
        void *InformationBuffer;
        u32 InformationBufferLength;
        void *pInfo;
        u8 *pByte;
        u16 *pWord;
        unsigned long flags;
        int ret = -EFAULT;

        InformationBufferLength = prtl_dash_usrdata->len;
        InformationBuffer = prtl_dash_usrdata->data_buffer;

        spin_lock_irqsave(&tp->lock, flags);
        do {
                if (tp->DASH) {
                        if (tp->AfterRecvFromFwBufLen > 0) {
                                ulInfoLen = tp->AfterRecvFromFwBufLen + 2 + 2;
                                if ( InformationBufferLength < ulInfoLen ) {
                                        ret = -EFAULT;
                                        break;
                                } else {
                                        if ( tp->rtk_enable_diag ) {
                                                u8 *tmpBuf;

                                                if (!(tmpBuf = kmalloc(ulInfoLen, GFP_ATOMIC))) {
                                                        ret = -ENOMEM;
                                                        break;
                                                }
                                                pInfo = ( void* ) tp->AfterRecvFromFwBuf;
                                                pWord = ( u16* ) tmpBuf;
                                                *pWord++ = tp->AfterRecvFromFwBufLen;
                                                pByte = ( u8* )pWord;
                                                memcpy(pByte, pInfo, tp->AfterRecvFromFwBufLen);
                                                pWord = (u16*)(pByte + tp->AfterRecvFromFwBufLen);
                                                *pWord= tp->DashReqRegValue;
                                                tp->AfterRecvFromFwBufLen = 0;
                                                spin_unlock_irqrestore(&tp->lock, flags);
                                                if (copy_to_user(InformationBuffer, tmpBuf, ulInfoLen)) {
                                                        kfree(tmpBuf);
                                                        ret = -EFAULT;
                                                        spin_lock_irqsave(&tp->lock, flags);
                                                        break;
                                                }
                                                spin_lock_irqsave(&tp->lock, flags);
                                                kfree(tmpBuf);
                                                ret = 0;
                                        } else {
                                                ret = -EFAULT;
                                        }
                                }
                        } else {
                                ret = -EFAULT;
                        }
                } else {
                        ret = -EFAULT;
                }
        } while(FALSE);

        spin_unlock_irqrestore(&tp->lock, flags);

        return ret;
}

static int DashIoctlCheckSendBufferToFwComplete(struct net_device *dev, struct rtl_dash_ioctl_struct *prtl_dash_usrdata)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        u32 ulInfoLen;
        void *InformationBuffer;
        u32 InformationBufferLength;
        u16 *pWord;
        unsigned long flags;
        int ret = -EFAULT;

        InformationBufferLength = prtl_dash_usrdata->len;
        InformationBuffer = prtl_dash_usrdata->data_buffer;

        spin_lock_irqsave(&tp->lock, flags);

        do {
                if (tp->DASH) {
                        if (tp->SendingToFw == FALSE)
                                ulInfoLen = tp->AfterSendToFwBufLen + sizeof( u16 );
                        else
                                ulInfoLen = sizeof( u16 );

                        if ( InformationBufferLength < ulInfoLen ) {
                                ret = -EFAULT;
                                break;
                        }

                        if ( tp->rtk_enable_diag ) {
                                u8 *tmpBuf;

                                if (!(tmpBuf = kmalloc(ulInfoLen, GFP_ATOMIC))) {
                                        ret = -ENOMEM;
                                        break;
                                }

                                pWord = ( u16* ) tmpBuf;
                                if (tp->SendingToFw == FALSE) {
                                        *pWord++ = tp->AfterSendToFwBufLen;
                                        memcpy(pWord, tp->AfterSendToFwBuf, tp->AfterSendToFwBufLen);
                                        tp->AfterSendToFwBufLen = 0;
                                } else {
                                        *pWord = 0xffff;
                                }

                                spin_unlock_irqrestore(&tp->lock, flags);
                                if (copy_to_user(InformationBuffer, tmpBuf, ulInfoLen))
                                        ret = -EFAULT;
                                else
                                        ret = 0;
                                spin_lock_irqsave(&tp->lock, flags);

                                kfree(tmpBuf);
                        } else {
                                ret = -EFAULT;
                        }
                } else {
                        ret = -EFAULT;
                }
        } while(FALSE);

        spin_unlock_irqrestore(&tp->lock, flags);

        return ret;
}

static int DashIoctlCheckSendBufferToFw(struct net_device *dev, struct rtl_dash_ioctl_struct *prtl_dash_usrdata)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        u32 ulInfoLen;
        void *InformationBuffer;
        u32 InformationBufferLength;
        u8 *pByte;
        u16 *pWord;
        u16 SetDataSize;
        u32 SetDataValue;
        unsigned long flags;
        int ret = -EFAULT;

        InformationBufferLength = prtl_dash_usrdata->len;
        if (!(InformationBuffer = kmalloc(InformationBufferLength, GFP_KERNEL)))
                return -ENOMEM;

        if (copy_from_user(InformationBuffer, prtl_dash_usrdata->data_buffer, InformationBufferLength)) {
                kfree(InformationBuffer);
                return -EFAULT;
        }

        spin_lock_irqsave(&tp->lock, flags);

        do {
                if (tp->DASH) {
                        ulInfoLen = sizeof( u16 ) + sizeof( u16 );

                        if ( InformationBufferLength < ulInfoLen ) {
                                break;
                        }

                        if ( tp->rtk_enable_diag ) {
                                pWord = ( u16* ) InformationBuffer;
                                SetDataSize = *pWord;

                                if (InformationBufferLength < ( SetDataSize + sizeof( u16 ) + sizeof( u16 ) )) {
                                        ret = -EFAULT;
                                } else {
                                        pWord = ( u16* ) InformationBuffer;
                                        SetDataSize = *pWord++;
                                        pByte = (u8*)pWord;
                                        pByte += SetDataSize;
                                        pWord = (u16*)pByte;
                                        SetDataValue = (u16)*pWord;
                                        pWord = ( u16* ) InformationBuffer;
                                        pWord++;
                                        ret = SendToDashFw(tp, (u8*)pWord, SetDataSize, (u16) SetDataValue);
                                }
                        } else {
                                ret = -EFAULT;
                        }
                } else {
                        ret = -EFAULT;
                }
        } while(FALSE);

        spin_unlock_irqrestore(&tp->lock, flags);

        kfree(InformationBuffer);

        return ret;
}

static int
OOB_set_ip_mac(struct rtl8168_private *tp, struct sockaddr_in *sa, u8 *mac)
{
        u32 data;

        if (tp->mcfg == CFG_METHOD_13) {
                OCP_write(tp, 0xd0, 4, be32_to_cpu(sa->sin_addr.s_addr));

                memcpy(&data, mac, 4);
                OCP_write(tp, 0x00, 4, le32_to_cpu(data));
                data = 0;
                memcpy(&data, mac + 4, 2);
                OCP_write(tp, 0x04, 2, le32_to_cpu(data));

                OOB_notify(tp, OOB_CMD_SET_IPMAC);
        } else if (tp->mcfg == CFG_METHOD_17) {
                void __iomem *ioaddr = tp->mmio_addr;
                struct net_device *dev = tp->dev;
                u32 rx_mode;

                rx_mode = RTL_R32(RxConfig);
                if (netif_running(dev)) {
                        netif_stop_queue(dev);
                        RTL_W32(RxConfig, rx_mode & ~0x3f);
                        while ((RTL_R8(0xd3) & (BIT_5 | BIT_4)) != ((BIT_5 | BIT_4)))
                                udelay(20);
                        RTL_W8(ChipCmd, RTL_R8(ChipCmd) & ~(CmdTxEnb | CmdRxEnb));
//		} else {
//			unsigned long flags;
//
//			spin_lock_irqsave(&tp->phy_lock, flags);
//			mdio_write(tp, 0x1f, 0x0000);
//			data = mdio_read(tp, MII_CTRL1000);
//			data &=	~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
//			mdio_write(tp, MII_CTRL1000, data);
//			mdio_write(tp, 0x00, 0x9200);
//			spin_unlock_irqrestore(&tp->phy_lock, flags);
//
//			sleep(3);
//			RTL_W16(IntrStatus, RTL_R16(IntrStatus));
//
//			RTL_W32(MAR0, 0);
//			RTL_W32(MAR0 + 4, 0);
//			RTL_W16(RxMaxSize, 0x05f3);
                }
                RTL_W8(0xD3, RTL_R8(0xD3) & ~BIT_7);
                rtl8168_eri_write(ioaddr, 0x180, 4, 0x06080888, ERIAR_ExGMAC);
                rtl8168_eri_write(ioaddr, 0x184, 4, 0xdd860008, ERIAR_ExGMAC);

                memcpy(&data, mac, 2);
                rtl8168_eri_write(ioaddr, 0xf0, 4, (le32_to_cpu(data) << 16), ERIAR_ExGMAC);
                memcpy(&data, mac + 2, 4);
                rtl8168_eri_write(ioaddr, 0xf4, 4, le32_to_cpu(data), ERIAR_ExGMAC);

                rtl8168_eri_write(ioaddr, 0x190, 4, 0x3c110600, ERIAR_ExGMAC);
                rtl8168_eri_write(ioaddr, 0x194, 4, 0x2c32332b, ERIAR_ExGMAC);
                rtl8168_eri_write(ioaddr, 0x198, 4, 0x003a0201, ERIAR_ExGMAC);
                rtl8168_eri_write(ioaddr, 0x19c, 4, 0x00000000, ERIAR_ExGMAC);

                rtl8168_eri_write(ioaddr, 0x1f0, 4, cpu_to_le32(sa->sin_addr.s_addr), ERIAR_ExGMAC);

                memcpy(&data, mac, 4);
                rtl8168_eri_write(ioaddr, 0x258, 4, le32_to_cpu(data), ERIAR_ExGMAC);
                memcpy(&data, mac + 4, 2);
                rtl8168_eri_write(ioaddr, 0x25c, 2, le32_to_cpu(data), ERIAR_ExGMAC);

                RTL_W8(0xe0, RTL_R8(0xe0) | BIT_6);
                while (!(RTL_R8(0xd3) & BIT_1))
                        udelay(20);

                RTL_W32(0xb0, 0x9800e035);
                RTL_W32(0xb0, 0x9801e034);
                RTL_W32(0xb0, 0x9802e019);
                RTL_W32(0xb0, 0x98039918);
                RTL_W32(0xb0, 0x9804c011);
                RTL_W32(0xb0, 0x98057100);
                RTL_W32(0xb0, 0x9806499f);
                RTL_W32(0xb0, 0x9807f011);
                RTL_W32(0xb0, 0x9808c00e);
                RTL_W32(0xb0, 0x98097100);
                RTL_W32(0xb0, 0x980A4995);
                RTL_W32(0xb0, 0x980Bf00d);
                RTL_W32(0xb0, 0x980C4895);
                RTL_W32(0xb0, 0x980D9900);
                RTL_W32(0xb0, 0x980Ec009);
                RTL_W32(0xb0, 0x980F7100);
                RTL_W32(0xb0, 0x98104890);
                RTL_W32(0xb0, 0x98119900);
                RTL_W32(0xb0, 0x98124810);
                RTL_W32(0xb0, 0x98139900);
                RTL_W32(0xb0, 0x9814e004);
                RTL_W32(0xb0, 0x9815d44e);
                RTL_W32(0xb0, 0x9816d506);
                RTL_W32(0xb0, 0x9817c0b4);
                RTL_W32(0xb0, 0x9818c002);
                RTL_W32(0xb0, 0x9819b800);
                RTL_W32(0xb0, 0x981A0500);
                RTL_W32(0xb0, 0x981B1a26);
                RTL_W32(0xb0, 0x981Ca4ca);
                RTL_W32(0xb0, 0x981D21bc);
                RTL_W32(0xb0, 0x981E25bc);
                RTL_W32(0xb0, 0x981F1305);
                RTL_W32(0xb0, 0x9820f00d);
                RTL_W32(0xb0, 0x9821c213);
                RTL_W32(0xb0, 0x98227340);
                RTL_W32(0xb0, 0x982349b0);
                RTL_W32(0xb0, 0x9824f009);
                RTL_W32(0xb0, 0x98251a3a);
                RTL_W32(0xb0, 0x9826a4ca);
                RTL_W32(0xb0, 0x982721b9);
                RTL_W32(0xb0, 0x982825b9);
                RTL_W32(0xb0, 0x98291303);
                RTL_W32(0xb0, 0x982Af006);
                RTL_W32(0xb0, 0x982B1309);
                RTL_W32(0xb0, 0x982Cf004);
                RTL_W32(0xb0, 0x982Dc306);
                RTL_W32(0xb0, 0x982E1a26);
                RTL_W32(0xb0, 0x982Fbb00);
                RTL_W32(0xb0, 0x9830c302);
                RTL_W32(0xb0, 0x9831bb00);
                RTL_W32(0xb0, 0x98320f3e);
                RTL_W32(0xb0, 0x98330f4e);
                RTL_W32(0xb0, 0x9834c0ae);
                RTL_W32(0xb0, 0x98351800);
                RTL_W32(0xb0, 0x9836b800);
                RTL_W32(0xb0, 0xfe173000);
                RTL_W32(0xb0, 0xfe1604ff);
                RTL_W32(0xb0, 0xfe150f4d);
                data = rtl8168_eri_read(ioaddr, 0xd6, 1, ERIAR_ExGMAC);
                rtl8168_eri_write(ioaddr, 0xd6, 1, data | BIT_0, ERIAR_ExGMAC);

                if (netif_running(dev)) {
                        rtl8168_init_ring_indexes(tp);
                        RTL_W8(ChipCmd, CmdRxEnb | CmdTxEnb);
                        RTL_W32(RxConfig, rx_mode);
                        netif_wake_queue(dev);
                } else {
                        RTL_W8(0xD3, RTL_R8(0xD3) | BIT_7);

//			data = rtl8168_eri_read(ioaddr, 0xDC, 1, ERIAR_ExGMAC);
//			data &= ~BIT_0;
//			rtl8168_eri_write( ioaddr, 0xDC, 1, data, ERIAR_ExGMAC);
//			data |= BIT_0;
//			rtl8168_eri_write( ioaddr, 0xDC, 1, data, ERIAR_ExGMAC);

                        RTL_W32(RxConfig, rx_mode | 0x0e);
                        rtl8168_eri_write(ioaddr, 0x2F8, 1, 0x0064, ERIAR_ExGMAC);
                }
        } else {
                return -EFAULT;
        }
        return 0;
}

#else

static int cmac_to_fw(struct rtl8168_private *tp, void *buf, u32 len, u8 type)
{
        unsigned long flags;
        __le32 *plen = buf;
        u8 *data = buf;
        int ret;
        long t;

        if (len < 8) {
                netif_err(tp, drv, tp->dev, "invalid length = %d\n", len);
                ret = -EINVAL;
                goto out;
        }

        reinit_completion(&tp->fw_host_ok);
        reinit_completion(&tp->fw_ack);

        plen = (__le32 *)data;
        *plen = __cpu_to_le32(len - 8);
        data[4] = type;
        data[5] = 0xf0;
        data[6] = 0x92;
        data[7] = 0xba;

        spin_lock_irqsave(&tp->lock, flags);
        ret = SendToDashFw(tp, buf, len, 0);
        spin_unlock_irqrestore(&tp->lock, flags);

        t = wait_for_completion_interruptible_timeout(&tp->fw_host_ok, HZ * 5);
        if (!t) {
                ret = -ETIMEDOUT;
                goto out;
        } else if (t < 0) {
                ret = t;
                goto out;
        }

        t = wait_for_completion_interruptible_timeout(&tp->fw_ack, HZ * 5);
        if (!t)
                ret = -ETIMEDOUT;
        else if (t < 0)
                ret = t;

out:
        return ret;
}

static int settings_to_fw(struct rtl8168_private *tp, const void __user *from,
                          u32 len, u8 type)
{
        u8 *data;
        int ret;

        if (tp->CmacResetting) {
                ret = -EBUSY;
                goto out1;
        }

        data = kmalloc(len + 8, GFP_KERNEL);
        if (!data) {
                ret = -ENOMEM;
                goto out1;
        }

        if (copy_from_user(data + 8, from, len)) {
                ret = -EFAULT;
                goto out2;
        }

        ret = cmac_to_fw(tp, data, len + 8, type);

out2:
        kfree(data);
out1:
        return ret;
}

static int settings_from_fw(struct rtl8168_private *tp, void __user *to, u32 len,
                            u8 type, void *extend, u32 ext_len)
{
        unsigned long flags;
        u8 *data;
        int ret;
        long t;

        if (tp->CmacResetting) {
                ret = -EBUSY;
                goto out1;
        }

        data = kmalloc(ext_len + 8, GFP_KERNEL);
        if (!data) {
                ret = -ENOMEM;
                goto out1;
        }

        if (extend && ext_len)
                memcpy(&data[8], extend, ext_len);
        else
                ext_len = 0;

        reinit_completion(&tp->fw_req);

        ret = cmac_to_fw(tp, data, ext_len + 8, type);
        if (ret < 0)
                goto out2;

        t = wait_for_completion_interruptible_timeout(&tp->fw_req, HZ * 5);
        if (!t) {
                ret = -ETIMEDOUT;
                goto out2;
        } else if (t < 0) {
                ret = t;
                goto out2;
        }

        spin_lock_irqsave(&tp->lock, flags);
        if (to && len) {
                u8 *buf;
                int BufLen = tp->AfterRecvFromFwBufLen;

                if (BufLen < 8) {
                        ret = -EFAULT;
                        goto out3;
                } else if (len < BufLen - 8) {
                        ret = -EINVAL;
                        goto out3;
                }

                buf = kmalloc(BufLen - 8, GFP_ATOMIC);
                if (!buf) {
                        ret = -ENOMEM;
                        goto out3;
                }

                memcpy(buf, tp->AfterRecvFromFwBuf + 8, BufLen - 8);
                spin_unlock_irqrestore(&tp->lock, flags);
                if (copy_to_user(to, buf, BufLen - 8))
                        ret = -EFAULT;
                kfree(buf);
                spin_lock_irqsave(&tp->lock, flags);
        }

out3:
        tp->AfterRecvFromFwBufLen = 0;
        spin_unlock_irqrestore(&tp->lock, flags);

        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
        data[6] = 0x91;

        spin_lock_irqsave(&tp->lock, flags);
        SendToDashFw(tp, data, 8, 0);
        spin_unlock_irqrestore(&tp->lock, flags);

        t = wait_for_completion_interruptible_timeout(&tp->fw_host_ok, HZ * 5);
        if (!t)
                ret = -ETIMEDOUT;
        else if (t < 0)
                ret = t;

out2:
        kfree(data);
out1:
        return ret;
}
#endif  // !defined(ENABLE_DASH_PRINTER_SUPPORT)

int rtl8168_dash_ioctl(struct net_device *dev, struct ifreq *ifr)
{
        struct rtl8168_private *tp = netdev_priv(dev);
        void *user_data = ifr->ifr_data;
        struct rtl_dash_ioctl_struct rtl_dash_usrdata;
#if !defined(ENABLE_DASH_PRINTER_SUPPORT)
        unsigned long flags;
#else
        u32 data32;
        u16 data16;
        u8 data8;
#endif
        int ret=0;

        if (FALSE == HW_DASH_SUPPORT_DASH(tp))
                return -EOPNOTSUPP;

        if (copy_from_user(&rtl_dash_usrdata, user_data, sizeof(struct rtl_dash_ioctl_struct)))
                return -EFAULT;

        switch (rtl_dash_usrdata.cmd) {
#if !defined(ENABLE_DASH_PRINTER_SUPPORT)
        case RTL_DASH_ARP_NS_OFFLOAD:
                break;

        case RTL_DASH_SET_OOB_IPMAC:
                if (rtl_dash_usrdata.len < sizeof(struct rtl_dash_ip_mac))
                        return -EINVAL;

                {
                        struct rtl_dash_ip_mac *dash_ip_mac;

                        if (!(dash_ip_mac = kmalloc(rtl_dash_usrdata.len, GFP_KERNEL)))
                                return -ENOMEM;

                        if (copy_from_user(dash_ip_mac, rtl_dash_usrdata.data_buffer, rtl_dash_usrdata.len)) {
                                kfree(dash_ip_mac);
                                return -EFAULT;
                        }

                        spin_lock_irqsave(&tp->lock, flags);
                        ret = OOB_set_ip_mac(tp,
                                             (struct sockaddr_in *)&dash_ip_mac->ifru_addr,
                                             dash_ip_mac->ifru_hwaddr.sa_data);
                        spin_unlock_irqrestore(&tp->lock, flags);

                        kfree(dash_ip_mac);
                }
                break;

        case RTL_DASH_NOTIFY_OOB:
                spin_lock_irqsave(&tp->lock, flags);
                OOB_mutex_lock(tp);
                OOB_notify(tp, rtl_dash_usrdata.data);
                OOB_mutex_unlock(tp);
                spin_unlock_irqrestore(&tp->lock, flags);
                break;

        case RTL_DASH_SEND_BUFFER_DATA_TO_DASH_FW:
                ret = DashIoctlCheckSendBufferToFw(dev, &rtl_dash_usrdata);
                break;

        case RTL_DASH_CHECK_SEND_BUFFER_TO_DASH_FW_COMPLETE:
                ret = DashIoctlCheckSendBufferToFwComplete(dev, &rtl_dash_usrdata);
                break;

        case RTL_DASH_GET_RCV_FROM_FW_BUFFER_DATA:
                ret = DashIoctlGetRcvFromFwData(dev, &rtl_dash_usrdata);
                break;

        case RTL_DASH_OOB_REQ:
                if (!tp->DASH)
                        return -EINVAL;

                spin_lock_irqsave(&tp->lock, flags);
                tp->OobReq = TRUE;
                tp->OobReqComplete = FALSE;
                spin_unlock_irqrestore(&tp->lock, flags);
                break;

        case RTL_DASH_OOB_ACK:
                if (!tp->DASH)
                        return -EINVAL;

                spin_lock_irqsave(&tp->lock, flags);
                tp->OobAck = TRUE;
                tp->OobAckComplete = FALSE;
                spin_unlock_irqrestore(&tp->lock, flags);
                break;

        case RTL_DASH_DETACH_OOB_REQ:
                if (!tp->DASH)
                        return -EINVAL;

                spin_lock_irqsave(&tp->lock, flags);
                tp->OobReq = FALSE;
                tp->OobReqComplete = FALSE;
                spin_unlock_irqrestore(&tp->lock, flags);
                break;

        case RTL_DASH_DETACH_OOB_ACK:
                if (!tp->DASH)
                        return -EINVAL;

                spin_lock_irqsave(&tp->lock, flags);
                tp->OobAck = FALSE;
                tp->OobAckComplete = FALSE;
                spin_unlock_irqrestore(&tp->lock, flags);
                break;
#else
        case RTL_FW_SET_IPV4:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len != sizeof(struct settings_ipv4))
                        return -EINVAL;

                ret = settings_to_fw(tp, rtl_dash_usrdata.data_buffer,
                                     rtl_dash_usrdata.len, 11);

                break;

        case RTL_FW_GET_IPV4:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len < sizeof(struct settings_ipv4))
                        return -EINVAL;

                ret = settings_from_fw(tp, rtl_dash_usrdata.data_buffer,
                                       rtl_dash_usrdata.len, 12, 0, 0);

                break;

        case RTL_FW_SET_IPV6:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len < sizeof(struct settings_ipv6))
                        return -EINVAL;

                ret = settings_to_fw(tp, rtl_dash_usrdata.data_buffer,
                                     rtl_dash_usrdata.len, 13);

                break;

        case RTL_FW_GET_IPV6:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len < sizeof(struct settings_ipv6))
                        return -EINVAL;

                data32 = 0;
                ret = settings_from_fw(tp, rtl_dash_usrdata.data_buffer,
                                       rtl_dash_usrdata.len, 14, &data32,
                                       sizeof(data32));

                break;

        case RTL_FW_SET_EXT_SNMP:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len < sizeof(struct settings_ext_snmp))
                        return -EINVAL;

                ret = settings_to_fw(tp, rtl_dash_usrdata.data_buffer,
                                     rtl_dash_usrdata.len, 20);

                break;

        case RTL_FW_GET_EXT_SNMP:
                if (!tp->dash_printer_enabled)
                        return -EINVAL;

                if (rtl_dash_usrdata.len < sizeof(struct settings_ext_snmp))
                        return -EINVAL;

                data16 = __cpu_to_le16((u16)rtl_dash_usrdata.offset);

                ret = settings_from_fw(tp, rtl_dash_usrdata.data_buffer,
                                       rtl_dash_usrdata.len, 21, &data16,
                                       sizeof(data16));

                break;

        case RTL_FW_SET_WAKEUP_PATTERN:
                if (rtl_dash_usrdata.len != sizeof(struct wakeup_pattern)) {
                        ret = -EINVAL;
                        break;
                }

                ret = settings_to_fw(tp, rtl_dash_usrdata.data_buffer,
                                     rtl_dash_usrdata.len, 24);

                break;

        case RTL_FW_GET_WAKEUP_PATTERN:
                if (rtl_dash_usrdata.len < sizeof(struct wakeup_pattern)) {
                        ret = -EINVAL;
                        break;
                }

                data8 = (u8)rtl_dash_usrdata.offset;

                ret = settings_from_fw(tp, rtl_dash_usrdata.data_buffer,
                                       rtl_dash_usrdata.len, 25, &data8,
                                       sizeof(data8));

                break;

        case RTL_FW_DEL_WAKEUP_PATTERN:
                if (rtl_dash_usrdata.len < sizeof(struct wakeup_pattern)) {
                        ret = -EINVAL;
                        break;
                }

                data8 = (u8)rtl_dash_usrdata.offset;

                ret = settings_from_fw(tp, rtl_dash_usrdata.data_buffer,
                                       rtl_dash_usrdata.len, 27, &data8,
                                       sizeof(data8));

                break;

#endif
        default:
                return -EOPNOTSUPP;
        }

        return ret;
}
