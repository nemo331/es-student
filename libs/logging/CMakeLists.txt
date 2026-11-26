add_library(logging STATIC
    src/log.c
)

target_include_directories(logging PUBLIC
    include
)

# журналу достаточно printf из stdio.h: SDK не подключается совсем
