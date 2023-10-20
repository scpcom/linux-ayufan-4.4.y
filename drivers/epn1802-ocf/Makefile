obj-m += m86xxx_elp.o 
m86xxx_elp-objs := m86xxx.o m86xxx_elpecc.o m86xxx_elprsa.o

EXTRA_CFLAGS += -I$(src)/../../crypto/ocf

override EXTRA_CFLAGS += -DSDK_ENDIAN_LITTLE -DSPACC_3 -I ../../crypto/ocf

override EXTRA_CFLAGS += -DDO_ECC -DDO_CLUE_ALL -DELP_SHA2

