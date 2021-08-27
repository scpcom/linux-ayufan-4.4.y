#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvSFlashSpecH
#define __INCmvSFlashSpecH

#ifdef __cplusplus
extern "C" {
#endif

#define		MV_SFLASH_READ_CMND_LENGTH		    5		 
#define		MV_SFLASH_SE_CMND_LENGTH		    5		 
#define		MV_SFLASH_BE_CMND_LENGTH		    1		 
#define		MV_SFLASH_PP_CMND_LENGTH		    5		 
#define		MV_SFLASH_WREN_CMND_LENGTH		    1		 
#define		MV_SFLASH_WRDI_CMND_LENGTH		    1		 
#define		MV_SFLASH_RDID_CMND_LENGTH		    1		 
#define		MV_SFLASH_RDID_REPLY_LENGTH		    3		 
#define		MV_SFLASH_RDSR_CMND_LENGTH		    1		 
#define		MV_SFLASH_RDSR_REPLY_LENGTH		    1		 
#define		MV_SFLASH_WRSR_CMND_LENGTH		    2		 
#define		MV_SFLASH_DP_CMND_LENGTH		    1		 
#define		MV_SFLASH_RES_CMND_LENGTH		    1		 

#define		MV_SFLASH_STATUS_REG_WIP_OFFSET	    0	     
#define		MV_SFLASH_STATUS_REG_WP_OFFSET	    2        
#define		MV_SFLASH_STATUS_REG_SRWD_OFFSET	7	     
#define		MV_SFLASH_STATUS_REG_WIP_MASK	    (0x1 << MV_SFLASH_STATUS_REG_WIP_OFFSET)
#define		MV_SFLASH_STATUS_REG_SRWD_MASK	    (0x1 << MV_SFLASH_STATUS_REG_SRWD_OFFSET)

#define		MV_SFLASH_MAX_WAIT_LOOP			    1000000
#define     MV_SFLASH_CHIP_ERASE_MAX_WAIT_LOOP  0x50000000

#define		MV_SFLASH_DEFAULT_RDID_OPCD		    0x9F	 
#define     MV_SFLASH_DEFAULT_WREN_OPCD         0x06	 
#define     MV_SFLASH_NO_SPECIFIC_OPCD          0x00

#ifdef CONFIG_SYNO_ARMADA
#define GD_SFLASH_STATUS_REG_WEL_MASK			0x2
#endif
 




#define     MV_M25PXXX_ST_MANF_ID               0x20
#ifdef CONFIG_SYNO_ARMADA
#define     MV_M25P80_DEVICE_ID                 0x2014
#define     MV_M25P80_MAX_SPI_FREQ              20000000     
#define     MV_M25P80_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25P80_FAST_READ_DUMMY_BYTES     1
#define     MV_M25P80_SECTOR_SIZE               0x10000      
#define     MV_M25P80_SECTOR_NUMBER             16
#endif
#define     MV_M25P32_DEVICE_ID                 0x2016
#define     MV_M25P32_MAX_SPI_FREQ              20000000     
#define     MV_M25P32_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25P32_FAST_READ_DUMMY_BYTES     1
#define	    MV_M25P32_ADDR_CYC_CNT	        3
#define     MV_M25P64_DEVICE_ID                 0x2017
#ifdef CONFIG_SYNO_ARMADA
#define     MV_NU25P64_DEVICE_ID                 0xBA17
#endif
#define     MV_M25P64_MAX_SPI_FREQ              20000000     
#define     MV_M25P64_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25P64_FAST_READ_DUMMY_BYTES     1
#define	    MV_M25P64_ADDR_CYC_CNT	        3
#define     MV_M25P128_DEVICE_ID                0x2018
#define     MV_M25P128_MAX_SPI_FREQ             20000000     
#define     MV_M25P128_MAX_FAST_SPI_FREQ        50000000     
#define     MV_M25P128_FAST_READ_DUMMY_BYTES    1
#define	    MV_M25P128_ADDR_CYC_CNT	        3
#define     MV_M25PX64_DEVICE_ID                 0x7117
#define     MV_M25PX64_MAX_SPI_FREQ              20000000     
#define     MV_M25PX64_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25PX64_FAST_READ_DUMMY_BYTES     1
#define	    MV_M25PX64_ADDR_CYC_CNT	         3

#define     MV_M25Q128_DEVICE_ID                 0xBA18
#define     MV_M25Q128_MAX_SPI_FREQ              20000000     
#define     MV_M25Q128_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25Q128_FAST_READ_DUMMY_BYTES     1
#define     MV_M25Q128_ADDR_CYC_CNT             3

#define     MV_M25P32_SECTOR_SIZE               0x10000  
#define     MV_M25P64_SECTOR_SIZE               0x10000  
#define     MV_M25P128_SECTOR_SIZE              0x40000  
#define     MV_M25Q128_SECTOR_SIZE              0x10000  
#define     MV_M25P32_SECTOR_NUMBER             64
#define     MV_M25P64_SECTOR_NUMBER             128
#define     MV_M25P128_SECTOR_NUMBER            64
#define     MV_M25Q128_SECTOR_NUMBER            256
#define		MV_M25P_PAGE_SIZE				    0x100    
#define	    MV_M25Q_PAGE_SIZE			0x100    

#define		MV_M25P_WREN_CMND_OPCD			    0x06	 
#define		MV_M25P_WRDI_CMND_OPCD			    0x04	 
#define		MV_M25P_RDID_CMND_OPCD			    0x9F	 
#define		MV_M25P_RDSR_CMND_OPCD			    0x05	 
#define		MV_M25P_WRSR_CMND_OPCD			    0x01	 
#define		MV_M25P_READ_CMND_OPCD			    0x03	 
#define		MV_M25P_FAST_RD_CMND_OPCD		    0x0B	 
#define		MV_M25P_PP_CMND_OPCD			    0x02	 
#define		MV_M25P_SE_CMND_OPCD			    0xD8	 
#define		MV_M25P_BE_CMND_OPCD			    0xC7	 
#define		MV_M25P_RES_CMND_OPCD			    0xAB	 
#define		MV_M25P_ADDR_CYC_CNT			3

#define		MV_M25P_STATUS_REG_WP_MASK	        (0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_NONE              (0x00 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_64           (0x01 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_32           (0x02 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_16           (0x03 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_8            (0x04 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_4            (0x05 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_1_OF_2            (0x06 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_M25P_STATUS_BP_ALL               (0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)






#define     MV_MXIC_MANF_ID                     0xC2
#ifdef CONFIG_SYNO_ARMADA
#define     MV_MX25L8006E_DEVICE_ID              0x2014
#define     MV_MX25L8006E_MAX_SPI_FREQ           20000000     
#define     MV_MX25L8006E_MAX_FAST_SPI_FREQ      86000000     
#define     MV_MX25L8006E_FAST_READ_DUMMY_BYTES  1
#define     MV_MX25L8006E_ADDR_CYC_CNT           3
#endif
#define     MV_MX25L1605_DEVICE_ID              0x2015
#define     MV_MX25L1605_MAX_SPI_FREQ           20000000     
#define     MV_MX25L1605_MAX_FAST_SPI_FREQ      50000000     
#define     MV_MX25L1605_FAST_READ_DUMMY_BYTES  1
#define	MV_MX25L1605_ADDR_CYC_CNT	    3
#define     MV_MX25L3205_DEVICE_ID              0x2016
#define     MV_MX25L3205_MAX_SPI_FREQ           20000000     
#define     MV_MX25L3205_MAX_FAST_SPI_FREQ      50000000     
#define     MV_MX25L3205_FAST_READ_DUMMY_BYTES  1
#define	MV_MX25L3205_ADDR_CYC_CNT	    3
#define     MV_MX25L6405_DEVICE_ID              0x2017
#define     MV_MX25L6405_MAX_SPI_FREQ           20000000     
#define     MV_MX25L6405_MAX_FAST_SPI_FREQ      50000000     
#define     MV_MX25L6405_FAST_READ_DUMMY_BYTES  1
#define	MV_MX25L6405_ADDR_CYC_CNT	    3
#define	    MV_MX25L257_DEVICE_ID		0x2019
#define	    MV_MX25L257_MAX_SPI_FREQ      	20000000     
#define	    MV_MX25L257_MAX_FAST_SPI_FREQ       50000000     
#define	    MV_MX25L257_FAST_READ_DUMMY_BYTES   1
#define	    MV_MX25L257_ADDR_CYC_CNT	        4
#define     MV_MXIC_DP_EXIT_DELAY               30           


#define     MV_MX25L1605_SECTOR_SIZE            0x10000  
#define     MV_MX25L3205_SECTOR_SIZE            0x10000  
#define     MV_MX25L6405_SECTOR_SIZE            0x10000  
#define     MV_MX25L257_SECTOR_SIZE        	0x10000  
#ifdef CONFIG_SYNO_ARMADA
#define     MV_MX25L8006E_SECTOR_SIZE           0x1000  
#endif
#define     MV_MX25L1605_SECTOR_NUMBER          32
#define     MV_MX25L3205_SECTOR_NUMBER          64
#define     MV_MX25L6405_SECTOR_NUMBER          128
#ifdef CONFIG_SYNO_ARMADA
#define     MV_MX25L8006E_SECTOR_NUMBER         256
#endif
#define     MV_MX25L257_SECTOR_NUMBER 		512
#define     MV_MXIC_PAGE_SIZE			0x100    

#define		MV_MX25L_WREN_CMND_OPCD			    0x06	 
#define		MV_MX25L_WRDI_CMND_OPCD			    0x04	 
#define		MV_MX25L_RDID_CMND_OPCD			    0x9F	 
#define		MV_MX25L_RDSR_CMND_OPCD			    0x05	 
#define		MV_MX25L_WRSR_CMND_OPCD			    0x01	 
#define		MV_MX25L_READ_CMND_OPCD			    0x03	 
#define		MV_MX25L_FAST_RD_CMND_OPCD		    0x0B	 
#define		MV_MX25L_PP_CMND_OPCD			    0x02	 
#define		MV_MX25L_SE_CMND_OPCD			    0xD8	 
#define		MV_MX25L_BE_CMND_OPCD			    0xC7	 
#define         MV_MX25L_DP_CMND_OPCD                       0xB9         
#define		MV_MX25L_RES_CMND_OPCD			    0xAB	 

#define     MV_MX25L_STATUS_REG_WP_MASK	        (0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_NONE             (0x00 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_128         (0x01 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_64          (0x02 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_32          (0x03 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_16          (0x04 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_8           (0x05 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_4           (0x06 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_1_OF_2           (0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_MX25L_STATUS_BP_ALL              (0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)

#define     MV_SPANSION_MANF_ID                     	0x01
#define     MV_S25FL128_DEVICE_ID              		0x2018
#define     MV_S25FL128_MAX_SPI_FREQ           		33000000     
#define     MV_S25FL128_MAX_FAST_SPI_FREQ        	104000000     
#define     MV_S25FL128_FAST_READ_DUMMY_BYTES    	1
#define	    MV_S25FL128_ADDR_CYC_CNT	        	3

#define     MV_S25FL128_SECTOR_SIZE            			0x40000  
#define     MV_S25FL128_SECTOR_NUMBER          			64
#define	    MV_S25FL_PAGE_SIZE			        	0x100    

#define		MV_S25FL_WREN_CMND_OPCD			    0x06	 
#define		MV_S25FL_WRDI_CMND_OPCD			    0x04	 
#define		MV_S25FL_RDID_CMND_OPCD			    0x9F	 
#define		MV_S25FL_RDSR_CMND_OPCD			    0x05	 
#define		MV_S25FL_WRSR_CMND_OPCD			    0x01	 
#define		MV_S25FL_READ_CMND_OPCD			    0x03	 
#define		MV_S25FL_FAST_RD_CMND_OPCD		    0x0B	 
#define		MV_S25FL_PP_CMND_OPCD			    0x02	 
#define		MV_S25FL_SE_CMND_OPCD			    0xD8	 
#define		MV_S25FL_BE_CMND_OPCD			    0xC7	 
#define     	MV_S25FL_DP_CMND_OPCD               	    0xB9    	 
#define		MV_S25FL_RES_CMND_OPCD			    0xAB	 

#define		MV_S25FL_STATUS_REG_WP_MASK	        (0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_NONE             	(0x00 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_128         	(0x01 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_64          	(0x02 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_32          	(0x03 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_16          	(0x04 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_8           	(0x05 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_4           	(0x06 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_1_OF_2           	(0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     	MV_S25FL_STATUS_BP_ALL              	(0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)

#ifdef CONFIG_SYNO_ARMADA

#define		GD_GD25Q_WREN_CMND_OPCD				0x06
#define		GD_GD25Q_WRDI_CMND_OPCD				0x04
#define		GD_GD25Q_RDID_CMND_OPCD				0x9F
#define		GD_GD25Q_RDSR_CMND_OPCD				0x05
#define		GD_GD25Q_WRSR_CMND_OPCD				0x01
#define		GD_GD25Q_READ_CMND_OPCD				0x03
#define		GD_GD25Q_FAST_RD_CMND_OPCD			0x0B
#define		GD_GD25Q_PP_CMND_OPCD				0x02
#define		GD_GD25Q_SE_CMND_OPCD				0xD8
#define		GD_GD25Q_BE_CMND_OPCD				0xC7
#define		GD_GD25Q_RES_CMND_OPCD				0xAB
#define		GD_SFLASH_NO_SPECIFIC_OPCD			0x0

#define		GD_GD25Q_SECTOR_SIZE				0x10000  
#define		GD_GD25Q_SECTOR_NUMBER				0x80
#define		GD_GD25Q_PAGE_SIZE					0x100  
#define		GD_GD25Q_MANF_ID					0xC8
#define		GD_GD25Q64B_DEVICE_ID				0x4017
#define		GD_GD25Q_MAX_SPI_FREQ				50000000
#define		GD_GD25Q_MAX_FAST_SPI_FREQ			50000000
#define		GD_GD25Q_FAST_READ_DUMMY_BYTES		1
#define		GD_GD25Q_ADDR_CYC_CNT				3

#define		GD_GD25Q_STATUS_REG_WP_MASK          (0x1F << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_NONE              (0x00 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_256          (0x01 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_128          (0x02 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_64           (0x03 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_32           (0x04 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_16           (0x05 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_8            (0x06 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_4            (0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_1_OF_2            (0x08 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define		GD_GD25Q_STATUS_BP_ALL               (0x1F << MV_SFLASH_STATUS_REG_WP_OFFSET)

#endif
#ifdef __cplusplus
}
#endif

#endif  
