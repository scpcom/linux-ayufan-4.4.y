#
#
# Note: This Makefile is for 2.6 kernel only, at present.
#
# V0.0.0.6 Ver.Make

# default to build for the running kernel
ifeq ("x", "x$(KERNEL_SRC)")
	KERNEL_SRC=/lib/modules/$(shell uname -r)/build
endif

# use KERNEL_SRC if not called by our driver disk maker
ifeq ("x", "x$(KERNEL_SOURCE_DIR)")
        KERNEL_SRC_DIR=$(KERNEL_SRC)
else
	KERNEL_SRC_DIR=$(KERNEL_SOURCE_DIR)
endif

ifeq ("x", "x$(KERNEL_BUILD_DIR)")
	KERNEL_BLD_DIR=$(KERNEL_SRC_DIR)
else
        KERNEL_BLD_DIR=$(KERNEL_BUILD_DIR)
endif

#ifneq ($(KERNELRELEASE),)
#	include $(SUBDIRS)/mv_conf.mk
#else
#	include mv_conf.mk
#endif

SUPPORT_THOR=y

CONFIG_64BIT=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_64BIT | awk -F= '{print $$2}')
CONFIG_REGPARM=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_REGPARM | awk -F= '{print $$2}')
CONFIG_SUSE_KERNEL=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_SUSE_KERNEL | awk -F= '{print $$2}')

ifeq ($(ARCH), )
ifeq ($(strip $(CONFIG_64BIT)),y)
	ARCH_TYPE=x86_64
else
	ARCH_TYPE=i386
endif
else
	ARCH_TYPE=$(ARCH)
endif

MV_CC     =  $(CROSS_COMPILE)$(CC)
MV_LD     =  $(CROSS_COMPILE)$(LD)

export LD_LIBRARY_PATH

odin_objs_$(SUPPORT_ODIN)   := core/odin/core_exp.o   core/odin/core_init.o   \
		               core/odin/core_ibcd.o  core/odin/core_adpt.o   \
		               core/odin/core_spi.o    \
		               core/odin/core_ses.o   core/odin/core_xor.o    \
		               core/odin/core_api.o   core/odin/scsi2sata.o   \
		               core/odin/core_i2c.o                           \
		               core/odin/ata2scsi.o   core/odin/core_helper.o \
			       core/odin/core_extern.o core/odin/core_swxor.o  \
#    core/odin/consolid.o \
#   core/odin/core_eeprom.o \

thor_objs_$(SUPPORT_THOR)   := core/thor/core_exp.o core/thor/core_init.o\
		   core/thor/core_api.o core/thor/core_xor.o\
		   core/thor/scsi2sata.o core/thor/core_swxor.o core/thor/core_sat.o

#cons_objs_$(SUPPORT_CONSOLIDATE) := core/thor/consolid.o

CORE_OBJS       :=  $(odin_objs_y) $(thor_objs_y)


COMM_OBJS       := lib/common/com_util.o lib/common/com_u64.o             \
		   lib/common/com_scsi.o lib/common/com_tag.o             \
		   lib/common/com_sgd.o  lib/common/com_nvram.o


OSDEP_OBJS      := linux/linux_main.o    linux/hba_exp.o          \
		   linux/hba_mod.o       linux/hba_timer.o        \
		   linux/oss_wrapper.o   linux/linux_iface.o      \
	           linux/res_mgmt.o

# for partial source tree building
core_dir_$(SUPPORT_ODIN) :=core/odin
core_dir_$(SUPPORT_THOR) :=core/thor
include_dir := include  include/generic  include/icommon

header_dirs     :=   $(include_dir)

LIBMV_OBJS	:=  $(COMM_OBJS) $(RAID_OBJS)

HBA_OBJS        := $(OSDEP_OBJS) $(CORE_OBJS) $(LIBMV_OBJS)

INCLUDE_DIR     = -I$(KERNEL_BLD_DIR)/include                          \
		  -I$(KERNEL_BLD_DIR)/include/scsi                     \
		  -I$(KERNEL_BLD_DIR)/drivers/scsi                     \
		  -I$(KERNEL_SRC_DIR)/include                          \
		  -I$(KERNEL_SRC_DIR)/include/scsi                     \
		  -I$(KERNEL_SRC_DIR)/drivers/scsi


ifneq ($(KERNELRELEASE),)
ifeq ($(SUPPORT_ODIN), y)
obj-m         :=   mv64xx.o
mv64xx-objs   :=   $(HBA_OBJS)
endif

ifeq ($(SUPPORT_THOR), y)
obj-m         :=   mv61xx.o
mv61xx-objs   :=   $(HBA_OBJS)
endif

clean-files   +=   Modules.symvers
clean-files   +=   Module.symvers
EXTRA_CFLAGS  :=   -I$(src)/include         \
		   -I$(src)/core            \
		   -I$(src)/.               \
		   -I$(src)/linux           \
                   -I$(src)/include/generic \
		   -I$(src)/include/icommon

EXTRA_CFLAGS  +=   -D__MV_LINUX__ $(INCLUDE_DIR)

ifeq ($(CONFIG_64BIT), y)
EXTRA_CFLAGS  +=   -D_64_SYS_
EXTRA_CFLAGS  +=   -D_64_BIT_COMPILER
else
EXTRA_CFLAGS  +=   -D_32_LEGACY_
endif
ifeq ($(CONFIG_4KSTACKS), y)
EXTRA_CFLAGS  +=   -D_4K_STACKS_
endif

ifeq ($(CONFIG_SUSE_KERNEL), y)
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=1
else
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=0
endif
EXTRA_CFLAGS  +=   -include $(src)/mv_config.h

ifeq ($(SUPPORT_ODIN), y)
EXTRA_CFLAGS  +=   -I$(src)/core/odin -DPRODUCTNAME_ODIN
endif

ifeq ($(SUPPORT_THOR), y)
EXTRA_CFLAGS  +=   -I$(src)/core/thor -DPRODUCTNAME_THOR
endif

EXTRA_CFLAGS  += #-D__MV_DEBUG__
EXTRA_CFLAGS  += -Wno-unused-variable  -Wno-pointer-arith
EXTRA_CFLAGS  += -D__LEGACY_OSSW__=1

else

# Why use SUBDIRS? for backward compatibility
all:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` modules

kbuild:
	-@cat patch.kbuild | patch -p1 -N -d $(KERNEL_SRC_DIR)/drivers/scsi
	@./patch.sh $(KERNEL_SRC_DIR)/drivers/scsi a
	@mkdir -p $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -r * $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -f Makefile.kbuild $(KERNEL_SRC_DIR)/drivers/scsi/mv/Makefile
	@echo done.

ukbuild:
	-@cat patch.kbuild | patch -p1 -N -R -d $(KERNEL_SRC_DIR)/drivers/scsi
	@./patch.sh $(KERNEL_SRC_DIR)/drivers/scsi
	@rm -rf $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@echo Patch has been successfully rewinded.

clean:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` clean
endif
