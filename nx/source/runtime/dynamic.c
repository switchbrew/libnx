#include "result.h"
#include "kernel/svc.h"
#include "runtime/diag.h"
#include <elf.h>
#include <string.h>

typedef struct Mod0Header {
	u32 magic_mod0;
	s32 dyn_offset;
	s32 bss_start_offset;
	s32 bss_end_offset;
	s32 eh_frame_hdr_start_offset;
	s32 eh_frame_hdr_end_offset;
	s32 unused;

	u32 magic_lny0;
	s32 got_start_offset;
	s32 got_end_offset;

	u32 magic_lny1;
	s32 relro_start_offset;
	s32 relro_end_offset;
} Mod0Header;

NX_INLINE void* _dynResolveOffset(const Mod0Header* mod0, s32 offset)
{
	return (void*)((uintptr_t)mod0 + offset);
}

static void _dynProcessRela(uintptr_t base, const Elf64_Rela* rela, size_t relasz)
{
	for (; relasz--; rela++) {
		switch (ELF64_R_TYPE(rela->r_info)) {
			default: {
				diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadReloc));
				break;
			}

			case R_AARCH64_RELATIVE: {
				u64* ptr = (u64*)(base + rela->r_offset);
				*ptr = base + rela->r_addend;
				break;
			}
		}
	}
}

void __nx_dynamic(uintptr_t base, const Mod0Header* mod0)
{
	// Return early if MOD0 header has been invalidated
	if (mod0->magic_mod0 != 0x30444f4d) { // MOD0
		return;
	}

	// Clear the BSS area
	u8* bss_start = _dynResolveOffset(mod0, mod0->bss_start_offset);
	u8* bss_end = _dynResolveOffset(mod0, mod0->bss_end_offset);
	if (bss_start != bss_end) {
		memset(bss_start, 0, bss_end - bss_start);
	}

	// Retrieve pointer to the ELF dynamic section
	const Elf64_Dyn* dyn = _dynResolveOffset(mod0, mod0->dyn_offset);

	// Extract relevant information from the ELF dynamic section
	const Elf64_Rela* rela = NULL;
	size_t relasz = 0;
	for (; dyn->d_tag != DT_NULL; dyn++) {
		switch (dyn->d_tag) {
			case DT_RELA:
				rela = (const Elf64_Rela*)(base + dyn->d_un.d_ptr);
				break;

			case DT_RELASZ:
				relasz = dyn->d_un.d_val / sizeof(Elf64_Rela);
				break;
		}
	}

	// Apply RELA relocations if present
	if (rela && relasz) {
		_dynProcessRela(base, rela, relasz);
	}

	// Return early if LNY0/LNY1 extensions are not present
	if (mod0->magic_lny0 != 0x30594e4c || mod0->magic_lny1 != 0x31594e4c) { // LNY0, LNY1
		return;
	}

	// Reprotect relro segment as read-only now that we're done processing relocations
	u8* relro_start = _dynResolveOffset(mod0, mod0->relro_start_offset);
	size_t relro_sz = (u8*)_dynResolveOffset(mod0, mod0->relro_end_offset) - relro_start;
	Result rc = svcSetMemoryPermission(relro_start, relro_sz, Perm_R);
	if (R_FAILED(rc)) {
		diagAbortWithResult(rc);
	}

	// Lock the relro segment's permissions
	svcSetMemoryAttribute(relro_start, relro_sz, MemAttr_IsPermissionLocked, MemAttr_IsPermissionLocked);
}
