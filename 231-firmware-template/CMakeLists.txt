cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

# предупреждения — всем целям, включая библиотеки; ставим до add_subdirectory()
add_compile_options(-Wall)

# libs лежит вне папки проекта: второй аргумент задаёт каталог сборки
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# выключатель профилирования: 0 — вызовов секундомеров в образе нет
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)

add_executable(231_firmware_template
    main.c
    api/api.c
    led-task/led-task.c
    button-task/button-task.c
    pi-task/pi-task.c
)

target_include_directories(231_firmware_template PRIVATE
    .
    api
    led-task
    button-task
    pi-task
)

target_link_libraries(231_firmware_template
    stdio_text_protocol
    systime
    profiling
    device
    logging
    pico_stdlib
)


pico_enable_stdio_usb(231_firmware_template 1)
pico_enable_stdio_uart(231_firmware_template 0)

pico_add_extra_outputs(231_firmware_template)
