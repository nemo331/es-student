#include "api/api.h"
#include "api/api-commands.h"

#include <stdio.h>
#include <string.h>

void api_handle(const command_t *command)
{
    // команда принята не целиком — выполнять нельзя
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    // help выполняет механизм: печатает каждую команду таблицы с её справкой,
    // начиная с help
    if (strcmp(command->name, "help") == 0)
    {
        for (uint32_t i = 0; i < api_command_count; i++)
        {
            printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
        }

        return;
    }

    // ищем команду в таблице
    for (uint32_t i = 0; i < api_command_count; i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            api_commands[i].callback(command);
            return;
        }
    }

    printf("error: unknown command '%s', try help\n", command->name);
}
