// command.h
#ifndef COMMAND_H
#define COMMAND_H

typedef void (*command_handler_t)(void);

struct command_t
{
    const char *name;
    command_handler_t handler;
};

extern const struct command_t commands[];
#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

#endif