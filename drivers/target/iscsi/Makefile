iscsi_target_mod-y +=		iscsi_target_parameters.o \
				iscsi_target_seq_pdu_list.o \
				iscsi_target_tq.o \
				iscsi_target_auth.o \
				iscsi_target_datain_values.o \
				iscsi_target_device.o \
				iscsi_target_erl0.o \
				iscsi_target_erl1.o \
				iscsi_target_erl2.o \
				iscsi_target_login.o \
				iscsi_target_nego.o \
				iscsi_target_nodeattrib.o \
				iscsi_target_tmr.o \
				iscsi_target_tpg.o \
				iscsi_target_util.o \
				iscsi_target.o \
				iscsi_target_configfs.o \
				iscsi_target_stat.o \
				iscsi_target_log.o

obj-$(CONFIG_ISCSI_TARGET)	+= iscsi_target_mod.o

#20121130, adam hsu support iscsi zero-copy
ccflags-y += -DSUPPORT_ISCSI_ZERO_COPY -DCONFIG_MACH_QNAPTS
#20121130, adam hsu support iscsi zero-copy