#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <grp.h>

typedef struct {
    int recursive;
} ChgrpOptions;

// Function prototypes
int chgrp_directory_recursive(const char* path, gid_t gid, const ChgrpOptions* opts);

char* build_path(const char* dir, const char* name) {
    int len = strlen(dir) + strlen(name) + 2;
    char* path = malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}

int is_numeric(const char* str) {
    if (!str || !*str) {
        return 0;
    }
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

gid_t parse_group(const char* group_str) {
    if (is_numeric(group_str)) {
        return (gid_t)atoi(group_str);
    }
    
    struct group* gr = getgrnam(group_str);
    if (gr) {
        return gr->gr_gid;
    }
    
    return (gid_t)-1;
}

int chgrp_file(const char* path, gid_t gid, const ChgrpOptions* opts) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        perror("chgrp");
        return 1;
    }
    
    if (S_ISLNK(st.st_mode)) {
        if (lchown(path, st.st_uid, gid) != 0) {
            perror("chgrp");
            return 1;
        }
    } else {
        if (chown(path, st.st_uid, gid) != 0) {
            perror("chgrp");
            return 1;
        }
    }
    
    if (opts->recursive && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        return chgrp_directory_recursive(path, gid, opts);
    }
    
    return 0;
}

int chgrp_directory_recursive(const char* path, gid_t gid, const ChgrpOptions* opts) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("chgrp");
        return 1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* full_path = build_path(path, entry->d_name);
        
        if (chgrp_file(full_path, gid, opts) != 0) {
            result = 1;
        }
        
        free(full_path);
    }
    
    closedir(dir);
    return result;
}

int main(int argc, char* argv[]) {
    ChgrpOptions opts = {0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'R':
                        opts.recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "chgrp: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 2) {
        fprintf(stderr, "chgrp: 그룹과 파일명이 필요합니다\n");
        return 1;
    }
    
    gid_t gid = parse_group(argv[start_idx]);
    if (gid == (gid_t)-1) {
        fprintf(stderr, "chgrp: 잘못된 그룹: '%s'\n", argv[start_idx]);
        return 1;
    }
    
    int overall_result = 0;
    
    for (int i = start_idx + 1; i < argc; i++) {
        if (chgrp_file(argv[i], gid, &opts) != 0) {
            overall_result = 1;
        }
    }
    
    return overall_result;
} 