#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kvm_host.h>
#include <asm/kvm_mmio.h>
#include <asm/kvm_emulate.h>
#include <trace/events/kvm.h>

#include "trace.h"

int kvm_handle_mmio_return(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	unsigned long *dest;
	unsigned int len;
	int mask;

	if (!run->mmio.is_write) {
		dest = vcpu_reg(vcpu, vcpu->arch.mmio_decode.rt);
		*dest = 0;

		len = run->mmio.len;
		if (len > sizeof(unsigned long))
			return -EINVAL;

		memcpy(dest, run->mmio.data, len);

		trace_kvm_mmio(KVM_TRACE_MMIO_READ, len, run->mmio.phys_addr,
				*((u64 *)run->mmio.data));

		if (vcpu->arch.mmio_decode.sign_extend &&
		    len < sizeof(unsigned long)) {
			mask = 1U << ((len * 8) - 1);
			*dest = (*dest ^ mask) - mask;
		}
	}

	return 0;
}

static int decode_hsr(struct kvm_vcpu *vcpu, phys_addr_t fault_ipa,
		      struct kvm_exit_mmio *mmio)
{
	unsigned long rt, len;
	bool is_write, sign_extend;

	if (kvm_vcpu_dabt_isextabt(vcpu)) {
		 
		kvm_inject_dabt(vcpu, kvm_vcpu_get_hfar(vcpu));
		return 1;
	}

	if (kvm_vcpu_dabt_iss1tw(vcpu)) {
		 
		kvm_inject_dabt(vcpu, kvm_vcpu_get_hfar(vcpu));
		return 1;
	}

	len = kvm_vcpu_dabt_get_as(vcpu);
	if (unlikely(len < 0))
		return len;

	is_write = kvm_vcpu_dabt_iswrite(vcpu);
	sign_extend = kvm_vcpu_dabt_issext(vcpu);
	rt = kvm_vcpu_dabt_get_rd(vcpu);

	if (kvm_vcpu_reg_is_pc(vcpu, rt)) {
		 
		kvm_inject_pabt(vcpu, kvm_vcpu_get_hfar(vcpu));
		return 1;
	}

	mmio->is_write = is_write;
	mmio->phys_addr = fault_ipa;
	mmio->len = len;
	vcpu->arch.mmio_decode.sign_extend = sign_extend;
	vcpu->arch.mmio_decode.rt = rt;

	kvm_skip_instr(vcpu, kvm_vcpu_trap_il_is32bit(vcpu));
	return 0;
}

#if defined(MY_DEF_HERE)
 
static int kvm_arm_mmio_read_write(struct kvm_vcpu *vcpu, struct kvm_run *run,
		 struct kvm_exit_mmio *mmio)
{
	int ret;
	gpa_t addr = mmio->phys_addr;
	int len = mmio->len;
	void *v = mmio->data;

	mutex_lock(&vcpu->kvm->slots_lock);
	if (mmio->is_write)
		ret = kvm_io_bus_write(vcpu->kvm, KVM_MMIO_BUS, addr, len, v);
	else
		ret = kvm_io_bus_read(vcpu->kvm, KVM_MMIO_BUS, addr, len, v);
	mutex_unlock(&vcpu->kvm->slots_lock);

	if (ret == 0) {
		kvm_prepare_mmio(run, mmio);
		kvm_handle_mmio_return(vcpu, run);
	}

	return !ret;
}
#endif  

int io_mem_abort(struct kvm_vcpu *vcpu, struct kvm_run *run,
		 phys_addr_t fault_ipa)
{
	struct kvm_exit_mmio mmio;
	unsigned long rt;
	int ret;

	if (kvm_vcpu_dabt_isvalid(vcpu)) {
		ret = decode_hsr(vcpu, fault_ipa, &mmio);
		if (ret)
			return ret;
	} else {
		kvm_err("load/store instruction decoding not implemented\n");
		return -ENOSYS;
	}

	rt = vcpu->arch.mmio_decode.rt;
	trace_kvm_mmio((mmio.is_write) ? KVM_TRACE_MMIO_WRITE :
					 KVM_TRACE_MMIO_READ_UNSATISFIED,
			mmio.len, fault_ipa,
			(mmio.is_write) ? *vcpu_reg(vcpu, rt) : 0);

	if (mmio.is_write)
		memcpy(mmio.data, vcpu_reg(vcpu, rt), mmio.len);

	if (vgic_handle_mmio(vcpu, run, &mmio))
		return 1;

#if defined(MY_DEF_HERE)
	if (kvm_arm_mmio_read_write(vcpu, run, &mmio))
		return 1;
#endif  

	kvm_prepare_mmio(run, &mmio);
	return 0;
}
