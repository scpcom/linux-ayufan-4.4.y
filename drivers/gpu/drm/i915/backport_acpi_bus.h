#pragma once

#include <acpi/acpi.h>

/* from include/acpi/acpi_bus.h */

bool acpi_check_dsm(acpi_handle handle, const u8 *uuid, int rev, u64 funcs);
union acpi_object *acpi_evaluate_dsm(acpi_handle handle, const u8 *uuid,
			int rev, int func, union acpi_object *argv4);

static inline union acpi_object *
acpi_evaluate_dsm_typed(acpi_handle handle, const u8 *uuid, int rev, int func,
			union acpi_object *argv4, acpi_object_type type)
{
	union acpi_object *obj;

	obj = acpi_evaluate_dsm(handle, uuid, rev, func, argv4);
	if (obj && obj->type != type) {
		ACPI_FREE(obj);
		obj = NULL;
	}

	return obj;
}
