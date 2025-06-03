#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

typedef struct {
    int interactive;
    int recursive;
} RmOptions;

// Function prototypes
int remove_directory_recursive(const char* path, const RmOptions* opts);

char* build_path(const char* dir, const char* name) {
    int len = strlen(dir) + strlen(name) + 2;
    char* path = malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}

int remove_file(const char* path, const RmOptions* opts) {
    struct stat st;
    
    if (stat(path, &st) != 0) {
        perror("rm");
        return 1;
    }
    
    if (S_ISDIR(st.st_mode)) {
        if (!opts->recursive) {
            fprintf(stderr, "rm: '%s'는 디렉토리입니다 (-r 옵션이 필요합니다)\n", path);
            return 1;
        }
        return remove_directory_recursive(path, opts);
    }
    
    if (opts->interactive) {
        printf("rm: '%s'를(을) 삭제하시겠습니까? ", path);
        int c = getchar();
        while (getchar() != '\n');
        if (c != 'y' && c != 'Y') {
            return 0;
        }
    }
    
    if (unlink(path) != 0) {
        perror("rm");
        return 1;
    }
    
    return 0;
}

int remove_directory_recursive(const char* path, const RmOptions* opts) {
    if (opts->interactive) {
        printf("rm: 디렉토리 '%s'에 들어가시겠습니까? ", path);
        int c = getchar();
        while (getchar() != '\n');
        if (c != 'y' && c != 'Y') {
            return 0;
        }
    }
    
    DIR* dir = opendir(path);
    if (!dir) {
        perror("rm");
        return 1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* full_path = build_path(path, entry->d_name);
        
        struct stat entry_stat;
        if (stat(full_path, &entry_stat) == 0) {
            if (S_ISDIR(entry_stat.st_mode)) {
                if (remove_directory_recursive(full_path, opts) != 0) {
                    result = 1;
                }
            } else {
                if (opts->interactive) {
                    printf("rm: '%s'를(을) 삭제하시겠습니까? ", full_path);
                    int c = getchar();
                    while (getchar() != '\n');
                    if (c != 'y' && c != 'Y') {
                        free(full_path);
                        continue;
                    }
                }
                
                if (unlink(full_path) != 0) {
                    perror("rm");
                    result = 1;
                }
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    if (result == 0) {
        if (opts->interactive) {
            printf("rm: 디렉토리 '%s'를(을) 삭제하시겠습니까? ", path);
            int c = getchar();
            while (getchar() != '\n');
            if (c != 'y' && c != 'Y') {
                return 0;
            }
        }
        
        if (rmdir(path) != 0) {
            perror("rm");
            result = 1;
        }
    }
    
    return result;
}

int is_directory(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int main(int argc, char* argv[]) {
    RmOptions opts = {0, 0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'i':
                        opts.interactive = 1;
                        break;
                    case 'r':
                    case 'R':
                        opts.recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "rm: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 1) {
        fprintf(stderr, "rm: 삭제할 파일이나 디렉토리가 필요합니다\n");
        return 1;
    }
    
    int overall_result = 0;
    
    for (int i = start_idx; i < argc; i++) {
        char* path = argv[i];
        
        if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) {
            fprintf(stderr, "rm: '%s'는 삭제할 수 없습니다\n", path);
            overall_result = 1;
            continue;
        }
        
        if (strcmp(path, "/") == 0) {
            fprintf(stderr, "rm: 루트 디렉토리는 삭제할 수 없습니다\n");
            overall_result = 1;
            continue;
        }
        
        if (remove_file(path, &opts) != 0) {
            overall_result = 1;
        }
    }
    
    return overall_result;
} 