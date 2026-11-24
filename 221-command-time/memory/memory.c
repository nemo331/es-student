#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "command.h"
#include "device.h"
#include "memory.h"
#include "led.h"

// main объявлена в другом файле, заголовка у main.c нет —
// поэтому одиночное объявление здесь уместнее, чем новый заголовок
int main(void);

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

// если объём флеша не задан при сборке — считаем дефолтную плату (Pico, 2 МиБ)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#define SRAM_SIZE_BYTES (264 * 1024)  // 264 КБ из datasheet RP2040
#define ROM_SIZE_BYTES  (16 * 1024)   // 16 КБ из datasheet RP2040

// переменные для опыта с секциями .data и .bss
uint32_t data_variable = 100;  // ненулевое значение -> попадёт в .data
uint32_t bss_variable;         // без значения -> попадёт в .bss

// таблица областей: имя, начало, конец, размер
static void row_area(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

// таблица объектов: имя, адрес (hex), значение (dec)
static void row(const char *object, uintptr_t address, uintptr_t value)
{
    printf("%-14s 0x%08x %10u\n", object, (unsigned)address, (unsigned)value);
}

// имя, адрес с признаком Thumb и машинный код (hex — это байты, а не число)
static void row_code(const char *object, uintptr_t address, uintptr_t code)
{
    printf("%-14s 0x%08x 0x%08x\n", object, (unsigned)address, (unsigned)code);
}

// memory.c: паспорт памяти

void mem_info(void)
{
    printf("%-10s %-10s %-10s %-8s\n", "area", "start", "end", "size");

    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES
    row_area("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);

    // sram — базовый адрес из SDK, размер из datasheet
    row_area("sram", SRAM_BASE, SRAM_BASE + SRAM_SIZE_BYTES);

    // rom — базовый адрес из SDK, размер из datasheet
    row_area("rom", ROM_BASE, ROM_BASE + ROM_SIZE_BYTES);

    printf("\n");

    // image — от __flash_binary_start до __flash_binary_end
    row_area("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);

    // free — от __flash_binary_end до конца флеш-памяти
    row_area("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);

    printf("\n");

    // boot2 — от __boot2_start__ до __boot2_end__
    row_area("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);

    // text — от __boot2_end__ до __etext: код и константы
    row_area("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    // data flash — хранение .data: от __etext до конца образа
    row_area("data flash", (uintptr_t)&__etext, (uintptr_t)&__flash_binary_end);

    printf("\n");

    // data ram — работа .data, от __data_start__ до __data_end__
    row_area("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);

    // bss — от __bss_start__ до __bss_end__
    row_area("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);

    // heap — от __bss_end__ до __HeapLimit
    row_area("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);

    // stack — от __StackBottom до __StackTop
    row_area("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

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
    printf("  %-12s%8u = boot2 %u + text %u + data %u\n",
           "flash image", image_size, boot2_size, text_size, data_size);
    printf("  %-12s%8u of %u\n",
           "flash free",
           (unsigned)(XIP_BASE + PICO_FLASH_SIZE_BYTES - (uintptr_t)&__flash_binary_end),
           (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  %-12s%8u = data %u + bss %u\n",
           "ram used", data_size + bss_size, data_size, bss_size);
    printf("  %-12s%8u for heap and %u for stack\n",
           "ram free", heap_size, stack_size);
}

// memory.c: паспорт прошивки

void fw_info(void)
{
    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));

    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
    }

    printf("%-14s %-10s %-10s\n", "object", "address", "value");

    // адреса функций со сброшенным признаком Thumb и два байта кода
    uintptr_t main_addr = (uintptr_t)main;
    uintptr_t fw_addr = (uintptr_t)fw_info;
    uint16_t *main_code = (uint16_t *)(main_addr & ~1u);
    uint16_t *fw_code = (uint16_t *)(fw_addr & ~1u);

    row_code("main", main_addr,
             (uintptr_t)main_code[0] | ((uintptr_t)main_code[1] << 16));
    row_code("fw_info", fw_addr,
             (uintptr_t)fw_code[0] | ((uintptr_t)fw_code[1] << 16));

    // таблица команд: адрес массива и её длина
    row("commands", (uintptr_t)commands, command_count);

    // обработчики: имя команды и адрес обработчика, строкой на команду
    for (uint i = 0; i < command_count; i++)
    {
        printf("%-14s 0x%08x\n",
               commands[i].name,
               (unsigned)(uintptr_t)commands[i].handler);
    }

    // константы паспорта: адрес строки во флеш и само значение
    printf("%-14s 0x%08x %s\n", "DEVICE_PROJECT", (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("%-14s 0x%08x %s\n", "DEVICE_REPO", (unsigned)(uintptr_t)DEVICE_REPO, DEVICE_REPO);
    printf("%-14s 0x%08x %s\n", "DEVICE_BOARD", (unsigned)(uintptr_t)DEVICE_BOARD, DEVICE_BOARD);

    // считаем вызов: data_variable и bss_variable на единицу больше
    data_variable++;
    bss_variable++;

    row("data_variable", (uintptr_t)&data_variable, data_variable);
    row("bss_variable", (uintptr_t)&bss_variable, bss_variable);

    // стек и куча: адрес блока кучи, а не адрес указателя
    row("stack_variable", (uintptr_t)&stack_variable, stack_variable);
    row("heap_variable", (uintptr_t)heap_variable,
        (heap_variable != NULL) ? (uintptr_t)*heap_variable : 0);

    // возвращаем блок кучи
    free(heap_variable);
}

void boot_info(void)
{
    // таблица векторов лежит сразу за загрузчиком второй стадии: 0x10000100
    const uint32_t *vectors = (const uint32_t *)0x10000100;

    uint32_t stack_top     = vectors[0];
    uint32_t reset_handler = vectors[1];

    printf("  %-15s0x%08x\n", "vector table", (unsigned)(uintptr_t)vectors);
    printf("  %-13s0x%08x\n", "stack top", (unsigned)stack_top);
    printf("  %-13s0x%08x\n", "reset", (unsigned)reset_handler);
    printf("  %-13s0x%08x\n", "reset (even)", (unsigned)(reset_handler & ~1u));

    // регистр GPIO_IN блока SIO — 0xd0000004 из datasheet RP2040.
    // volatile: значение меняет аппаратура, компилятору кэшировать нельзя
    volatile uint32_t *gpio_in = (volatile uint32_t *)0xd0000004;

    printf("%-15s0x%08x\n", "gpio in", (unsigned)(uintptr_t)gpio_in);

    // уровень вывода светодиода: из регистра напрямую и через SDK
    printf("  %-13s%u\n", "led bit", (unsigned)((*gpio_in >> led_pin()) & 1u));
    printf("  %-13s%u\n", "gpio_get", (unsigned)gpio_get(led_pin()));
}
