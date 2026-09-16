/*
 * Linker script cho STM32F103C8T6
 *   FLASH : 64 KB tai 0x08000000
 *   RAM   : 20 KB tai 0x20000000
 *
 * Dung voi startup_stm32f10x_md.s ban GCC (thu muc startup/gcc_ride7).
 * File startup do can cac ky hieu: _estack, _sidata, _sdata, _edata,
 * _sbss, _ebss.
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Dinh stack o cuoi RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Kich thuoc toi thieu phai con trong cho stack va heap, neu khong du
   thi linker bao loi ngay luc build thay vi hong luc chay. */
_Min_Heap_Size  = 0x200;   /* 512 byte  */
_Min_Stack_Size = 0x400;   /* 1 KB      */

SECTIONS
{
  /* Bang vector ngat, phai nam dau vung FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* Ma lenh */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Du lieu chi doc */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* Vi tri anh du lieu khoi tao nam trong FLASH */
  _sidata = LOADADDR(.data);

  /* Bien co gia tri khoi tao: nap trong FLASH, chay trong RAM */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* Bien khong khoi tao, duoc xoa ve 0 luc khoi dong */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Kiem tra con du cho cho heap va stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Khong dung khoi /DISCARD/ cho libc.a / libgcc.a nhu mot so mau cua ST:
     firmware nay goi memcpy va memset, neu loai bo cac section do thi ham
     se bien mat khoi anh chuong trinh. */

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
