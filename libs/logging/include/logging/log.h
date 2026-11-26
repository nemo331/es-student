#pragma once

#include <stdio.h>

// журнальные уровни; в собранном приборе работают err и inf
#define LOG_ERR(...) do { fputs("error: ", stdout); printf(__VA_ARGS__); } while (0)
#define LOG_INF(...) printf(__VA_ARGS__)

// отладочные записи собираются только с ключом, по умолчанию пустые
#define LOG_DBG(...) ((void)0)
