
obj-$(CONFIG_FSL_DPAA2_ETHSW) += ethsw.o

ethsw-objs := switch.o dpsw.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
