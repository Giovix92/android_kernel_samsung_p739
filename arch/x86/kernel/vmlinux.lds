/*
 * Automatically generated C config: don't edit
 * Linux kernel version: 2.6.36.3
 * Thu Dec 29 13:12:18 2011
 */
/*
 * ld script for the x86 kernel
 *
 * Historic 32-bit version written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 *
 * Modernisation, unification and other changes and fixes:
 *   Copyright (C) 2007-2009  Sam Ravnborg <sam@ravnborg.org>
 *
 *
 * Don't define absolute symbols until and unless you know that symbol
 * value is should remain constant even if kernel image is relocated
 * at run time. Absolute symbols are not relocated. If symbol value should
 * change if kernel is relocated, make the symbol section relative and
 * put it inside the section definition.
 */
/*
 * Helper macros to support writing architecture specific
 * linker scripts.
 *
 * A minimal linker scripts has following content:
 * [This is a sample, architectures may have special requiriements]
 *
 * OUTPUT_FORMAT(...)
 * OUTPUT_ARCH(...)
 * ENTRY(...)
 * SECTIONS
 * {
 *	. = START;
 *	__init_begin = .;
 *	HEAD_TEXT_SECTION
 *	INIT_TEXT_SECTION(PAGE_SIZE)
 *	INIT_DATA_SECTION(...)
 *	PERCPU(PAGE_SIZE)
 *	__init_end = .;
 *
 *	_stext = .;
 *	TEXT_SECTION = 0
 *	_etext = .;
 *
 *      _sdata = .;
 *	RO_DATA_SECTION(PAGE_SIZE)
 *	RW_DATA_SECTION(...)
 *	_edata = .;
 *
 *	EXCEPTION_TABLE(...)
 *	NOTES
 *
 *	BSS_SECTION(0, 0, 0)
 *	_end = .;
 *
 *	STABS_DEBUG
 *	DWARF_DEBUG
 *
 *	DISCARDS		// must be the last
 * }
 *
 * [__init_begin, __init_end] is the init section that may be freed after init
 * [_stext, _etext] is the text section
 * [_sdata, _edata] is the data section
 *
 * Some of the included output section have their own set of constants.
 * Examples are: [__initramfs_start, __initramfs_end] for initramfs and
 *               [__nosave_begin, __nosave_end] for the nosave data
 */
/* Align . to a 8 byte boundary equals to maximum function alignment. */
/*
 * Align to a 32 byte boundary equal to the
 * alignment gcc 4.5 uses for a struct
 */
/* The actual configuration determine if the init/exit sections
 * are handled as text/data or they can be discarded (which
 * often happens at runtime)
 */
/* .data section */
/*
 * Data section helpers
 */
/*
 * Read only Data
 */
/* RODATA & RO_DATA provided for backward compatibility.
 * All archs are supposed to use RO_DATA() */
/* .text section. Map to function alignment to avoid address changes
 * during second ld run in second ld pass when generating System.map */
/* sched.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
/* spinlock.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
/* Section used for early init (in .S files) */
/*
 * Exception table
 */
/*
 * Init task
 */
/* init and exit section handling */
/*
 * bss (Block Started by Symbol) - uninitialized data
 * zeroed during startup
 */
/*
 * DWARF debug sections.
 * Symbols in the DWARF debugging sections are relative to
 * the beginning of the section so we begin them at 0.
 */
  /* Stabs debugging sections.  */
/*
 * Default discarded sections.
 *
 * Some archs want to discard exit text/data at runtime rather than
 * link time due to cross-section references such as alt instructions,
 * bug table, eh_frame, etc.  DISCARDS must be the last of output
 * section definitions so that such archs put those in earlier section
 * definitions.
 */
/**
 * PERCPU_VADDR - define output section for percpu area
 * @vaddr: explicit base address (optional)
 * @phdr: destination PHDR (optional)
 *
 * Macro which expands to output section for percpu area.  If @vaddr
 * is not blank, it specifies explicit base address and all percpu
 * symbols will be offset from the given address.  If blank, @vaddr
 * always equals @laddr + LOAD_OFFSET.
 *
 * @phdr defines the output PHDR to use if not blank.  Be warned that
 * output PHDR is sticky.  If @phdr is specified, the next output
 * section in the linker script will go there too.  @phdr should have
 * a leading colon.
 *
 * Note that this macros defines __per_cpu_load as an absolute symbol.
 * If there is no need to put the percpu section at a predetermined
 * address, use PERCPU().
 */
/**
 * PERCPU - define output section for percpu area, simple version
 * @align: required alignment
 *
 * Align to @align and outputs output section for percpu area.  This
 * macro doesn't maniuplate @vaddr or @phdr and __per_cpu_load and
 * __per_cpu_start will be identical.
 *
 * This macro is equivalent to ALIGN(align); PERCPU_VADDR( , ) except
 * that __per_cpu_load is defined as a relative symbol against
 * .data..percpu which is required for relocatable x86_32
 * configuration.
 */
/*
 * Definition of the high level *_SECTION macros
 * They will fit only a subset of the architectures
 */
/*
 * Writeable data.
 * All sections are combined in a single .data section.
 * The sections following CONSTRUCTORS are arranged so their
 * typical alignment matches.
 * A cacheline is typical/always less than a PAGE_SIZE so
 * the sections that has this restriction (or similar)
 * is located before the ones requiring PAGE_SIZE alignment.
 * NOSAVE_DATA starts and ends with a PAGE_SIZE alignment which
 * matches the requirment of PAGE_ALIGNED_DATA.
 *
 * use 0 as page_align if page_aligned data is not used */
/*
 * DO NOT MODIFY.
 *
 * This file was generated by Kbuild
 *
 */
/* thread_info.h: low-level thread information
 *
 * Copyright (C) 2002  David Howells (dhowells@redhat.com)
 * - Incorporating suggestions made by Linus Torvalds and Dave Miller
 */
/*
 * Allow us to mark functions as 'deprecated' and have gcc emit a nice
 * warning for each use, in hopes of speeding the functions removal.
 * Usage is:
 * 		int __deprecated foo(void)
 */
/*
 * Allow us to avoid 'defined but not used' warnings on functions and data,
 * as well as force them to be emitted to the assembly file.
 *
 * As of gcc 3.4, static functions that are not marked with attribute((used))
 * may be elided from the assembly file.  As of gcc 3.4, static data not so
 * marked will not be elided, but this may change in a future gcc version.
 *
 * NOTE: Because distributions shipped with a backported unit-at-a-time
 * compiler in gcc 3.3, we must define __used to be __attribute__((used))
 * for gcc >=3.3 instead of 3.4.
 *
 * In prior versions of gcc, such functions and data would be emitted, but
 * would be warned about except with attribute((unused)).
 *
 * Mark functions that are referenced only in inline assembly as __used so
 * the code is emitted even though it appears to be unreferenced.
 */
/*
 * Rather then using noinline to prevent stack consumption, use
 * noinline_for_stack instead.  For documentaiton reasons.
 */
/*
 * From the GCC manual:
 *
 * Many functions do not examine any values except their arguments,
 * and have no effects except the return value.  Basically this is
 * just slightly more strict class than the `pure' attribute above,
 * since function is not allowed to read global memory.
 *
 * Note that a function that has pointer arguments and examines the
 * data pointed to must _not_ be declared `const'.  Likewise, a
 * function that calls a non-`const' function usually must not be
 * `const'.  It does not make sense for a `const' function to return
 * `void'.
 */
/*
 * Tell gcc if a function is cold. The compiler will assume any path
 * directly leading to the call is unlikely.
 */
/* Simple shorthand for a section definition */
/* Are two types/vars the same type (ignoring qualifiers)? */
/* Compile time object size, -1 for unknown */
/*
 * Prevent the compiler from merging or refetching accesses.  The compiler
 * is also forbidden from reordering successive instances of ACCESS_ONCE(),
 * but only when the compiler is aware of some particular ordering.  One way
 * to make the compiler aware of ordering is to put the two invocations of
 * ACCESS_ONCE() in different C statements.
 *
 * This macro does absolutely -nothing- to prevent the CPU from reordering,
 * merging, or refetching absolutely anything at any time.  Its main intended
 * use is to mediate communication between process-level code and irq/NMI
 * handlers, all running on the same CPU.
 */
/*
 * int-ll64 is used practically everywhere now,
 * so use it as a reasonable default.
 */
/*
 * asm-generic/int-ll64.h
 *
 * Integer declarations for architectures which use "long long"
 * for 64-bit types.
 */
/*
 * There seems to be no way of detecting this automatically from user
 * space, so 64 bit architectures should override this in their
 * bitsperlong.h. In particular, an architecture that supports
 * both 32 and 64 bit user space must not rely on CONFIG_64BIT
 * to decide it, but rather check a compiler provided macro.
 */
/*
 * FIXME: The check currently breaks x86-64 build, so it's
 * temporarily disabled. Please fix x86-64 and reenable
 */
/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
/* const.h: Macros for dealing with constants.  */
/* Some constant macros are used in both assembler and
 * C code.  Therefore we cannot annotate them always with
 * 'UL' and other type specifiers unilaterally.  We
 * use the following macros to deal with this.
 *
 * Similarly, _AT() will cast an expression with a type in C, but
 * leave it unchanged in asm.
 */
/* PAGE_SHIFT determines the page size */
/* Cast PAGE_MASK to a signed type so that it is sign-extended if
   virtual addresses are 32-bits but physical addresses are larger
   (ie, 32-bit PAE). */
/*
 * Set __PAGE_OFFSET to the most negative possible address +
 * PGDIR_SIZE*16 (pgd slot 272).  The gap is to allow a space for a
 * hypervisor to fit.  Choosing 16 slots here is arbitrary, but it's
 * what Xen requires.
 */
/* See Documentation/x86/x86_64/mm.txt for a description of the memory map. */
/*
 * Kernel image size is limited to 512 MB (see level2_kernel_pgt in
 * arch/x86/kernel/head_64.S), and it is mapped here:
 */
/*
 * low level task data that entry.S needs immediate access to
 * - this struct should fit entirely inside of one cache line
 * - this struct shares the supervisor stack pages
 */
/*
 * thread information flags
 * - these are process state flags that various assembly files
 *   may need to access
 * - pending work-to-be-done flags are in LSW
 * - other flags in MSW
 * Warning: layout of LSW is hardcoded in entry.S
 */
/* work to do in syscall_trace_enter() */
/* work to do in syscall_trace_leave() */
/* work to do on interrupt/exception return */
/* work to do on any return to user space */
/* Only used for 64 bit */
/* flags to check in __switch_to() */
/* thread information allocation */
/*
 * PER_CPU finds an address of a per-cpu variable.
 *
 * Args:
 *    var - variable name
 *    reg - 32bit register
 *
 * The resulting address is stored in the "reg" argument.
 *
 * Example:
 *    PER_CPU(cpu_gdt_descr, %ebx)
 */
/*
 * Define the "EARLY_PER_CPU" macros.  These are used for some per_cpu
 * variables that are initialized and accessed before there are per_cpu
 * areas allocated.
 */
/*
 * macros/functions for gaining access to the thread information structure
 * preempt_count needs to be 1 initially, until the scheduler is functional.
 */
/* how to get the thread information struct from ASM */
/*
 * Thread-synchronous status.
 *
 * This is different from the flags in that nobody else
 * ever touches our thread-synchronous status, so we don't
 * have to worry about atomic accesses.
 */
/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
/*
 * For assembly routines.
 *
 * Note when using these that you must specify the appropriate
 * alignment directives yourself
 */
/*
 * This is used by architectures to keep arguments on the stack
 * untouched by the compiler by keeping them live until the end.
 * The argument stack may be owned by the assembly-language
 * caller, not the callee, and gcc doesn't always understand
 * that.
 *
 * We have the return value, and a maximum of six arguments.
 *
 * This should always be followed by a "return ret" for the
 * protection to work (ie no more work that the compiler might
 * end up needing stack temporaries for).
 */
/* Assembly files may be compiled with -traditional .. */
/* If symbol 'name' is treated as a subroutine (gets called, and returns)
 * then please use ENDPROC to mark 'name' as STT_FUNC for the benefit of
 * static analysis tools such as stack depth analyzer.
 */
/* L1 cache line size */
/* Internal svga startup constants */
/* const.h: Macros for dealing with constants.  */
/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
/* - set: nonlinear file mapping, saved PTE; unset:swap */
/* Set of bits not changed in pte_modify */
/*         xwr */
/*
 * early identity mapping  pte attrib macros.
 */
/*
 * PGDIR_SHIFT determines what a top-level page table entry can map
 */
/*
 * 3rd level page
 */
/*
 * PMD_SHIFT determines the size of the area a middle-level
 * page table can map
 */
/*
 * entries per page directory level
 */
/* See Documentation/x86/x86_64/mm.txt for a description of the memory map. */
/* Physical address where kernel should be loaded. */
/* Minimum kernel alignment, as a power of two */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(phys_startup_64)
jiffies_64 = jiffies;
/*
 * On 64-bit, align RODATA to 2MB so that even with CONFIG_DEBUG_RODATA
 * we retain large page mappings for boundaries spanning kernel text, rodata
 * and data sections.
 *
 * However, kernel identity mappings will have different RWX permissions
 * to the pages mapping to text and to the pages padding (which are freed) the
 * text section. Hence kernel identity mappings will be broken to smaller
 * pages. For 64-bit, kernel text and kernel identity mappings are different,
 * so we can enable protection checks that come with CONFIG_DEBUG_RODATA,
 * as well as retain 2MB large page mappings for kernel text.
 */
PHDRS {
 text PT_LOAD FLAGS(5); /* R_E */
 data PT_LOAD FLAGS(7); /* RWE */
 user PT_LOAD FLAGS(5); /* R_E */
 percpu PT_LOAD FLAGS(6); /* RW_ */
 init PT_LOAD FLAGS(7); /* RWE */
 note PT_NOTE FLAGS(0); /* ___ */
}
SECTIONS
{
        . = (0xffffffff80000000 + ((0x1000000 + (0x1000000 - 1)) & ~(0x1000000 - 1)));
        phys_startup_64 = startup_64 - 0xffffffff80000000;
 /* Text and read-only data */
 .text : AT(ADDR(.text) - 0xffffffff80000000) {
  _text = .;
  /* bootstrapping code */
  *(.head.text)
  . = ALIGN(8);
  _stext = .;
  . = ALIGN(8); *(.text.hot) *(.text) *(.ref.text) *(.devinit.text) *(.devexit.text) *(.cpuinit.text) *(.cpuexit.text) *(.text.unlikely)
  . = ALIGN(8); __sched_text_start = .; *(.sched.text) __sched_text_end = .;
  . = ALIGN(8); __lock_text_start = .; *(.spinlock.text) __lock_text_end = .;
  . = ALIGN(8); __kprobes_text_start = .; *(.kprobes.text) __kprobes_text_end = .;
 
  *(.fixup)
  *(.gnu.warning)
  /* End of text section */
  _etext = .;
 } :text = 0x9090
 .notes : AT(ADDR(.notes) - 0xffffffff80000000) { __start_notes = .; *(.note.*) __stop_notes = .; } :text :note
 . = ALIGN(16); __ex_table : AT(ADDR(__ex_table) - 0xffffffff80000000) { __start___ex_table = .; *(__ex_table) __stop___ex_table = .; } :text = 0x9090
 . = ALIGN((1 << 21));
 . = ALIGN(((1 << 12))); .rodata : AT(ADDR(.rodata) - 0xffffffff80000000) { __start_rodata = .; *(.rodata) *(.rodata.*) *(__vermagic) *(__markers_strings) *(__tracepoints_strings) } .rodata1 : AT(ADDR(.rodata1) - 0xffffffff80000000) { *(.rodata1) } . = ALIGN(8); __bug_table : AT(ADDR(__bug_table) - 0xffffffff80000000) { __start___bug_table = .; *(__bug_table) __stop___bug_table = .; } .pci_fixup : AT(ADDR(.pci_fixup) - 0xffffffff80000000) { __start_pci_fixups_early = .; *(.pci_fixup_early) __end_pci_fixups_early = .; __start_pci_fixups_header = .; *(.pci_fixup_header) __end_pci_fixups_header = .; __start_pci_fixups_final = .; *(.pci_fixup_final) __end_pci_fixups_final = .; __start_pci_fixups_enable = .; *(.pci_fixup_enable) __end_pci_fixups_enable = .; __start_pci_fixups_resume = .; *(.pci_fixup_resume) __end_pci_fixups_resume = .; __start_pci_fixups_resume_early = .; *(.pci_fixup_resume_early) __end_pci_fixups_resume_early = .; __start_pci_fixups_suspend = .; *(.pci_fixup_suspend) __end_pci_fixups_suspend = .; } .builtin_fw : AT(ADDR(.builtin_fw) - 0xffffffff80000000) { __start_builtin_fw = .; *(.builtin_fw) __end_builtin_fw = .; } .rio_ops : AT(ADDR(.rio_ops) - 0xffffffff80000000) { __start_rio_switch_ops = .; *(.rio_switch_ops) __end_rio_switch_ops = .; } . = ALIGN(4); .tracedata : AT(ADDR(.tracedata) - 0xffffffff80000000) { __tracedata_start = .; *(.tracedata) __tracedata_end = .; } __ksymtab : AT(ADDR(__ksymtab) - 0xffffffff80000000) { __start___ksymtab = .; *(__ksymtab) __stop___ksymtab = .; } __ksymtab_gpl : AT(ADDR(__ksymtab_gpl) - 0xffffffff80000000) { __start___ksymtab_gpl = .; *(__ksymtab_gpl) __stop___ksymtab_gpl = .; } __ksymtab_unused : AT(ADDR(__ksymtab_unused) - 0xffffffff80000000) { __start___ksymtab_unused = .; *(__ksymtab_unused) __stop___ksymtab_unused = .; } __ksymtab_unused_gpl : AT(ADDR(__ksymtab_unused_gpl) - 0xffffffff80000000) { __start___ksymtab_unused_gpl = .; *(__ksymtab_unused_gpl) __stop___ksymtab_unused_gpl = .; } __ksymtab_gpl_future : AT(ADDR(__ksymtab_gpl_future) - 0xffffffff80000000) { __start___ksymtab_gpl_future = .; *(__ksymtab_gpl_future) __stop___ksymtab_gpl_future = .; } __kcrctab : AT(ADDR(__kcrctab) - 0xffffffff80000000) { __start___kcrctab = .; *(__kcrctab) __stop___kcrctab = .; } __kcrctab_gpl : AT(ADDR(__kcrctab_gpl) - 0xffffffff80000000) { __start___kcrctab_gpl = .; *(__kcrctab_gpl) __stop___kcrctab_gpl = .; } __kcrctab_unused : AT(ADDR(__kcrctab_unused) - 0xffffffff80000000) { __start___kcrctab_unused = .; *(__kcrctab_unused) __stop___kcrctab_unused = .; } __kcrctab_unused_gpl : AT(ADDR(__kcrctab_unused_gpl) - 0xffffffff80000000) { __start___kcrctab_unused_gpl = .; *(__kcrctab_unused_gpl) __stop___kcrctab_unused_gpl = .; } __kcrctab_gpl_future : AT(ADDR(__kcrctab_gpl_future) - 0xffffffff80000000) { __start___kcrctab_gpl_future = .; *(__kcrctab_gpl_future) __stop___kcrctab_gpl_future = .; } __ksymtab_strings : AT(ADDR(__ksymtab_strings) - 0xffffffff80000000) { *(__ksymtab_strings) } __init_rodata : AT(ADDR(__init_rodata) - 0xffffffff80000000) { *(.ref.rodata) *(.devinit.rodata) *(.devexit.rodata) *(.cpuinit.rodata) *(.cpuexit.rodata) } __param : AT(ADDR(__param) - 0xffffffff80000000) { __start___param = .; *(__param) __stop___param = .; . = ALIGN(((1 << 12))); __end_rodata = .; } . = ALIGN(((1 << 12)));
 . = ALIGN((1 << 21)); __end_rodata_hpage_align = .;
 /* Data */
 .data : AT(ADDR(.data) - 0xffffffff80000000) {
  /* Start of data section */
  _sdata = .;
  /* init_task */
  . = ALIGN(((1 << 12) << 1)); *(.data..init_task)
  . = ALIGN((1 << 12)); *(.data..page_aligned)
  . = ALIGN((1 << (6))); *(.data..cacheline_aligned)
  *(.data) *(.ref.data) *(.devinit.data) *(.devexit.data) *(.cpuinit.data) *(.cpuexit.data) . = ALIGN(32); __start___tracepoints = .; *(__tracepoints) __stop___tracepoints = .; . = ALIGN(8); __start___verbose = .; *(__verbose) __stop___verbose = .; __start___trace_bprintk_fmt = .; *(__trace_printk_fmt) __stop___trace_bprintk_fmt = .; . = ALIGN(32); __start_ftrace_events = .; *(_ftrace_events) __stop_ftrace_events = .; . = ALIGN(32);
  CONSTRUCTORS
  /* rarely changed data like cpu maps */
  . = ALIGN((1 << 7)); *(.data..read_mostly)
  /* End of data section */
  _edata = .;
 } :data
 . = ALIGN(4096);
 __vsyscall_0 = .;
 . = (-10*1024*1024);
 .vsyscall_0 : AT((ADDR(.vsyscall_0) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_0)
 } :user
 . = ALIGN((1 << (6)));
 .vsyscall_fn : AT((ADDR(.vsyscall_fn) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_fn)
 }
 . = ALIGN((1 << (6)));
 .vsyscall_gtod_data : AT((ADDR(.vsyscall_gtod_data) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_gtod_data)
 }
 vsyscall_gtod_data = (ADDR(.vsyscall_gtod_data) - ((-10*1024*1024) - __vsyscall_0));
 .vsyscall_clock : AT((ADDR(.vsyscall_clock) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_clock)
 }
 vsyscall_clock = (ADDR(.vsyscall_clock) - ((-10*1024*1024) - __vsyscall_0));
 .vsyscall_1 ADDR(.vsyscall_0) + 1024: AT((ADDR(.vsyscall_1) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_1)
 }
 .vsyscall_2 ADDR(.vsyscall_0) + 2048: AT((ADDR(.vsyscall_2) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_2)
 }
 .vgetcpu_mode : AT((ADDR(.vgetcpu_mode) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vgetcpu_mode)
 }
 vgetcpu_mode = (ADDR(.vgetcpu_mode) - ((-10*1024*1024) - __vsyscall_0));
 . = ALIGN((1 << (6)));
 .jiffies : AT((ADDR(.jiffies) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.jiffies)
 }
 jiffies = (ADDR(.jiffies) - ((-10*1024*1024) - __vsyscall_0));
 .vsyscall_3 ADDR(.vsyscall_0) + 3072: AT((ADDR(.vsyscall_3) - ((-10*1024*1024) - __vsyscall_0 + 0xffffffff80000000))) {
  *(.vsyscall_3)
 }
 . = __vsyscall_0 + (1 << 12);
 /* Init code and data - will be freed after init */
 . = ALIGN((1 << 12));
 .init.begin : AT(ADDR(.init.begin) - 0xffffffff80000000) {
  __init_begin = .; /* paired with __init_end */
 }
 /*
	 * percpu offsets are zero-based on SMP.  PERCPU_VADDR() changes the
	 * output PHDR, so the next output section - .init.text - should
	 * start another segment - init.
	 */
 __per_cpu_load = .; .data..percpu 0 : AT(__per_cpu_load - 0xffffffff80000000) { __per_cpu_start = .; *(.data..percpu..first) *(.data..percpu..page_aligned) *(.data..percpu) *(.data..percpu..shared_aligned) __per_cpu_end = .; } :percpu . = __per_cpu_load + SIZEOF(.data..percpu);
 . = ALIGN((1 << 12)); .init.text : AT(ADDR(.init.text) - 0xffffffff80000000) { _sinittext = .; *(.init.text) *(.meminit.text) _einittext = .; }
 :init
 .init.data : AT(ADDR(.init.data) - 0xffffffff80000000) { *(.init.data) *(.meminit.data) . = ALIGN(8); __ctors_start = .; *(.ctors) __ctors_end = .; *(.init.rodata) *(.meminit.rodata) . = ALIGN(16); __setup_start = .; *(.init.setup) __setup_end = .; __initcall_start = .; *(.initcallearly.init) __early_initcall_end = .; *(.initcall0.init) *(.initcall0s.init) *(.initcall1.init) *(.initcall1s.init) *(.initcall2.init) *(.initcall2s.init) *(.initcall3.init) *(.initcall3s.init) *(.initcall4.init) *(.initcall4s.init) *(.initcall5.init) *(.initcall5s.init) *(.initcallrootfs.init) *(.initcall6.init) *(.initcall6s.init) *(.initcall7.init) *(.initcall7s.init) __initcall_end = .; __con_initcall_start = .; *(.con_initcall.init) __con_initcall_end = .; __security_initcall_start = .; *(.security_initcall.init) __security_initcall_end = .; . = ALIGN((1 << 12)); __initramfs_start = .; *(.init.ramfs) __initramfs_end = .; }
 .x86_cpu_dev.init : AT(ADDR(.x86_cpu_dev.init) - 0xffffffff80000000) {
  __x86_cpu_dev_start = .;
  *(.x86_cpu_dev.init)
  __x86_cpu_dev_end = .;
 }
 . = ALIGN(8);
 .parainstructions : AT(ADDR(.parainstructions) - 0xffffffff80000000) {
  __parainstructions = .;
  *(.parainstructions)
  __parainstructions_end = .;
 }
 . = ALIGN(8);
 .altinstructions : AT(ADDR(.altinstructions) - 0xffffffff80000000) {
  __alt_instructions = .;
  *(.altinstructions)
  __alt_instructions_end = .;
 }
 .altinstr_replacement : AT(ADDR(.altinstr_replacement) - 0xffffffff80000000) {
  *(.altinstr_replacement)
 }
 /*
	 * .exit.text is discard at runtime, not link time, to deal with
	 *  references from .altinstructions and .eh_frame
	 */
 .exit.text : AT(ADDR(.exit.text) - 0xffffffff80000000) {
  *(.exit.text) *(.memexit.text)
 }
 .exit.data : AT(ADDR(.exit.data) - 0xffffffff80000000) {
  *(.exit.data) *(.memexit.data) *(.memexit.rodata)
 }
 . = ALIGN((1 << 12));
 /* freed after init ends here */
 .init.end : AT(ADDR(.init.end) - 0xffffffff80000000) {
  __init_end = .;
 }
 /*
	 * smp_locks might be freed after init
	 * start/end must be page aligned
	 */
 . = ALIGN((1 << 12));
 .smp_locks : AT(ADDR(.smp_locks) - 0xffffffff80000000) {
  __smp_locks = .;
  *(.smp_locks)
  . = ALIGN((1 << 12));
  __smp_locks_end = .;
 }
 .data_nosave : AT(ADDR(.data_nosave) - 0xffffffff80000000) {
  . = ALIGN((1 << 12)); __nosave_begin = .; *(.data..nosave) . = ALIGN((1 << 12)); __nosave_end = .;
 }
 /* BSS */
 . = ALIGN((1 << 12));
 .bss : AT(ADDR(.bss) - 0xffffffff80000000) {
  __bss_start = .;
  *(.bss..page_aligned)
  *(.bss)
  . = ALIGN(4);
  __bss_stop = .;
 }
 . = ALIGN((1 << 12));
 .brk : AT(ADDR(.brk) - 0xffffffff80000000) {
  __brk_base = .;
  . += 64 * 1024; /* 64k alignment slop space */
  *(.brk_reservation) /* areas brk users have reserved */
  __brk_limit = .;
 }
 _end = .;
        .stab 0 : { *(.stab) } .stabstr 0 : { *(.stabstr) } .stab.excl 0 : { *(.stab.excl) } .stab.exclstr 0 : { *(.stab.exclstr) } .stab.index 0 : { *(.stab.index) } .stab.indexstr 0 : { *(.stab.indexstr) } .comment 0 : { *(.comment) }
        .debug 0 : { *(.debug) } .line 0 : { *(.line) } .debug_srcinfo 0 : { *(.debug_srcinfo) } .debug_sfnames 0 : { *(.debug_sfnames) } .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .debug_weaknames 0 : { *(.debug_weaknames) } .debug_funcnames 0 : { *(.debug_funcnames) } .debug_typenames 0 : { *(.debug_typenames) } .debug_varnames 0 : { *(.debug_varnames) }
 /* Sections to be discarded */
 /DISCARD/ : { *(.exit.text) *(.memexit.text) *(.exit.data) *(.memexit.data) *(.memexit.rodata) *(.exitcall.exit) *(.discard) *(.discard.*) }
 /DISCARD/ : { *(.eh_frame) }
}
/*
 * Per-cpu symbols which need to be offset from __per_cpu_load
 * for the boot processor.
 */
init_per_cpu__gdt_page = gdt_page + __per_cpu_load;
init_per_cpu__irq_stack_union = irq_stack_union + __per_cpu_load;
/*
 * Build-time check on the image size:
 */
. = ASSERT((_end - _text <= (512 * 1024 * 1024)),
    "kernel image bigger than KERNEL_IMAGE_SIZE");
. = ASSERT((irq_stack_union == 0),
           "irq_stack_union is not at start of per-cpu area");
. = ASSERT(kexec_control_code_size <= 2048,
           "kexec control code size is too big");
