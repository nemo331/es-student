add_library(stdio_text_protocol STATIC
    src/stdio-text-protocol.c
)

target_include_directories(stdio_text_protocol PUBLIC
    include
)

# протоколу нужен стандартный ввод-вывод SDK целиком:
# pico_stdio без pico_stdlib не собирается
target_link_libraries(stdio_text_protocol PRIVATE
    pico_stdlib
)
