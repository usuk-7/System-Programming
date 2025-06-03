#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct {
    int symbolic;
} LnOptions;

char* build_path(const char* dir, const char* name) {
    int len = strlen(dir) + strlen(name) + 2;
    char* path = malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
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

int create_link(const char* src, const char* dest, const LnOptions* opts) {
    struct stat dest_stat;
    
    if (stat(dest, &dest_stat) == 0) {
        fprintf(stderr, "ln: '%s': 파일이 이미 존재합니다\n", dest);
        return 1;
    }
    
    if (opts->symbolic) {
        if (symlink(src, dest) != 0) {
            perror("ln");
            return 1;
        }
    } else {
        struct stat src_stat;
        if (stat(src, &src_stat) != 0) {
            perror("ln");
            return 1;
        }
        
        if (S_ISDIR(src_stat.st_mode)) {
            fprintf(stderr, "ln: '%s': 디렉토리에는 하드 링크를 만들 수 없습니다\n", src);
            return 1;
        }
        
        if (link(src, dest) != 0) {
            perror("ln");
            return 1;
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    LnOptions opts = {0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 's':
                        opts.symbolic = 1;
                        break;
                    default:
                        fprintf(stderr, "ln: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 2) {
        fprintf(stderr, "ln: 소스와 대상이 필요합니다\n");
        return 1;
    }
    
    char* dest = argv[argc - 1];
    int num_sources = argc - start_idx - 1;
    
    if (num_sources > 1 && !is_directory(dest)) {
        fprintf(stderr, "ln: 여러 소스에 대해 링크를 만들 때 대상은 디렉토리여야 합니다\n");
        return 1;
    }
    
    int overall_result = 0;
    
    for (int i = start_idx; i < argc - 1; i++) {
        char* src = argv[i];
        char* final_dest;
        
        if (is_directory(dest)) {
            char* basename = get_basename(src);
            final_dest = build_path(dest, basename);
        } else {
            final_dest = strdup(dest);
        }
        
        if (!opts.symbolic) {
            struct stat src_stat;
            if (stat(src, &src_stat) != 0) {
                perror("ln");
                if (final_dest != dest) {
                    free(final_dest);
                }
                overall_result = 1;
                continue;
            }
        }
        
        if (create_link(src, final_dest, &opts) != 0) {
            overall_result = 1;
        }
        
        if (final_dest != dest) {
            free(final_dest);
        }
    }
    
    return overall_result;
} 