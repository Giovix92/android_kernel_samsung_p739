/*
 * Automatically generated C config: don't edit
 * Linux kernel version: 2.6.36.3
 * Thu Dec 29 13:12:18 2011
 */
/*
 * wakeup.ld
 *
 * Linker script for the real-mode wakeup code
 */
/*
 * Definitions for the wakeup data structure at the head of the
 * wakeup code.
 */
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
 . = 0;
 .text : {
   *(.text*)
 }
 . = ALIGN(16);
 .rodata : {
  *(.rodata*)
 }
 .videocards : {
  video_cards = .;
  *(.videocards)
  video_cards_end = .;
 }
 . = ALIGN(16);
 .data : {
   *(.data*)
 }
 .signature : {
  end_signature = .;
  LONG(0x65a22c82)
 }
 . = ALIGN(16);
 .bss : {
  __bss_start = .;
  *(.bss)
  __bss_end = .;
 }

 . = 0x3f00;
 .header : {
  *(.header)
 }

 . = ALIGN(16);
 _end = .;

 /DISCARD/ : {
  *(.note*)
 }

 /*
	 * The ASSERT() sink to . is intentional, for binutils 2.14 compatibility:
	 */
 . = ASSERT(_end <= 0x4000, "Wakeup too big!");
}
