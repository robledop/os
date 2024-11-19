#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_results(const struct dir_entries *directory);

int main(int argc, char **argv)
{
    struct dir_entries *directory = calloc(sizeof(struct dir_entries *), 1);
    if (directory == NULL) {
        printf("\nFailed to allocate memory for directory");
        return -ENOMEM;
    }

    const char *current_directory = get_current_directory();

    int res = 0;
    if (argv[1] == NULL || strlen(argv[1]) < 2) {
        res = opendir(directory, current_directory);
    } else {
        res = opendir(directory, argv[1]);
    }

    switch (res) {
    case ALL_OK:
        print_results(directory);
        break;
    default:
        printf("\nError: %s", get_error_message(res));
        return 0;
    }

    return 0;
}

void print_results(const struct dir_entries *directory)
{
    printf(KRESET "\n Entries in directory: %lu\n", directory->count);
    printf(KBOLD KBLU " %-14s%-14s%-14s\n", "Name", "Size", "inode" KRESET KWHT);


    for (int i = 0; i < directory->count; i++) {
        struct dir_entry *dir_entry;
        const int res = readdir(directory, &dir_entry, i);
        if (res < 0) {
            printf("Failed to read entry %d\n", i);
        }

        switch (dir_entry->inode->type) {
        case INODE_FILE:
            printf(" %-14s%-14lu%-14lu\n", dir_entry->name, dir_entry->inode->size, dir_entry->inode->inode_number);
            break;
        case INODE_DIRECTORY:
            printf(KCYN " %-14s" KBOLD KWHT "%-14s" KRESET "%-14lu\n",
                   dir_entry->name,
                   "[DIR]",
                   dir_entry->inode->inode_number);
            break;
        case INODE_DEVICE:
            printf(KBOLD KRED " %-14s" KWHT "%-14s" KRESET "%-14lu\n",
                   dir_entry->name,
                   "[DEV]",
                   dir_entry->inode->inode_number);
            break;
        }

        // if (entry.is_long_name) {
        //     continue;
        // }
        //
        // int len = strlen(entry.name);
        // if (strlen(entry.ext) > 0) {
        //     len += strlen(entry.ext) + 1;
        // }
        // const int spaces = 14 - len;
        //
        // const uint16_t created_day   = entry.creation_date & 0b00011111;
        // const uint16_t created_month = (entry.creation_date & 0b111100000) >> 5;
        // const uint16_t created_year  = ((entry.creation_date & 0b1111111100000000) >> 9) + 1980;
        //
        // const uint16_t created_hour   = (entry.creation_time & 0b1111100000000000) >> 11;
        // const uint16_t created_minute = (entry.creation_time & 0b0000011111100000) >> 5;
        // const uint16_t created_second = (entry.creation_time & 0b0000000000011111) * 2;
        //
        // if (entry.is_directory) {
        //     printf(KCYN " %s" KWHT, entry.name);
        // } else {
        //     printf(KWHT " %s", entry.name);
        // }
        //
        // if (strlen(entry.ext) > 0) {
        //     printf(KWHT ".%s", entry.ext);
        // }
        //
        // for (int x = 0; x < spaces; x++) {
        //     printf(KWHT " ");
        // }
        //
        // printf("%d-%d-%d %d:%d:%d ",
        //        created_year,
        //        created_month,
        //        created_day,
        //        created_hour,
        //        created_minute,
        //        created_second);
        //
        // if (!entry.is_directory) {
        //     printf(KYEL " %d bytes" KWHT, entry.size);
        // }
        //
        // if (entry.is_directory) {
        //     printf(KCYN " [DIR]" KWHT);
        // }
        //
        // for (size_t x = 0; x < 13; x++) {
        //     printf(" ");
        // }
        //
        // if (entry.is_read_only) {
        //     printf(" [RO]");
        // }
        //
        // if (entry.is_hidden) {
        //     printf(" [H]");
        // }
        //
        // if (entry.is_system) {
        //     printf(" [S]");
        // }
        //
        // if (entry.is_volume_label) {
        //     printf(" [V]");
        // }
    }
}