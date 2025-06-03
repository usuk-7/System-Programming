#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

typedef struct {
    int recursive;
} ChmodOptions;

// Function prototypes
int chmod_directory_recursive(const char* path, const char* mode_str, const ChmodOptions* opts);

char* build_path(const char* dir, const char* name) {
    int len = strlen(dir) + strlen(name) + 2;
    char* path = malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}
    
int parse_octal_mode(const char* mode_str) {
    if (!mode_str || strlen(mode_str) == 0) {
        return -1;
    }
    
    for (int i = 0; mode_str[i]; i++) {
        if (!isdigit(mode_str[i]) || mode_str[i] > '7') {
            return -1;
        }
    }
    
    int mode = 0;
    int len = strlen(mode_str);
    
    if (len > 4) {
        return -1;
    }
    
    for (int i = 0; i < len; i++) {
        mode = mode * 8 + (mode_str[i] - '0');
    }
    
    return mode;
}

mode_t parse_symbolic_mode(const char* mode_str, mode_t current_mode) {
    mode_t new_mode = current_mode;
    char* str = strdup(mode_str);
    char* token = strtok(str, ",");
    
    while (token) {
        char* ptr = token;
        mode_t who_mask = 0;
        char op = 0;
        mode_t perm_mask = 0;
        
        while (*ptr && *ptr != '+' && *ptr != '-' && *ptr != '=') {
            switch (*ptr) {
                case 'u': who_mask |= S_IRWXU; break;
                case 'g': who_mask |= S_IRWXG; break;
                case 'o': who_mask |= S_IRWXO; break;
                case 'a': who_mask |= S_IRWXU | S_IRWXG | S_IRWXO; break;
                default:
                    free(str);
                    return (mode_t)-1;
            }
            ptr++;
        }
        
        if (!who_mask) {
            who_mask = S_IRWXU | S_IRWXG | S_IRWXO;
        }
        
        if (*ptr) {
            op = *ptr++;
        } else {
            free(str);
            return (mode_t)-1;
        }
        
        while (*ptr) {
            switch (*ptr) {
                case 'r':
                    if (who_mask & S_IRWXU) perm_mask |= S_IRUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IRGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IROTH;
                    break;
                case 'w':
                    if (who_mask & S_IRWXU) perm_mask |= S_IWUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IWGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IWOTH;
                    break;
                case 'x':
                    if (who_mask & S_IRWXU) perm_mask |= S_IXUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IXGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IXOTH;
                    break;
                default:
                    free(str);
                    return (mode_t)-1;
            }
            ptr++;
        }
        
        switch (op) {
            case '+':
                new_mode |= perm_mask;
                break;
            case '-':
                new_mode &= ~perm_mask;
                break;
            case '=':
                new_mode &= ~who_mask;
                new_mode |= perm_mask;
                break;
        }
        
        token = strtok(NULL, ",");
    }
    
    free(str);
    return new_mode;
}

int chmod_file(const char* path, const char* mode_str, const ChmodOptions* opts) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        perror("chmod");
        return 1;
    }
    
    mode_t new_mode;
    int octal_mode = parse_octal_mode(mode_str);
    
    if (octal_mode >= 0) {
        new_mode = octal_mode;
    } else {
        new_mode = parse_symbolic_mode(mode_str, st.st_mode);
        if (new_mode == (mode_t)-1) {
            fprintf(stderr, "chmod: 잘못된 모드: '%s'\n", mode_str);
            return 1;
        }
    }
    
    if (S_ISLNK(st.st_mode)) {
        return 0;
    }
    
    if (chmod(path, new_mode) != 0) {
        perror("chmod");
        return 1;
    }
    
    if (opts->recursive && S_ISDIR(st.st_mode)) {
        return chmod_directory_recursive(path, mode_str, opts);
    }
    
    return 0;
}

int chmod_directory_recursive(const char* path, const char* mode_str, const ChmodOptions* opts) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("chmod");
        return 1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* full_path = build_path(path, entry->d_name);
        
        if (chmod_file(full_path, mode_str, opts) != 0) {
            result = 1;
        }
        
        free(full_path);
    }
    
    closedir(dir);
    return result;
}

int main(int argc, char* argv[]) {
    ChmodOptions opts = {0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'R':
                        opts.recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "chmod: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 2) {
        fprintf(stderr, "chmod: 모드와 파일명이 필요합니다\n");
        return 1;
    }
    
    char* mode_str = argv[start_idx];
    int overall_result = 0;
    
    for (int i = start_idx + 1; i < argc; i++) {
        if (chmod_file(argv[i], mode_str, &opts) != 0) {
            overall_result = 1;
        }
    }
    
    return overall_result;
} 