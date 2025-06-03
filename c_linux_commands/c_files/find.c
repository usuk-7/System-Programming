#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

typedef struct {
    char* pattern;
    int action_print;
    int action_ls;
} find_options;

// 함수 프로토타입
void find_files(const char* path, find_options* opts);
void print_ls_format(const char* path, struct stat* st);
char get_file_type(mode_t mode);
void print_permissions(mode_t mode);

char get_file_type(mode_t mode) {
    if (S_ISREG(mode)) return '-';
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

void print_permissions(mode_t mode) {
    printf("%c%c%c%c%c%c%c%c%c",
        (mode & S_IRUSR) ? 'r' : '-',
        (mode & S_IWUSR) ? 'w' : '-',
        (mode & S_IXUSR) ? 'x' : '-',
        (mode & S_IRGRP) ? 'r' : '-',
        (mode & S_IWGRP) ? 'w' : '-',
        (mode & S_IXGRP) ? 'x' : '-',
        (mode & S_IROTH) ? 'r' : '-',
        (mode & S_IWOTH) ? 'w' : '-',
        (mode & S_IXOTH) ? 'x' : '-');
}

void print_ls_format(const char* path, struct stat* st) {
    // inode 번호
    printf("%8lu ", (unsigned long)st->st_ino);
    
    // 블록 수 (1K 블록)
    printf("%4lu ", (unsigned long)((st->st_blocks + 1) / 2));
    
    // 파일 타입과 권한
    printf("%c", get_file_type(st->st_mode));
    print_permissions(st->st_mode);
    
    // 링크 수
    printf("%3lu ", (unsigned long)st->st_nlink);
    
    // 소유자 이름
    struct passwd* pw = getpwuid(st->st_uid);
    if (pw) {
        printf("%-8s ", pw->pw_name);
    } else {
        printf("%-8u ", st->st_uid);
    }
    
    // 그룹 이름
    struct group* gr = getgrgid(st->st_gid);
    if (gr) {
        printf("%-8s ", gr->gr_name);
    } else {
        printf("%-8u ", st->st_gid);
    }
    
    // 파일 크기
    printf("%8lld ", (long long)st->st_size);
    
    // 수정 시간
    char time_str[64];
    struct tm* tm = localtime(&st->st_mtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);
    printf("%s ", time_str);
    
    // 파일 경로
    printf("%s", path);
    
    // 심볼릭 링크인 경우 링크 대상 표시
    if (S_ISLNK(st->st_mode)) {
        char link_target[1024];
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);
        }
    }
    
    printf("\n");
}

void find_files(const char* path, find_options* opts) {
    DIR* dir;
    struct dirent* entry;
    struct stat st;
    char full_path[1024];
    
    // 현재 경로의 파일 정보 확인
    if (lstat(path, &st) == 0) {
        // 패턴이 지정되지 않았거나 패턴과 일치하는 경우
        int matches = 1;
        if (opts->pattern) {
            const char* filename = strrchr(path, '/');
            filename = filename ? filename + 1 : path;
            matches = (fnmatch(opts->pattern, filename, 0) == 0);
        }
        
        if (matches) {
            if (opts->action_ls) {
                print_ls_format(path, &st);
            } else {
                printf("%s\n", path);
            }
        }
    }
    
    // 디렉토리가 아니면 재귀 탐색 종료
    if (!S_ISDIR(st.st_mode)) {
        return;
    }
    
    // 디렉토리 열기
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "find: '%s': 권한이 거부됨\n", path);
        return;
    }
    
    // 디렉토리 내 모든 항목 탐색
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 은 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        if (strcmp(path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }
        
        // 재귀 탐색
        find_files(full_path, opts);
    }
    
    closedir(dir);
}

void print_usage() {
    printf("사용법: find [경로] [옵션]\n");
    printf("옵션:\n");
    printf("  -name 패턴    지정된 패턴과 일치하는 파일명 검색\n");
    printf("  -print        결과를 출력 (기본 동작)\n");
    printf("  -ls          ls -dils 형식으로 자세한 정보 출력\n");
    printf("\n예제:\n");
    printf("  find .               현재 디렉토리의 모든 파일\n");
    printf("  find . -name '*.c'   .c 확장자 파일 검색\n");
    printf("  find /tmp -name test -ls  /tmp에서 test 파일을 ls 형식으로 출력\n");
}

int main(int argc, char* argv[]) {
    find_options opts = {0};
    char* search_path = ".";
    int i;
    
    // 기본값 설정
    opts.action_print = 1; // 기본적으로 -print 동작
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "find: -name 옵션에는 패턴이 필요합니다\n");
                return 1;
            }
            opts.pattern = argv[++i];
        } else if (strcmp(argv[i], "-print") == 0) {
            opts.action_print = 1;
            opts.action_ls = 0;
        } else if (strcmp(argv[i], "-ls") == 0) {
            opts.action_print = 0;
            opts.action_ls = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] != '-') {
            search_path = argv[i];
        } else {
            fprintf(stderr, "find: 알 수 없는 옵션: %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }
    
    // 검색 시작
    find_files(search_path, &opts);
    
    return 0;
} 