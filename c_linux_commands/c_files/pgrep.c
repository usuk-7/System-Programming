#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

typedef struct {
    int show_name;
    int newest_only;
} PgrepOptions;

typedef struct {
    pid_t pid;
    char comm[256];
    unsigned long starttime;
} ProcessInfo;

int is_number(const char* str) {
    if (!str || !*str) return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

int read_proc_info(pid_t pid, ProcessInfo* info) {
    char path[256];
    FILE* fp;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) return 0;
    
    char comm_with_parens[256];
    fscanf(fp, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*lu %*lu %*d %*d %*d %*d %*d %*d %lu",
           &info->pid, comm_with_parens, &info->starttime);
    fclose(fp);
    
    if (strlen(comm_with_parens) > 2) {
        strncpy(info->comm, comm_with_parens + 1, sizeof(info->comm) - 1);
        info->comm[strlen(info->comm) - 1] = '\0';
    }
    
    return 1;
}

int match_process_name(const char* proc_name, const char* pattern) {
    return strstr(proc_name, pattern) != NULL;
}

void search_processes(const char* pattern, const PgrepOptions* opts) {
    DIR* proc_dir;
    struct dirent* entry;
    ProcessInfo info;
    ProcessInfo matches[1024];
    int match_count = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("pgrep");
        return;
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) {
            continue;
        }
        
        pid_t pid = atoi(entry->d_name);
        
        if (read_proc_info(pid, &info)) {
            if (match_process_name(info.comm, pattern)) {
                if (match_count < 1024) {
                    matches[match_count++] = info;
                }
            }
        }
    }
    
    closedir(proc_dir);
    
    if (match_count == 0) {
        return;
    }
    
    if (opts->newest_only) {
        ProcessInfo* newest = &matches[0];
        for (int i = 1; i < match_count; i++) {
            if (matches[i].starttime > newest->starttime) {
                newest = &matches[i];
            }
        }
        
        if (opts->show_name) {
            printf("%d %s\n", newest->pid, newest->comm);
        } else {
            printf("%d\n", newest->pid);
        }
    } else {
        for (int i = 0; i < match_count; i++) {
            if (opts->show_name) {
                printf("%d %s\n", matches[i].pid, matches[i].comm);
            } else {
                printf("%d\n", matches[i].pid);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    PgrepOptions opts = {0, 0};
    int pattern_idx = -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            opts.show_name = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            opts.newest_only = 1;
        } else if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        opts.show_name = 1;
                        break;
                    case 'n':
                        opts.newest_only = 1;
                        break;
                    default:
                        fprintf(stderr, "pgrep: 잘못된 옵션: -%c\n", argv[i][j]);
                        return 1;
                }
            }
        } else {
            pattern_idx = i;
            break;
        }
    }
    
    if (pattern_idx == -1) {
        fprintf(stderr, "pgrep: 검색할 패턴이 필요합니다\n");
        fprintf(stderr, "사용법: pgrep [-l] [-n] 패턴\n");
        return 1;
    }
    
    search_processes(argv[pattern_idx], &opts);
    
    return 0;
} 