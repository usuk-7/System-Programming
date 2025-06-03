#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

typedef struct {
    int interactive;
    int recursive;
} CpOptions;

int copy_file(const char* src, const char* dest, const CpOptions* opts) {
    struct stat src_stat, dest_stat;
    
    if (stat(src, &src_stat) != 0) {
        perror("cp");
        return 1;
    }
    
    if (!S_ISREG(src_stat.st_mode)) {
        fprintf(stderr, "cp: '%s'는 일반 파일이 아닙니다\n", src);
        return 1;
    }
    
    if (stat(dest, &dest_stat) == 0) {
        if (opts->interactive) {
            printf("cp: '%s'를(을) 덮어쓰시겠습니까? ", dest);
            int c = getchar();
            while (getchar() != '\n');
            if (c != 'y' && c != 'Y') {
                return 0;
            }
        }
    }
    
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("cp");
        return 1;
    }
    
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode & 0777);
    if (dest_fd < 0) {
        perror("cp");
        close(src_fd);
        return 1;
    }
    
    char buffer[8192];
    ssize_t bytes_read, bytes_written;
    
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("cp");
            close(src_fd);
            close(dest_fd);
            return 1;
        }
    }
    
    if (bytes_read < 0) {
        perror("cp");
        close(src_fd);
        close(dest_fd);
        return 1;
    }
    
    close(src_fd);
    close(dest_fd);
    return 0;
}

char* build_path(const char* dir, const char* name) {
    int len = strlen(dir) + strlen(name) + 2;
    char* path = malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}

int copy_directory(const char* src, const char* dest, const CpOptions* opts) {
    struct stat src_stat;
    
    if (stat(src, &src_stat) != 0) {
        perror("cp");
        return 1;
    }
    
    if (mkdir(dest, src_stat.st_mode & 0777) != 0 && errno != EEXIST) {
        perror("cp");
        return 1;
    }
    
    DIR* dir = opendir(src);
    if (!dir) {
        perror("cp");
        return 1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* src_path = build_path(src, entry->d_name);
        char* dest_path = build_path(dest, entry->d_name);
        
        struct stat entry_stat;
        if (stat(src_path, &entry_stat) == 0) {
            if (S_ISDIR(entry_stat.st_mode)) {
                if (copy_directory(src_path, dest_path, opts) != 0) {
                    result = 1;
                }
            } else if (S_ISREG(entry_stat.st_mode)) {
                if (copy_file(src_path, dest_path, opts) != 0) {
                    result = 1;
                }
            }
        }
        
        free(src_path);
        free(dest_path);
    }
    
    closedir(dir);
    return result;
}

int is_directory(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

char* get_basename(const char* path) {
    char* last_slash = strrchr(path, '/');
    if (last_slash) {
        return last_slash + 1;
    }
    return (char*)path;
}

int main(int argc, char* argv[]) {
    CpOptions opts = {0, 0};
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
                        fprintf(stderr, "cp: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 2) {
        fprintf(stderr, "cp: 소스와 대상이 필요합니다\n");
        return 1;
    }
    
    char* dest = argv[argc - 1];
    int num_sources = argc - start_idx - 1;
    
    if (num_sources > 1 && !is_directory(dest)) {
        fprintf(stderr, "cp: 여러 소스를 복사할 때 대상은 디렉토리여야 합니다\n");
        return 1;
    }
    
    for (int i = start_idx; i < argc - 1; i++) {
        char* src = argv[i];
        char* final_dest;
        
        if (is_directory(dest)) {
            char* basename = get_basename(src);
            final_dest = build_path(dest, basename);
        } else {
            final_dest = strdup(dest);
        }
        
        struct stat src_stat;
        if (stat(src, &src_stat) != 0) {
            perror("cp");
            if (final_dest != dest) {
                free(final_dest);
            }
            continue;
        }
        
        int result;
        if (S_ISDIR(src_stat.st_mode)) {
            if (!opts.recursive) {
                fprintf(stderr, "cp: '%s'는 디렉토리입니다 (-r 옵션이 필요합니다)\n", src);
                result = 1;
            } else {
                result = copy_directory(src, final_dest, &opts);
            }
        } else {
            result = copy_file(src, final_dest, &opts);
        }
        
        if (final_dest != dest) {
            free(final_dest);
        }
        
        if (result != 0) {
            return 1;
        }
    }
    
    return 0;
} 