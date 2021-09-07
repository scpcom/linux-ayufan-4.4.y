#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvSFlashSpecH
#define __INCmvSFlashSpecH

#define		MV_SFLASH_READ_CMND_LENGTH		    4		 
#define		MV_SFLASH_SE_CMND_LENGTH		    4		 
#define		MV_SFLASH_BE_CMND_LENGTH		    1		 
#define		MV_SFLASH_PP_CMND_LENGTH		    4		 
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

#define     MV_M25PXXX_ST_MANF_ID               0x20
#define     MV_M25P32_DEVICE_ID                 0x2016
#define     MV_M25P32_MAX_SPI_FREQ              20000000     
#define     MV_M25P32_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25P32_FAST_READ_DUMMY_BYTES     1
#define     MV_M25P64_DEVICE_ID                 0x2017
#define     MV_M25P64_MAX_SPI_FREQ              20000000     
#define     MV_M25P64_MAX_FAST_SPI_FREQ         50000000     
#define     MV_M25P64_FAST_READ_DUMMY_BYTES     1
#define     MV_M25P128_DEVICE_ID                0x2018
#define     MV_M25P128_MAX_SPI_FREQ             20000000     
#define     MV_M25P128_MAX_FAST_SPI_FREQ        50000000     
#define     MV_M25P128_FAST_READ_DUMMY_BYTES    1

#define     MV_M25P32_SECTOR_SIZE               0x10000  
#define     MV_M25P64_SECTOR_SIZE               0x10000  
#define     MV_M25P128_SECTOR_SIZE              0x40000  
#define     MV_M25P32_SECTOR_NUMBER             64
#define     MV_M25P64_SECTOR_NUMBER             128
#define     MV_M25P128_SECTOR_NUMBER            64
#define		MV_M25P_PAGE_SIZE				    0x100    

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
#define     MV_MX25L6405_DEVICE_ID              0x2017
#define     MV_MX25L6405_MAX_SPI_FREQ           20000000     
#define     MV_MX25L6405_MAX_FAST_SPI_FREQ      50000000     
#define     MV_MX25L6405_FAST_READ_DUMMY_BYTES  1
#define     MV_MXIC_DP_EXIT_DELAY               30           

#define     MV_MX25L6405_SECTOR_SIZE            0x10000  
#define     MV_MX25L6405_SECTOR_NUMBER          128
#define		MV_MXIC_PAGE_SIZE			        0x100    

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
#define     MV_MX25L_DP_CMND_OPCD               0xB9     
#define		MV_MX25L_RES_CMND_OPCD			    0xAB	 

#define		MV_MX25L_STATUS_REG_WP_MASK	        (0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)
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

#define     MV_S25FL128_SECTOR_SIZE            			0x40000  
#define     MV_S25FL128_SECTOR_NUMBER          			64
#define	    MV_S25FL_PAGE_SIZE			        	0x100    
#ifdef MY_ABC_HERE
#define     MV_S25FL032A_DEVICE_ID					0x0215
#define     MV_S25FL032A_SECTOR_SIZE				0x10000  
#define     MV_S25FL032A_SECTOR_NUMBER				64
#define     MV_S25FL032A_MAX_SPI_FREQ				20000000  
#define     MV_S25FL032A_MAX_FAST_SPI_FREQ			50000000  
#define     MV_S25FL032A_FAST_READ_DUMMY_BYTES		1

#define     MV_MX25L3206_SECTOR_SIZE            0x10000  
#define     MV_MX25L3206_SECTOR_NUMBER          64
#define     MV_MX25L3206_DEVICE_ID              0x2016
#define     MV_MX25L3206_MAX_SPI_FREQ           20000000  
#define     MV_MX25L3206_MAX_FAST_SPI_FREQ      50000000  
#define     MV_MX25L3206_FAST_READ_DUMMY_BYTES  1

#define		MV_N25Q_WREN_CMND_OPCD				0x06     
#define		MV_N25Q_WRDI_CMND_OPCD				0x04     
#define		MV_N25Q_RDID_CMND_OPCD				0x9F     
#define		MV_N25Q_RDSR_CMND_OPCD				0x05     
#define		MV_N25Q_WRSR_CMND_OPCD				0x01     
#define		MV_N25Q_READ_CMND_OPCD				0x03     
#define		MV_N25Q_FAST_RD_CMND_OPCD			0x0B     
#define		MV_N25Q_PP_CMND_OPCD				0x02     
#define		MV_N25Q_SE_CMND_OPCD				0xD8     
#define		MV_N25Q_BE_CMND_OPCD				0xC7     
#define		MV_N25Q032_SECTOR_SIZE				0x10000  
#define		MV_N25Q032_SECTOR_NUMBER			64
#define		MV_N25Q064_SECTOR_SIZE				0x10000  
#define		MV_N25Q064_SECTOR_NUMBER			128
#define		MV_N25Q_PAGE_SIZE					0x100    
#define		MV_N25QXXX_ST_MANF_ID				0x20
#define		MV_N25Q032_DEVICE_ID				0xBA16
#define		MV_N25Q032_MAX_SPI_FREQ				50000000
#define		MV_N25Q032_MAX_FAST_SPI_FREQ		100000000
#define		MV_N25Q032_FAST_READ_DUMMY_BYTES	1
#define		MV_N25Q064_DEVICE_ID				0xBA17
#define		MV_N25Q064_MAX_SPI_FREQ				54000000
#define		MV_N25Q064_MAX_FAST_SPI_FREQ		108000000
#define		MV_N25Q064_FAST_READ_DUMMY_BYTES	1

#define     MV_S25FL064_DEVICE_ID              		0x0216
#define     MV_S25FL064_MAX_SPI_FREQ           		40000000     
#define     MV_S25FL064_MAX_FAST_SPI_FREQ        	104000000     
#define     MV_S25FL064_FAST_READ_DUMMY_BYTES    	1

#define     MV_S25FL064_SECTOR_SIZE            			0x10000  
#define     MV_S25FL064_SECTOR_NUMBER          			128
#endif  

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

#ifdef MY_ABC_HERE
 
#define     MV_SST_MANF_ID                     0xBF
#define     MV_SST25VF032_DEVICE_ID              0x254A
#define     MV_SST25VF032_MAX_SPI_FREQ           25000000     
#define     MV_SST25VF032_MAX_FAST_SPI_FREQ      80000000     
#define     MV_SST25VF032_FAST_READ_DUMMY_BYTES  1

#define     MV_SST25VF032_SECTOR_SIZE            0x10000  
#define     MV_SST25VF032_SECTOR_NUMBER          64
#define                MV_SST_PAGE_SIZE                                0x1    

#define                MV_SST25VF_WREN_CMND_OPCD                           0x06         
#define                MV_SST25VF_WRDI_CMND_OPCD                           0x04         
#define                MV_SST25VF_RDID_CMND_OPCD                           0x9F         
#define                MV_SST25VF_RDSR_CMND_OPCD                           0x05         
#define                MV_SST25VF_WRSR_CMND_OPCD                           0x01         
#define                MV_SST25VF_READ_CMND_OPCD                           0x03         
#define                MV_SST25VF_FAST_RD_CMND_OPCD                0x0B         
#define                MV_SST25VF_PP_CMND_OPCD                     0x02         
#define                MV_SST25VF_SE_CMND_OPCD                     0xD8         
#define                MV_SST25VF_BE_CMND_OPCD                     0xC7         
#define                MV_SST25VF_RES_CMND_OPCD                            0xAB         

#define                MV_SST25VF_STATUS_REG_WP_MASK           (0x0F << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_NONE             (0x00 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_64          (0x01 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_32          (0x02 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_16          (0x03 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_8           (0x04 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_4           (0x05 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_1_OF_2           (0x06 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#define     MV_SST25VF_STATUS_BP_ALL              (0x07 << MV_SFLASH_STATUS_REG_WP_OFFSET)
#endif

#endif  
