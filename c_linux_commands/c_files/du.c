#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

void format_size(long long size, int human_readable, char* result) {
    if (human_readable) {
        if (size < 1024) {
            sprintf(result, "%lldB", size);
        } else if (size < 1024 * 1024) {
            sprintf(result, "%.1fK", size / 1024.0);
        } else if (size < 1024 * 1024 * 1024) {
            sprintf(result, "%.1fM", size / (1024.0 * 1024.0));
        } else {
            sprintf(result, "%.1fG", size / (1024.0 * 1024.0 * 1024.0));
        }
    } else {
        sprintf(result, "%lld", size / 1024);
    }
}

long long get_dir_size(const char* path) {
    struct stat st;
    long long total_size = 0;
    DIR* dir;
    struct dirent* entry;
    char full_path[1024];
    
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    if (S_ISREG(st.st_mode)) {
        return st.st_size;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        return 0;
    }
    
    dir = opendir(path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        total_size += get_dir_size(full_path);
    }
    
    closedir(dir);
    return total_size;
}

int main(int argc, char* argv[]) {
    int human_readable = 0;
    char* target = ".";
    char size_str[32];
    long long size;
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            human_readable = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "du: 알 수 없는 옵션: %s\n", argv[i]);
            return 1;
        } else {
            target = argv[i];
        }
    }
    
    size = get_dir_size(target);
    format_size(size, human_readable, size_str);
    printf("%s\t%s\n", size_str, target);
    
    return 0;
} 