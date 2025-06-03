#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>

typedef struct {
    int show_all;
    int full_format;
} PsOptions;

typedef struct {
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    char comm[256];
    char cmdline[512];
    unsigned long utime;
    unsigned long stime;
    char tty[16];
} ProcessInfo;

int is_number(const char* str) {
    if (!str || !*str) return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

char* get_current_tty() {
    static char tty[16];
    char* tty_name = ttyname(STDIN_FILENO);
    if (tty_name && strstr(tty_name, "/dev/")) {
        char* tty_part = strstr(tty_name, "/dev/") + 5;
        if (strncmp(tty_part, "pts/", 4) == 0) {
            strcpy(tty, tty_part);
        } else {
            strcpy(tty, tty_part);
        }
    } else {
        strcpy(tty, "?");
    }
    return tty;
}

int read_proc_info(pid_t pid, ProcessInfo* info) {
    char path[256];
    FILE* fp;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) return 0;
    
    char comm_with_parens[256];
    int tty_nr;
    fscanf(fp, "%d %s %*c %d %*d %*d %d %*d %*u %*u %*u %*u %*u %lu %lu",
           &info->pid, comm_with_parens, &info->ppid, &tty_nr, 
           &info->utime, &info->stime);
    fclose(fp);
    
    if (strlen(comm_with_parens) > 2) {
        strncpy(info->comm, comm_with_parens + 1, sizeof(info->comm) - 1);
        info->comm[strlen(info->comm) - 1] = '\0';
    }
    
    if (tty_nr == 0) {
        strcpy(info->tty, "?");
    } else {
        snprintf(info->tty, sizeof(info->tty), "pts/%d", tty_nr & 0xff);
    }
    
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (fp) {
        size_t len = fread(info->cmdline, 1, sizeof(info->cmdline) - 1, fp);
        info->cmdline[len] = '\0';
        
        for (size_t i = 0; i < len; i++) {
            if (info->cmdline[i] == '\0' && i + 1 < len) {
                info->cmdline[i] = ' ';
            }
        }
        fclose(fp);
    }
    
    if (strlen(info->cmdline) == 0) {
        strcpy(info->cmdline, info->comm);
    }
    
    struct stat st;
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (stat(path, &st) == 0) {
        info->uid = st.st_uid;
    }
    
    return 1;
}

char* format_time(unsigned long time_ticks) {
    static char time_str[32];
    unsigned long seconds = time_ticks / sysconf(_SC_CLK_TCK);
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;
    
    snprintf(time_str, sizeof(time_str), "%02lu:%02lu", minutes, seconds);
    return time_str;
}

char* get_username(uid_t uid) {
    static char uid_str[16];
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return pw->pw_name;
    } else {
        snprintf(uid_str, sizeof(uid_str), "%u", uid);
        return uid_str;
    }
}

void list_processes(const PsOptions* opts) {
    DIR* proc_dir;
    struct dirent* entry;
    ProcessInfo info;
    uid_t current_uid = getuid();
    char* current_tty = get_current_tty();
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("ps");
        return;
    }
    
    if (opts->full_format) {
        printf("%-8s %5s %5s %5s %-8s %8s %s\n", 
               "UID", "PID", "PPID", "C", "TTY", "TIME", "CMD");
    } else {
        printf("%5s %-8s %8s %s\n", "PID", "TTY", "TIME", "CMD");
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) {
            continue;
        }
        
        pid_t pid = atoi(entry->d_name);
        
        if (read_proc_info(pid, &info)) {
            if (!opts->show_all) {
                if (info.uid != current_uid || 
                    (strcmp(info.tty, current_tty) != 0 && strcmp(info.tty, "?") != 0)) {
                    continue;
                }
            }
            
            unsigned long total_time = info.utime + info.stime;
            
            if (opts->full_format) {
                printf("%-8s %5d %5d %5d %-8s %8s %s\n",
                       get_username(info.uid),
                       info.pid,
                       info.ppid,
                       0,
                       info.tty,
                       format_time(total_time),
                       info.cmdline);
            } else {
                printf("%5d %-8s %8s %s\n",
                       info.pid,
                       info.tty,
                       format_time(total_time),
                       info.cmdline);
            }
        }
    }
    
    closedir(proc_dir);
}

int main(int argc, char* argv[]) {
    PsOptions opts = {0, 0};
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            opts.show_all = 1;
        } else if (strcmp(argv[i], "-f") == 0) {
            opts.full_format = 1;
        } else if (strcmp(argv[i], "-ef") == 0 || strcmp(argv[i], "-fe") == 0) {
            opts.show_all = 1;
            opts.full_format = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "ps: 잘못된 옵션: %s\n", argv[i]);
            return 1;
        }
    }
    
    list_processes(&opts);
    
    return 0;
} 