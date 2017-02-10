SECTIONS {
  /* assign load address (per  QEMU) */
  .       =     0x70010000;
  /* place text segment(s)           */
  .text : { kernel/lolevel.o(.text) *(.text .rodata) }
  /* place data segment(s)           */
  .data : {                         *(.data        ) }
  /* place bss  segment(s)           */
  .bss  : {                         *(.bss         ) }
  /* align       address (per AAPCS) */
  .       = ALIGN( 8 );
  /* allocate stack for irq mode     */
  .       = . + 0x00001000;
  tos_irq = .;
  /* allocate stack for svc mode     */
  .       = . + 0x00001000;
  tos_svc = .;
  /* allocate stack for P1           */
  .       = . + 0x00001000;
  tos_P1  = .;
  /* allocate stack for P2           */
  .       = . + 0x00001000;
  tos_P2  = .;
  /* allocate stack for P3           */
  .       = . + 0x00001000;
  tos_P3  = .;
  /* allocate stack for P4           */
  .       = . + 0x00001000;
  tos_P4  = .;
  /* allocate stack for P5           */
  .       = . + 0x00001000;
  tos_P5  = .;
  /* allocate stack for P6           */
  .       = . + 0x00001000;
  tos_P6  = .;
}
