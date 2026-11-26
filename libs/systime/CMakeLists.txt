add_library(systime STATIC
    src/systime.c
)

target_include_directories(systime PUBLIC
    include
)

# SDK нужен только этой библиотеке, пользователь о нём не узнаёт
target_link_libraries(systime PRIVATE
    pico_stdlib
)
