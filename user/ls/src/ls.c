#include <assert.h>
#include <dirent.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print_file_info(const char *path, const struct dirent *entry)
{
    struct stat file_stat = {0};
    char full_path[1024];

    if (strlen(path) == 1 && strncmp(path, "/", 1) == 0) {
        snprintf(full_path, sizeof(full_path), "/%s", entry->name);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->name);
    }

    const int fd = open(full_path, O_RDONLY);
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        return;
    }
    close(fd);

    if (file_stat.st_lfn) {
        return;
    }

    printf(" ");

    // File type and permissions
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
    printf(" ");

    printf("%10ld ", (long)file_stat.st_size);

    struct tm modify_time = {0};
    unix_timestamp_to_tm(file_stat.st_mtime, &modify_time);

    const char *date_time_format = "%Y %B %d %H:%M";
    char modify_time_str[25]     = {0};

    strftime(date_time_format, &modify_time, modify_time_str, sizeof(modify_time_str));
    printf("%s ", modify_time_str);

    if (S_ISDIR(file_stat.st_mode)) {
        printf(KBBLU "%s\n" KWHT, entry->name);
    } else if (file_stat.st_mode & S_IFCHR) {
        printf(KGRN "%s\n" KWHT, entry->name);
    } else if (file_stat.st_mode & S_IFBLK) {
        printf(KBRED "%s\n" KWHT, entry->name);
    } else {
        printf("%s\n", entry->name);
    }
}

int main(int argc, char **argv)
{
    const char *path;

    if (argc == 2) {
        path = argv[1];
    } else if (argc == 1) {
        path = getcwd();
    } else {
        printf("Usage: ls [directory]\n");
        return -1;
    }

    DIR *dir = opendir(path);

    ASSERT(dir);

    printf("\n");
    const struct dirent *entry = readdir(dir);
    if ((int)(int *)entry < 0) {
        printf("ls: cannot access '%s': No such file or directory\n", argv[1]);
        return -1;
    }

    while (entry != nullptr) {
        print_file_info(path, entry);
        entry = readdir(dir);
    }

    free(dir);

    return ALL_OK;
}