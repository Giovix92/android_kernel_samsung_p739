/*
 * Automatically generated C config: don't edit
 * Linux kernel version: 2.6.36.3
 * Thu Dec 29 13:12:18 2011
 */
/*
 * Linker script for 32-bit vDSO.
 * We #include the file to define the layout details.
 * Here we only choose the prelinked virtual address.
 *
 * This file defines the version script giving the user-exported symbols in
 * the DSO.  We can define local symbols here called VDSO* to make their
 * values visible using the asm-x86/vdso.h macros from the kernel proper.
 */
/*
 * Linker script for vDSO.  This is an ELF shared object prelinked to
 * its virtual address, and with only one read-only segment.
 * This script controls its layout.
 */
SECTIONS
{
 . = 0 + SIZEOF_HEADERS;
 .hash : { *(.hash) } :text
 .gnu.hash : { *(.gnu.hash) }
 .dynsym : { *(.dynsym) }
 .dynstr : { *(.dynstr) }
 .gnu.version : { *(.gnu.version) }
 .gnu.version_d : { *(.gnu.version_d) }
 .gnu.version_r : { *(.gnu.version_r) }
 .note : { *(.note.*) } :text :note
 .eh_frame_hdr : { *(.eh_frame_hdr) } :text :eh_frame_hdr
 .eh_frame : { KEEP (*(.eh_frame)) } :text
 .dynamic : { *(.dynamic) } :text :dynamic
 .rodata : { *(.rodata*) } :text
 .data : {
       *(.data*)
       *(.sdata*)
       *(.got.plt) *(.got)
       *(.gnu.linkonce.d.*)
       *(.bss*)
       *(.dynbss*)
       *(.gnu.linkonce.b.*)
 }
 .altinstructions : { *(.altinstructions) }
 .altinstr_replacement : { *(.altinstr_replacement) }
 /*
	 * Align the actual code well away from the non-instruction data.
	 * This is the best thing for the I-cache.
	 */
 . = ALIGN(0x100);
 .text : { *(.text*) } :text =0x90909090
}
/*
 * Very old versions of ld do not recognize this name token; use the constant.
 */
/*
 * We must supply the ELF program headers explicitly to get just one
 * PT_LOAD segment, and set the flags explicitly to make segments read-only.
 */
PHDRS
{
 text PT_LOAD FLAGS(5) FILEHDR PHDRS; /* PF_R|PF_X */
 dynamic PT_DYNAMIC FLAGS(4); /* PF_R */
 note PT_NOTE FLAGS(4); /* PF_R */
 eh_frame_hdr 0x6474e550;
}
/* The ELF entry point can be used to set the AT_SYSINFO value.  */
ENTRY(__kernel_vsyscall);
/*
 * This controls what userland symbols we export from the vDSO.
 */
VERSION
{
 LINUX_2.5 {
 global:
  __kernel_vsyscall;
  __kernel_sigreturn;
  __kernel_rt_sigreturn;
 local: *;
 };
}
/*
 * Symbols we define here called VDSO* get their values into vdso32-syms.h.
 */
VDSO32_PRELINK = 0;
VDSO32_vsyscall = __kernel_vsyscall;
VDSO32_sigreturn = __kernel_sigreturn;
VDSO32_rt_sigreturn = __kernel_rt_sigreturn;
