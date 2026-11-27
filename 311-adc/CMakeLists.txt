cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(311_adc)

pico_sdk_init()

# предупреждения — всем целям, включая библиотеки; ставим до add_subdirectory()
add_compile_options(-Wall)

add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# выключатель профилирования: 0 — вызовов секундомеров в образе нет
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
    PROFILING_STOPWATCH_COUNT=6
)

# железо прибора: единственное место проекта с SDK
add_library(platform STATIC
    platform/platform.c
)
# путь к заголовкам — папка проекта: через него logic находит
# и platform/platform.h, и firmware.h
target_include_directories(platform PUBLIC
    .
)
# SDK platform подключает только для себя
target_link_libraries(platform PRIVATE
    pico_stdlib
    hardware_adc
)

# логика прибора: задачи и команды, без SDK
add_library(logic STATIC
    logic/api/api.c
    logic/api/api-callbacks.c
    logic/led-task/led-task.c
    logic/button-task/button-task.c
    logic/pi-task/pi-task.c
    logic/telemetry-task/telemetry-task.c
)

# путь к заголовкам — папка logic
target_include_directories(logic PUBLIC
    logic
)
# протокол — PUBLIC: команда command_t нужна и пользователю api,
# остальные библиотеки — PRIVATE: SDK наружу не уходит
target_link_libraries(logic PUBLIC
    stdio_text_protocol
)
target_link_libraries(logic PRIVATE
    systime
    profiling
    device
    platform
)

add_executable(${PROJECT_NAME}
    main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    .
)

target_link_libraries(${PROJECT_NAME}
    logic
    platform
    profiling
    logging
    pico_stdlib
)


pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})