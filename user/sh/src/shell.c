#include "shell.h"
#include <config.h>
#include <os.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 256

// @brief Generate a stack overflow to test the stack guard page
void stack_overflow();
void print_registers();
bool directory_exists(const char *path);
void shell_terminal_readline(uint8_t *out, int max, bool output_while_typing);
void print_help();
int cmd_lookup(const char *name);
void change_directory(char *args, char *current_directory);
void rand_test();

void test()
{
    int a         = 0;
    const int res = scanf("%d", &a);
    printf("\nResult: %d\n", res);
    printf("Value: %d\n", a);
}

char *command_history[256];
uint8_t history_index = 0;

const cmd commands[] = {
    {"cls",      clear_screen   },
    {"clear",    clear_screen   },
    {"exit",     exit           },
    {"reboot",   reboot         },
    {"shutdown", shutdown       },
    {"help",     print_help     },
    {"so",       stack_overflow },
    {"ps",       ps             },
    {"reg",      print_registers},
    {"ms",       memstat        },
    {"test",     test           },
    {"rand",     rand_test      },
};

int number_of_commands = sizeof(commands) / sizeof(struct cmd);

#pragma GCC diagnostic ignored "-Winfinite-recursion"
static uint32_t pass = 0;

// Generate a stack overflow to test the stack guard page
void stack_overflow() // NOLINT(*-no-recursion)
{
    // char a = 0;
    // if (a) {
    // }
    // putchar('.');
    // printf(KWHT "%d | Stack address: %p | Stack usage: %d KiB | Max: %d KiB\n",
    //        ++pass,
    //        &a,
    //        ((uint32_t)USER_STACK_TOP - (uint32_t)&a) / 1024,
    //        USER_STACK_SIZE / 1024);

    // ReSharper disable once CppDFAInfiniteRecursion
    stack_overflow();
}
#pragma GCC diagnostic pop


void rand_test()
{
    const int fd2 = open("/dev/random", O_RDONLY);
    if (fd2 < 0) {
        printf("Failed to open /dev/random\n");
        return;
    }
    printf("\n");
    while (true) {
        char random[sizeof(int)];
        read(&random, sizeof(int), 1, fd2);
        printf("%s", random);
    }
    close(fd2);
}


int main(int argc, char **argv)
{
    printf(KWHT "\nUser mode shell started\n");
    pass = 0;

    for (int i = 0; i < 256; i++) {
        command_history[i] = calloc(1, 256);
    }

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        char *current_directory = getcwd();
        printf(KWHT "%s" KGRN "> " KWHT, current_directory);

        uint8_t buffer[1024] = {0};
        shell_terminal_readline(buffer, sizeof(buffer), true);

        if (strlen((char *)buffer) != 0) {
            strncpy(command_history[history_index], (char *)buffer, sizeof(buffer));
            history_index++;
        }

        const int command_index = cmd_lookup((char *)buffer);
        if (command_index != -1) {
            commands[command_index].function();
            continue;
        }

        if (strncmp((char *)buffer, "cd", 2) == 0) {
            change_directory((char *)buffer + 3, current_directory);
            continue;
        }

        bool return_immediately = false;
        return_immediately      = str_ends_with((char *)buffer, " &");

        if (return_immediately) {
            buffer[strlen((char *)buffer) - 2] = 0x00;
        }

        // const int pid = fork();
        //
        // if (pid < 0) {
        //     printf("Failed to fork\n");
        // } else if (pid == 0) {
        //     const int count     = count_words((const char *)buffer);
        //     const char *command = strtok((char *)buffer, " ");
        //
        //     const char *args[count];
        //     const char *token = strtok(nullptr, " ");
        //     int i             = 0;
        //     while (token != nullptr && i < count) {
        //         args[i] = token;
        //         token   = strtok(nullptr, " ");
        //         i++;
        //     }
        //
        //     args[i] = nullptr;
        //
        //     exec(command, (const char **)args);
        //     printf("Failed to exec\n");
        //     exit();
        // } else {
        //     if (return_immediately) {
        //         continue;
        //     }
        //     waitpid(pid, nullptr);
        // }


        const int pid = create_process((char *)buffer, current_directory);
        if (pid < 0) {
            printf("\nCommand: %s\n", (char *)buffer);
            printf("Error: %s", get_error_message(pid));
        } else {
            if (return_immediately) {
                continue;
            }
            waitpid(pid, nullptr);
        }

        printf("\n");
    }

    return 0;
}


void shell_terminal_readline(uint8_t *out, const int max, const bool output_while_typing)
{
    uint8_t current_history_index = history_index;
    int i                         = 0;
    for (; i < max - 1; i++) {
        const unsigned char key = getkey_blocking();
        if (key == 0) {
            continue;
        }

        // Up arrow
        if (key == 226) {
            if (current_history_index == 0) {
                continue;
            }

            for (int j = 0; j < i - 1; j++) {
                printf("\b");
            }
            current_history_index--;
            strncpy((char *)out, command_history[current_history_index], max);
            i = (int)strnlen((char *)out, max);
            printf((char *)out);
            continue;
        }

        // TODO: This is still a little buggy
        // Down arrow
        if (key == 227) {
            if (current_history_index == history_index - 1) {
                continue;
            }

            for (int j = 0; j < i - 1; j++) {
                printf("\b");
            }
            current_history_index++;
            strncpy((char *)out, command_history[current_history_index], max);
            i = (int)strlen((char *)out);
            printf((char *)out);
            continue;
        }

        // Left arrow key
        if (key == 228) {
            if (i <= 0) {
                i = -1;
                continue;
            } else {
                i -= 2;
            }
        }

        if (key == '\n' || key == '\r') {
            break;
        }

        if (key == '\b' && i <= 0) {
            i = -1;
            continue;
        }

        if (output_while_typing) {
            putchar((char)key);
        }

        if (key == '\b' && i > 0) {
            i -= 2;
            continue;
        }

        out[i] = key;
    }

    out[i] = 0x00;
}

void print_help()
{
    printf("\nShell commands:\n");
    printf(KCYN "  cls" KWHT " or" KCYN " clear        " KWHT " Clear the screen\n");
    printf(KCYN "  exit                " KWHT " Exit the shell\n");
    printf(KCYN "  reboot              " KWHT " Reboot the system\n");
    printf(KCYN "  shutdown            " KWHT " Shutdown the system\n");
    printf(KCYN "  help                " KWHT " Display this help message\n");
    printf(KCYN "  cd " KYEL "[directory]      " KWHT " Change the current directory\n");
    printf(KCYN "  ps                  " KWHT " Shows the list of the processes currently executing\n");
    printf(KCYN "  ms                  " KWHT " Memory utilization stats\n");
    printf(KCYN "  so                  " KWHT " Causes a stack overflow for testing purposes\n");
    printf(KCYN "  reg                 " KWHT " Display current registers\n");
    printf(KCYN "  [command]           " KWHT " Run a command\n");
}

int cmd_lookup(const char *name)
{
    for (int index = 0; index < number_of_commands; index++) {
        const size_t cmd_len   = strnlen(commands[index].name, MAX_COMMAND_LENGTH);
        const size_t input_len = strnlen(name, MAX_COMMAND_LENGTH);
        if (strncmp(name, commands[index].name, cmd_len) == 0 && cmd_len == input_len) {
            return index;
        }
    }
    return -1;
}


void change_directory(char *args, char *current_directory)
{
    char new_dir[MAX_PATH_LENGTH] = {0};
    strncpy(new_dir, trim((char *)args, MAX_PATH_LENGTH), MAX_PATH_LENGTH);

    if (strlen(new_dir) == 0) {
        printf("\n");
        return;
    }

    if (strncmp(new_dir, "/", 3) == 0) {
        if (!directory_exists(new_dir)) {
            printf("\nDirectory does not exist: %s\n", new_dir);
            return;
        }

        if (str_ends_with(new_dir, "/")) {
            chdir((char *)args);
        } else {
            const char *new_path = strcat(new_dir, "/");
            chdir(new_path);
        }
    } else if (strncmp(new_dir, "/", 1) == 0) {
        if (!directory_exists(new_dir)) {
            printf("\nDirectory does not exist: %s\n", new_dir);
            return;
        }

        if (!str_ends_with(new_dir, "/")) {
            strcat(new_dir, "/");
        }

        char root[MAX_PATH_LENGTH];

        chdir(strcat(root, new_dir));
    } else if (strncmp(new_dir, "..", 2) == 0) {
        const size_t len = strnlen(current_directory, MAX_PATH_LENGTH);
        if (len == 3) {
            printf("\n");
            return;
        }
        size_t i = len - 2;

        while (current_directory[i] != '/') {
            i--;
        }

        current_directory[i]     = '/';
        current_directory[i + 1] = '\0';

        if (i == 2) {
            current_directory[3] = '\0';
        }

        chdir(current_directory);
    } else {
        if (!directory_exists((char *)args)) {
            printf("\nDirectory does not exist: %s\n", args);
            return;
        }

        if (str_ends_with(new_dir, "/")) {
            const char *new_path = strcat(current_directory, new_dir);
            chdir(new_path);
        } else {
            char *new_path = strcat(current_directory, new_dir);
            chdir(strcat(new_path, "/"));
        }
    }

    printf("\n");
}

bool directory_exists(const char *path)
{
    char current_directory[MAX_PATH_LENGTH];
    const char *current_dir = getcwd();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);

    DIR *dir;
    if (strncmp(path, "/", 3) == 0) {
        dir = opendir(path);
    } else if (strncmp(path, "/", 1) == 0) {
        char root[MAX_PATH_LENGTH] = "/";
        const char *new_path       = strcat(root, path);
        dir                        = opendir(new_path);
    } else {
        const char *new_path = strcat(current_directory, path);
        dir                  = opendir(new_path);
    }

    const bool exists = dir != nullptr;
    if (dir) {
        closedir(dir);
    }

    return exists;
}

void print_registers()
{
    uint32_t esp, ebp, eax, ebx, ecx, edx, esi, edi, eflags;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    asm volatile("movl %%ebp, %0" : "=r"(ebp));
    asm volatile("movl %%eax, %0" : "=r"(eax));
    asm volatile("movl %%ebx, %0" : "=r"(ebx));
    asm volatile("movl %%ecx, %0" : "=r"(ecx));
    asm volatile("movl %%edx, %0" : "=r"(edx));
    asm volatile("movl %%esi, %0" : "=r"(esi));
    asm volatile("movl %%edi, %0" : "=r"(edi));
    asm volatile("pushfl; popl %0" : "=r"(eflags));

    printf("\nESP:\t%lX\n"
           "EBP:    %lx\n"
           "EAX:    %lx\n"
           "EBX:    %lx\n"
           "ECX:    %lx\n"
           "EDX:    %lx\n"
           "ESI:    %lx\n"
           "EDI:    %lx\n"
           "EFLAGS: %lx\n",
           esp,
           ebp,
           eax,
           ebx,
           ecx,
           edx,
           esi,
           edi,
           eflags);
}
