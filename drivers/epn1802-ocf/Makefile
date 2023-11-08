obj-m += m86xxx_elp.o 
m86xxx_elp-objs := m86xxx.o m86xxx_elpecc.o m86xxx_elprsa.o

override EXTRA_CFLAGS += -DSDK_ENDIAN_LITTLE -DSPACC_3 -Icrypto/ocf

override EXTRA_CFLAGS += -DDO_ECC -DDO_CLUE_ALL

all:
	$(MAKE) -C $(KERNEL_PATH) SUBDIRS=$(shell pwd) modules
clean:
	$(MAKE) -C $(KERNEL_PATH) SUBDIRS=$(shell pwd) clean
	rm -f *.o *.ko
