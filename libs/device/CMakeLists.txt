add_library(device STATIC
    src/device.c
)

target_include_directories(device PUBLIC
    include
)

target_link_libraries(device PRIVATE
    pico_stdlib
    pico_unique_id
)

# тип платы библиотека получает сама
target_compile_definitions(device PRIVATE
    DEVICE_BOARD="pico_w"
)
