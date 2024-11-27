#include <dirent.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// void print_results(const struct dir_entries *directory);

int main(int argc, char **argv)
{
    // struct dir_entries *directory = calloc(sizeof(struct dir_entries *), 1);
    // if (directory == NULL) {
    //     printf("\nFailed to allocate memory for directory");
    //     return -ENOMEM;
    // }

    const char *current_directory = get_current_directory();

    DIR *dir;
    if (argv[1] == NULL || strlen(argv[1]) < 2) {
        dir = opendir(current_directory);
    } else {
        dir = opendir(argv[1]);
    }

    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);
    printf("\n%s", readdir(dir)->name);

    // if (res == ALL_OK) {
    //     print_results(directory);
    // } else {
    //     printf("\nError: %s", get_error_message(res));
    // }

    free(dir);
    return 0;
}

// void print_results(const struct dir_entries *directory)
// {
// if (directory->count == 0) {
//     return;
// }
// printf(KWHT "\n Entries in directory: %lu\n", directory->count);
// printf(KBBLU " %-14s%-9s%-21s%-8s%s\n", "Name", "Size", "Created", "inode", "Attributes" KWHT);
//
// for (int i = 0; i < directory->count; i++) {
//     struct dir_entry *dir_entry = {};
//     const int res               = readdir(directory, &dir_entry, i);
//     if (res < 0) {
//         printf("Failed to read entry %d\n", i);
//     }
//
//     struct tm create_time = {0};
//     unix_timestamp_to_tm(dir_entry->inode->ctime, &create_time);
//
//     const char *date_time_format = "%Y-%m-%d %H:%M:%S";
//
//     char create_time_str[25] = {0};
//     strftime(date_time_format, &create_time, create_time_str, sizeof(create_time_str));
//
//     char attributes[5] = {0};
//
//     attributes[0] = dir_entry->inode->is_read_only ? 'r' : '-';
//     attributes[1] = dir_entry->inode->is_hidden ? 'h' : '-';
//     attributes[2] = dir_entry->inode->is_system ? 's' : '-';
//     attributes[3] = dir_entry->inode->is_archive ? 'a' : '-';
//
//     switch (dir_entry->inode->type) {
//     case INODE_FILE:
//
//         printf(" %-14s%-9lu%-21s%-8lu%s\n",
//                dir_entry->name,
//                dir_entry->inode->size,
//                create_time_str,
//                dir_entry->inode->inode_number,
//                attributes);
//         break;
//     case INODE_DIRECTORY:
//         if (dir_entry->inode->fs_type == FS_TYPE_RAMFS) {
//             printf(KBYEL " %-14s" KBWHT "%-9s" KWHT "%-21s%-8lu%s\n",
//                    dir_entry->name,
//                    "[DIR]",
//                    create_time_str,
//                    dir_entry->inode->inode_number,
//                    attributes);
//         } else {
//             printf(KCYN " %-14s" KBWHT "%-9s" KWHT "%-21s%-8lu%s\n",
//                    dir_entry->name,
//                    "[DIR]",
//                    create_time_str,
//                    dir_entry->inode->inode_number,
//                    attributes);
//         }
//         break;
//     case INODE_DEVICE:
//         printf(KBRED " %-14s" KBWHT "%-9s" KWHT "%-21s%-8lu%s\n",
//                dir_entry->name,
//                "[DEV]",
//                create_time_str,
//                dir_entry->inode->inode_number,
//                attributes);
//         break;
// }
// }
// }