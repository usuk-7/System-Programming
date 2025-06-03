#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

typedef struct {
    int list_mode;
} crontab_options;

char* get_crontab_file() {
    char* home_dir;
    char* crontab_path;
    struct passwd* pw;
    
    pw = getpwuid(getuid());
    if (!pw) {
        fprintf(stderr, "crontab: 현재 사용자 정보를 가져올 수 없습니다\n");
        return NULL;
    }
    home_dir = pw->pw_dir;
    
    crontab_path = malloc(strlen(home_dir) + 20);
    if (!crontab_path) {
        fprintf(stderr, "crontab: 메모리 할당 실패\n");
        return NULL;
    }
    
    sprintf(crontab_path, "%s/.crontab", home_dir);
    return crontab_path;
}

int list_crontab() {
    char* crontab_file;
    FILE* file;
    char line[1024];
    
    crontab_file = get_crontab_file();
    if (!crontab_file) {
        return 1;
    }
    
    file = fopen(crontab_file, "r");
    if (!file) {
        printf("crontab이 설정되지 않았습니다\n");
        free(crontab_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }
    
    fclose(file);
    free(crontab_file);
    return 0;
}

int main(int argc, char* argv[]) {
    crontab_options opts = {0};
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            opts.list_mode = 1;
        } else {
            fprintf(stderr, "crontab: 알 수 없는 옵션: %s\n", argv[i]);
            fprintf(stderr, "사용법: crontab -l\n");
            return 2;
        }
    }
    
    if (!opts.list_mode) {
        fprintf(stderr, "사용법: crontab -l\n");
        return 2;
    }
    
    return list_crontab();
} 