#include "shell.h"
#include "os.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"

bool directory_exists(const char *path);
void shell_terminal_readline(uchar *out, int max, bool output_while_typing);
void print_help();
int cmd_lookup(const char *name);
void change_directory(char *args, char *current_directory);

char *command_history[256];
uint8_t history_index = 0;

const cmd commands[] = {
    {"cls",      clear_screen},
    {"clear",    clear_screen},
    {"exit",     exit        },
    {"reboot",   reboot      },
    {"shutdown", shutdown    },
    {"help",     print_help  },
};

int number_of_commands = sizeof(commands) / sizeof(struct cmd);


int main(int argc, char **argv)
{
    printf(KWHT "\nUser mode shell started\n");

    for (int i = 0; i < 256; i++) {
        command_history[i] = malloc(256);
    }

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        char *current_directory = get_current_directory();
        printf(KGRN "%s> " KWHT, current_directory);

        uchar buffer[1024];
        shell_terminal_readline(buffer, sizeof(buffer), true);

        strncpy(command_history[history_index], (char *)buffer, sizeof(buffer));
        history_index++;

        const int command_index = cmd_lookup((char *)buffer);
        if (command_index != -1) {
            commands[command_index].function();
            continue;
        }

        if (strncmp((char *)buffer, "cd", 2) == 0) {
            change_directory((char *)buffer + 3, current_directory);
            continue;
        }

        const int pid = create_process((char *)buffer, current_directory);
        if (pid < 0) {
            printf("\nError: %d", pid);
        } else {
            waitpid(pid, ZOMBIE);
            // wait(ZOMBIE);
        }

        putchar('\n');
    }

    return 0;
}


void shell_terminal_readline(uchar *out, const int max, const bool output_while_typing)
{
    uint8_t current_history_index = history_index;
    int i                         = 0;
    for (; i < max - 1; i++) {
        const unsigned char key = getkey_blocking();

        // Up arrow
        if (key == 226) {
            if (current_history_index == 0) {
                continue;
            }

            for (int j = 0; j < i - 1; j++) {
                putchar('\b');
            }
            current_history_index--;
            strncpy((char *)out, command_history[current_history_index], max);
            i = strlen((char *)out);
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
                putchar('\b');
            }
            current_history_index++;
            strncpy((char *)out, command_history[current_history_index], max);
            i = strlen((char *)out);
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
            putchar(key);
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
    printf(KCYN "  cls" KWHT " or" KCYN " clear " KWHT "- Clear the screen\n");
    printf(KCYN "  exit" KWHT " - Exit the shell\n");
    printf(KCYN "  reboot" KWHT " - Reboot the system\n");
    printf(KCYN "  shutdown" KWHT " - Shutdown the system\n");
    printf(KCYN "  help" KWHT " - Display this help message\n");
    printf(KCYN "  cd " KYEL "[directory] " KWHT " - Change the current directory\n");
    printf(KCYN "  [command]" KWHT " - Run a command\n");
}

int cmd_lookup(const char *name)
{
    for (int index = 0; index < number_of_commands; index++) {
        const int len = strlen(commands[index].name);
        if (strncmp(name, commands[index].name, len) == 0) {
            return index;
        }
    }
    return -1;
}

void change_directory(char *args, char *current_directory)
{
    char *new_dir = trim((char *)args);
    if (strlen(new_dir) == 0) {
        printf("\n");
        return;
    }

    if (strncmp(new_dir, "0:/", 3) == 0) {
        if (!directory_exists(new_dir)) {
            printf("\nDirectory does not exist\n");
            return;
        }

        if (str_ends_with(new_dir, "/")) {
            set_current_directory((char *)args);
        } else {
            const char *new_path = strcat(new_dir, "/");
            set_current_directory(new_path);
        }
    } else if (strncmp(new_dir, "/", 1) == 0) {
        if (!directory_exists(new_dir)) {
            printf("\nDirectory does not exist\n");
            return;
        }
        char root[MAX_PATH_LENGTH] = "0:";

        set_current_directory(strcat(root, new_dir));
    } else if (strncmp(new_dir, "..", 2) == 0) {
        const int len = strlen(current_directory);
        if (len == 3) {
            printf("\n");
            return;
        }
        int i = len - 2;

        while (current_directory[i] != '/') {
            i--;
        }

        current_directory[i]     = '/';
        current_directory[i + 1] = '\0';

        if (i == 2) {
            current_directory[3] = '\0';
        }

        set_current_directory(current_directory);
    } else {
        if (!directory_exists((char *)args)) {
            printf("\nDirectory does not exist\n");
            return;
        }

        if (str_ends_with(new_dir, "/")) {
            const char *new_path = strcat(current_directory, new_dir);
            set_current_directory(new_path);
        } else {
            char *new_path = strcat(current_directory, new_dir);
            set_current_directory(strcat(new_path, "/"));
        }
    }

    printf("\n");
}

bool directory_exists(const char *path)
{
    struct file_directory *directory = malloc(sizeof(struct file_directory));
    char current_directory[MAX_PATH_LENGTH];
    const char *current_dir = get_current_directory();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);
    int res = 0;

    if (strncmp(path, "0:/", 3) == 0) {
        res = opendir(directory, path);
    } else if (strncmp(path, "/", 1) == 0) {
        char root[MAX_PATH_LENGTH] = "0:";
        const char *new_path       = strcat(root, path);
        res                        = opendir(directory, new_path);
    } else {
        const char *new_path = strcat(current_directory, path);
        res                  = opendir(directory, new_path);
    }

    free(directory);
    return res >= 0;
}
