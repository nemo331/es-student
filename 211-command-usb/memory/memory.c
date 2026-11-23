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
    // шапка таблицы: область, начало, конец, размер

    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES

    // sram — базовый адрес из SDK, размер из datasheet
    // rom — базовый адрес из SDK, размер из datasheet

    // image — от __flash_binary_start до __flash_binary_end
    // row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);

    // free  — от __flash_binary_end до конца флеш-памяти
    // boot2 — от __boot2_start__ до __boot2_end__
    // text  — от __boot2_end__ до __etext: код и константы

    // data flash — хранение .data, от __etext, длиной с .data
    // data ram   — работа .data, от __data_start__ до __data_end__
    // bss        — от __bss_start__ до __bss_end__
    // heap       — от __bss_end__ до __HeapLimit
    // stack      — от __StackBottom до __StackTop

    // итог: образ во флеш и из чего он сложился
    // итог: свободно во флеш-памяти из всего её объёма
    // итог: занято в ОЗУ — .data и .bss
    // итог: свободно в ОЗУ — под кучу и под стек

    printf("%-10s %-10s %-10s %8s\n", "area", "start", "end", "size");

    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES
    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);

    // sram — базовый адрес из SDK, размер из datasheet
    row("sram", SRAM_BASE, SRAM_BASE + SRAM_SIZE_BYTES);

    // rom — базовый адрес из SDK, размер из datasheet
    row("rom", ROM_BASE, ROM_BASE + ROM_SIZE_BYTES);

    printf("\n");

    // image — от __flash_binary_start до __flash_binary_end
    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);

    // free  — от __flash_binary_end до конца флеш-памяти
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);

    printf("\n");

    // boot2 — от __boot2_start__ до __boot2_end__ (256 байт загрузчика)
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);

    // text  — от __boot2_end__ до __etext: код и константы
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    // data flash — хранение .data: от __etext, длиной как .data в ОЗУ
    row("data(fl)", (uintptr_t)&__etext,
        (uintptr_t)&__etext + ((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__));

    printf("\n");

    // data ram — работа .data, от __data_start__ до __data_end__
    row("data", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);

    // bss — от __bss_start__ до __bss_end__
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);

    // heap — от __bss_end__ до __HeapLimit
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);

    // stack — от __StackBottom до __StackTop
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\n");

    // итог: образ во флеш и из чего он сложился
    uintptr_t image_size = (uintptr_t)&__flash_binary_end - (uintptr_t)&__flash_binary_start;
    uintptr_t boot2_size = (uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__;
    uintptr_t text_size  = (uintptr_t)&__etext - (uintptr_t)&__boot2_end__;
    uintptr_t data_size  = (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__;

    printf("image: %u = boot2 %u + text %u + data %u\n",
           (unsigned)image_size, (unsigned)boot2_size,
           (unsigned)text_size, (unsigned)data_size);

    // итог: свободно во флеш-памяти из всего её объёма
    printf("flash: %u of %u used, %u free\n",
           (unsigned)((uintptr_t)&__flash_binary_end - XIP_BASE),
           (unsigned)PICO_FLASH_SIZE_BYTES,
           (unsigned)(XIP_BASE + PICO_FLASH_SIZE_BYTES - (uintptr_t)&__flash_binary_end));

    // итог: занято в ОЗУ — .data и .bss
    uintptr_t ram_used = data_size + ((uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__);
    printf("ram used: %u (data %u + bss %u)\n",
           (unsigned)ram_used, (unsigned)data_size,
           (unsigned)((uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__));

    // итог: свободно в ОЗУ — под кучу и под стек
    printf("ram free: heap %u + stack %u\n",
           (unsigned)((uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__),
           (unsigned)((uintptr_t)&__StackTop - (uintptr_t)&__StackBottom));
}