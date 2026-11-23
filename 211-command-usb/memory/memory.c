#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

// memory.c

// если объём флеша не задан при сборке — считаем дефолтную плату (Pico, 2 МиБ)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#define SRAM_SIZE_BYTES (264 * 1024)  // 264 КБ из datasheet RP2040
#define ROM_SIZE_BYTES  (16 * 1024)   // 16 КБ из datasheet RP2040

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    printf("%-10s %-10s %-10s %8s\n", "area", "start", "end", "size");

    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_BASE + SRAM_SIZE_BYTES);
    row("rom", ROM_BASE, ROM_BASE + ROM_SIZE_BYTES);

    printf("\n");

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);

    printf("\n");

    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    // data flash — хранение .data: от __etext до конца образа
    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__flash_binary_end);

    // data ram — работа .data в ОЗУ
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);

    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\n");

    // итог
    unsigned image_size = (unsigned)((uintptr_t)&__flash_binary_end - (uintptr_t)&__flash_binary_start);
    unsigned boot2_size = (unsigned)((uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__);
    unsigned text_size  = (unsigned)((uintptr_t)&__etext - (uintptr_t)&__boot2_end__);
    unsigned data_size  = (unsigned)((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__);
    unsigned bss_size   = (unsigned)((uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__);
    unsigned heap_size  = (unsigned)((uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__);
    unsigned stack_size = (unsigned)((uintptr_t)&__StackTop - (uintptr_t)&__StackBottom);

    printf("total\n");
    printf("  flash image %8u = boot2 %u + text %u + data %u\n",
           image_size, boot2_size, text_size, data_size);
    printf("  flash free %8u of %u\n",
           (unsigned)(XIP_BASE + PICO_FLASH_SIZE_BYTES - (uintptr_t)&__flash_binary_end),
           (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used %10u = data %u + bss %u\n",
           data_size + bss_size, data_size, bss_size);
    printf("  ram free %10u for heap and %u for stack\n",
           heap_size, stack_size);
}
