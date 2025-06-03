#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

typedef struct command {
    char* cmd;
    struct command* next;
} command_t;

typedef struct target {
    char* name;
    char** dependencies;
    int dep_count;
    command_t* commands;
    struct target* next;
} target_t;

typedef struct {
    char* makefile;
    char* target;
} make_options;

target_t* targets = NULL;

void free_targets() {
    target_t* current = targets;
    while (current) {
        target_t* next = current->next;
        free(current->name);
        for (int i = 0; i < current->dep_count; i++) {
            free(current->dependencies[i]);
        }
        free(current->dependencies);
        
        command_t* cmd = current->commands;
        while (cmd) {
            command_t* next_cmd = cmd->next;
            free(cmd->cmd);
            free(cmd);
            cmd = next_cmd;
        }
        
        free(current);
        current = next;
    }
}

target_t* find_target(const char* name) {
    target_t* current = targets;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

time_t get_file_mtime(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

int needs_rebuild(target_t* target) {
    time_t target_time = get_file_mtime(target->name);
    
    if (target_time == 0) {
        return 1;
    }
    
    for (int i = 0; i < target->dep_count; i++) {
        time_t dep_time = get_file_mtime(target->dependencies[i]);
        if (dep_time > target_time) {
            return 1;
        }
    }
    
    return 0;
}

int build_target(const char* target_name) {
    target_t* target = find_target(target_name);
    if (!target) {
        if (get_file_mtime(target_name) != 0) {
            return 0;
        }
        fprintf(stderr, "make: *** 타겟 '%s'를 만들 규칙이 없습니다. 정지.\n", target_name);
        return 1;
    }
    
    for (int i = 0; i < target->dep_count; i++) {
        if (build_target(target->dependencies[i]) != 0) {
            return 1;
        }
    }
    
    if (needs_rebuild(target)) {
        printf("Building target: %s\n", target->name);
        
        command_t* cmd = target->commands;
        while (cmd) {
            printf("%s\n", cmd->cmd);
            int result = system(cmd->cmd);
            if (result != 0) {
                fprintf(stderr, "make: *** [%s] 오류 %d\n", target->name, result);
                return 1;
            }
            cmd = cmd->next;
        }
    }
    
    return 0;
}

char* trim_whitespace(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

int parse_makefile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "make: %s: 파일을 열 수 없습니다\n", filename);
        return 1;
    }
    
    char line[1024];
    target_t* current_target = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim_whitespace(line);
        
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }
        
        if (trimmed[0] == '\t' || trimmed[0] == ' ') {
            if (!current_target) {
                fprintf(stderr, "make: *** 타겟 없이 명령어가 나타났습니다. 정지.\n");
                fclose(file);
                return 1;
            }
            
            char* cmd_str = trim_whitespace(trimmed);
            command_t* cmd = malloc(sizeof(command_t));
            cmd->cmd = malloc(strlen(cmd_str) + 1);
            strcpy(cmd->cmd, cmd_str);
            cmd->next = NULL;
            
            if (!current_target->commands) {
                current_target->commands = cmd;
            } else {
                command_t* last = current_target->commands;
                while (last->next) {
                    last = last->next;
                }
                last->next = cmd;
            }
        } else {
            char* colon = strchr(trimmed, ':');
            if (!colon) {
                continue;
            }
            
            *colon = '\0';
            char* target_name = trim_whitespace(trimmed);
            char* deps_str = trim_whitespace(colon + 1);
            
            target_t* target = malloc(sizeof(target_t));
            target->name = malloc(strlen(target_name) + 1);
            strcpy(target->name, target_name);
            target->commands = NULL;
            target->dep_count = 0;
            target->dependencies = NULL;
            target->next = NULL;
            
            if (strlen(deps_str) > 0) {
                char* dep_copy = malloc(strlen(deps_str) + 1);
                strcpy(dep_copy, deps_str);
                
                char* token = strtok(dep_copy, " \t");
                int capacity = 10;
                target->dependencies = malloc(capacity * sizeof(char*));
                
                while (token) {
                    if (target->dep_count >= capacity) {
                        capacity *= 2;
                        target->dependencies = realloc(target->dependencies, capacity * sizeof(char*));
                    }
                    
                    target->dependencies[target->dep_count] = malloc(strlen(token) + 1);
                    strcpy(target->dependencies[target->dep_count], token);
                    target->dep_count++;
                    
                    token = strtok(NULL, " \t");
                }
                
                free(dep_copy);
            }
            
            target->next = targets;
            targets = target;
            current_target = target;
        }
    }
    
    fclose(file);
    return 0;
}

const char* find_makefile() {
    if (access("Makefile", F_OK) == 0) {
        return "Makefile";
    }
    if (access("makefile", F_OK) == 0) {
        return "makefile";
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    make_options opts = {0};
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "make: -f 옵션에는 파일명이 필요합니다\n");
                return 2;
            }
            opts.makefile = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "make: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            opts.target = argv[i];
        }
    }
    
    const char* makefile_name;
    if (opts.makefile) {
        makefile_name = opts.makefile;
    } else {
        makefile_name = find_makefile();
        if (!makefile_name) {
            fprintf(stderr, "make: *** Makefile이 없습니다. 정지.\n");
            return 2;
        }
    }
    
    if (parse_makefile(makefile_name) != 0) {
        free_targets();
        return 1;
    }
    
    const char* target_name;
    if (opts.target) {
        target_name = opts.target;
    } else {
        if (targets) {
            target_name = targets->name;
        } else {
            fprintf(stderr, "make: *** 타겟이 없습니다. 정지.\n");
            free_targets();
            return 2;
        }
    }
    
    int result = build_target(target_name);
    free_targets();
    return result;
} 