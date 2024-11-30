#include <config.h>
#include <memory.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 25
#define MAX_COLS 80

char buffer[MAX_ROWS][MAX_COLS];
int cursor_row = 0;
int cursor_col = 0;

// void clear_screen()
// {
//     printf("\x1b[2J"); // Clear the entire screen
//     printf("\x1b[H");  // Move cursor to home position
// }

void move_cursor(int row, int col)
{
    printf("\x1b[%d;%dH", row, col);
}

void refresh_screen()
{
    clear_screen();
    for (int i = 0; i < MAX_ROWS - 1; i++) {
        move_cursor(i, 0);
        printf("%.*s", MAX_COLS, buffer[i]);
    }
    move_cursor(cursor_row, cursor_col);
}

int main(const int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
    }

    char *path = argv[1];

    FILE *file = fopen(path, "w+");
    if (file == NULL) {
        printf("Failed to open file %s\n", path);
        return 1;
    }

    struct stat st;
    int res = fstat(file->fd, &st);
    if (res < 0) {
        printf("Failed to get file stat\n");
        return 1;
    }

    int to_read = MAX_COLS * MAX_ROWS;
    if (st.st_size < to_read) {
        to_read = st.st_size;
    }

    memset(buffer, ' ', sizeof(buffer));
    refresh_screen();

    fread(&buffer, to_read, 1, file);

    int ch;
    while ((ch = getkey()) != EOF) {
        if (ch == 0) {
            continue;
        }
        if (ch == 27) { // ESC key
            break;
        } else if (ch == '\n') {
            cursor_row = (cursor_row + 1) % MAX_ROWS;
            cursor_col = 0;
        } else if (ch == 127 || ch == 8) { // Backspace
            if (cursor_col > 0) {
                cursor_col--;
                buffer[cursor_row][cursor_col] = ' ';
            }
        } else {
            buffer[cursor_row][cursor_col] = ch;
            cursor_col                     = (cursor_col + 1) % MAX_COLS;
            if (cursor_col == 0) {
                cursor_row = (cursor_row + 1) % MAX_ROWS;
            }
        }
        refresh_screen();
    }

    fwrite(&buffer, to_read, 1, file);

    fclose(file);

    return 0;
}