SECTIONS
{
	.class_dmem_sh : SUBALIGN(8) {
		__class_dmem_sh = .;
		*(SORT(.class_dmem_sh_*))
	}

	.class_pe_lmem_sh : SUBALIGN(8) {
		__class_pe_lmem_sh = .;
		*(SORT(.class_pe_lmem_sh_*))
	}

	.tmu_dmem_sh : SUBALIGN(8) {
		__tmu_dmem_sh = .;
		*(SORT(.tmu_dmem_sh_*))
	}

	.util_dmem_sh : SUBALIGN(8) {
		__util_dmem_sh = .;
		*(SORT(.util_dmem_sh_*))
	}
	
	.util_ddr_sh : SUBALIGN(8) {
		__util_ddr_sh = .;
		*(SORT(.util_ddr_sh_*))
	}
}
