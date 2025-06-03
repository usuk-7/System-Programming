#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

typedef struct {
    int recursive;
} ChownOptions;

typedef struct {
    uid_t uid;
    gid_t gid;
    int change_user;
    int change_group;
} ChownSpec;

// Function prototypes
int chown_directory_recursive(const char* path, const ChownSpec* spec, const ChownOptions* opts);

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

uid_t parse_user(const char* user_str) {
    if (is_numeric(user_str)) {
        return (uid_t)atoi(user_str);
    }
    
    struct passwd* pw = getpwnam(user_str);
    if (pw) {
        return pw->pw_uid;
    }
    
    return (uid_t)-1;
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

int parse_owner_spec(const char* spec_str, ChownSpec* spec) {
    spec->change_user = 0;
    spec->change_group = 0;
    spec->uid = -1;
    spec->gid = -1;
    
    char* str = strdup(spec_str);
    char* colon = strchr(str, ':');
    
    if (colon) {
        *colon = '\0';
        char* user_part = str;
        char* group_part = colon + 1;
        
        if (strlen(user_part) > 0) {
            spec->uid = parse_user(user_part);
            if (spec->uid == (uid_t)-1) {
                fprintf(stderr, "chown: 잘못된 사용자: '%s'\n", user_part);
                free(str);
                return 1;
            }
            spec->change_user = 1;
        }
        
        if (strlen(group_part) > 0) {
            spec->gid = parse_group(group_part);
            if (spec->gid == (gid_t)-1) {
                fprintf(stderr, "chown: 잘못된 그룹: '%s'\n", group_part);
                free(str);
                return 1;
            }
            spec->change_group = 1;
        }
    } else {
        spec->uid = parse_user(str);
        if (spec->uid == (uid_t)-1) {
            fprintf(stderr, "chown: 잘못된 사용자: '%s'\n", str);
            free(str);
            return 1;
        }
        spec->change_user = 1;
    }
    
    free(str);
    
    if (!spec->change_user && !spec->change_group) {
        fprintf(stderr, "chown: 변경할 소유권이 지정되지 않았습니다\n");
        return 1;
    }
    
    return 0;
}

int chown_file(const char* path, const ChownSpec* spec, const ChownOptions* opts) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        perror("chown");
        return 1;
    }
    
    uid_t new_uid = spec->change_user ? spec->uid : st.st_uid;
    gid_t new_gid = spec->change_group ? spec->gid : st.st_gid;
    
    if (S_ISLNK(st.st_mode)) {
        if (lchown(path, new_uid, new_gid) != 0) {
            perror("chown");
            return 1;
        }
    } else {
        if (chown(path, new_uid, new_gid) != 0) {
            perror("chown");
            return 1;
        }
    }
    
    if (opts->recursive && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        return chown_directory_recursive(path, spec, opts);
    }
    
    return 0;
}

int chown_directory_recursive(const char* path, const ChownSpec* spec, const ChownOptions* opts) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("chown");
        return 1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* full_path = build_path(path, entry->d_name);
        
        if (chown_file(full_path, spec, opts) != 0) {
            result = 1;
        }
        
        free(full_path);
    }
    
    closedir(dir);
    return result;
}

int main(int argc, char* argv[]) {
    ChownOptions opts = {0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'R':
                        opts.recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "chown: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 2) {
        fprintf(stderr, "chown: 소유자 지정과 파일명이 필요합니다\n");
        return 1;
    }
    
    ChownSpec spec;
    if (parse_owner_spec(argv[start_idx], &spec) != 0) {
        return 1;
    }
    
    int overall_result = 0;
    
    for (int i = start_idx + 1; i < argc; i++) {
        if (chown_file(argv[i], &spec, &opts) != 0) {
            overall_result = 1;
        }
    }
    
    return overall_result;
} 