#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

/*
 * Linux ls 명령어 구현
 * 
 * 기능:
 * - 기본: 현재 디렉토리의 파일 목록을 컬러로 출력
 * - -A: 숨김 파일 포함 (. 및 .. 제외)
 * - -a: 모든 파일 포함 (. 및 .. 포함)
 * - -l: 상세 정보 출력 (권한, 소유자, 크기, 수정시간 등)
 * - -F: 파일 타입 표시자 추가 (/, *, @ 등)
 */

// ANSI 컬러 코드 정의
#define COLOR_RESET     "\033[0m"
#define COLOR_BLUE      "\033[1;34m"    // 디렉토리
#define COLOR_GREEN     "\033[1;32m"    // 실행 파일
#define COLOR_CYAN      "\033[1;36m"    // 심볼릭 링크
#define COLOR_YELLOW    "\033[1;33m"    // 장치 파일
#define COLOR_RED       "\033[1;31m"    // 압축 파일
#define COLOR_MAGENTA   "\033[1;35m"    // 이미지 파일

// 옵션 플래그 구조체
typedef struct {
    int show_all;       // -a 옵션: 모든 파일 (. 및 .. 포함)
    int show_almost_all; // -A 옵션: 숨김 파일 (. 및 .. 제외)
    int long_format;    // -l 옵션: 상세 정보
    int classify;       // -F 옵션: 파일 타입 표시자
    int use_color;      // 컬러 출력 여부
    int show_size;      // -s 옵션: 블록 크기 표시
    int recursive;      // -R 옵션: 재귀적 디렉토리 표시
} ls_options;

// 파일 정보 구조체
typedef struct {
    char name[256];
    struct stat stat_info;
} file_info;

// 파일 타입에 따른 컬러 반환
const char* get_file_color(mode_t mode, const char* filename) {
    if (S_ISDIR(mode)) {
        return COLOR_BLUE;      // 디렉토리
    } else if (S_ISLNK(mode)) {
        return COLOR_CYAN;      // 심볼릭 링크
    } else if (mode & S_IXUSR) {
        return COLOR_GREEN;     // 실행 파일
    } else if (S_ISCHR(mode) || S_ISBLK(mode)) {
        return COLOR_YELLOW;    // 장치 파일
    }
    
    // 파일 확장자에 따른 컬러
    const char* ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".tar") == 0 || 
            strcmp(ext, ".gz") == 0 || strcmp(ext, ".bz2") == 0) {
            return COLOR_RED;   // 압축 파일
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png") == 0 || 
                   strcmp(ext, ".gif") == 0 || strcmp(ext, ".bmp") == 0) {
            return COLOR_MAGENTA; // 이미지 파일
        }
    }
    
    return COLOR_RESET;         // 일반 파일
}

// 파일 타입 표시자 반환 (-F 옵션)
char get_file_indicator(mode_t mode) {
    if (S_ISDIR(mode)) {
        return '/';             // 디렉토리
    } else if (S_ISLNK(mode)) {
        return '@';             // 심볼릭 링크
    } else if (mode & S_IXUSR) {
        return '*';             // 실행 파일
    } else if (S_ISFIFO(mode)) {
        return '|';             // FIFO (named pipe)
    } else if (S_ISSOCK(mode)) {
        return '=';             // 소켓
    }
    return '\0';                // 일반 파일
}

// 권한을 문자열로 변환
void mode_to_string(mode_t mode, char* str) {
    // 파일 타입
    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else str[0] = '-';
    
    // 소유자 권한
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    
    // 그룹 권한
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    
    // 기타 권한
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    
    str[10] = '\0';
}

// 시간을 문자열로 변환
void format_time(time_t time, char* buffer, size_t size) {
    struct tm* tm_info = localtime(&time);
    strftime(buffer, size, "%b %d %H:%M", tm_info);
}

// 상세 정보 출력 (-l 옵션)
void print_long_format(const file_info* file, const ls_options* opts) {
    char mode_str[11];
    char time_str[20];
    struct passwd* pwd;
    struct group* grp;
    
    // 권한 문자열 생성
    mode_to_string(file->stat_info.st_mode, mode_str);
    
    // 소유자 및 그룹 정보
    pwd = getpwuid(file->stat_info.st_uid);
    grp = getgrgid(file->stat_info.st_gid);
    
    // 수정 시간 포맷팅
    format_time(file->stat_info.st_mtime, time_str, sizeof(time_str));
    
    // 상세 정보 출력
    if (opts->show_size) {
        printf("%4ld ", (long)(file->stat_info.st_blocks / 2));
    }
    
    printf("%s %3ld %-8s %-8s %8ld %s ",
           mode_str,                                    // 권한
           (long)file->stat_info.st_nlink,             // 링크 수
           pwd ? pwd->pw_name : "unknown",             // 소유자
           grp ? grp->gr_name : "unknown",             // 그룹
           (long)file->stat_info.st_size,              // 파일 크기
           time_str);                                  // 수정 시간
    
    // 파일명 출력 (컬러 적용)
    if (opts->use_color) {
        printf("%s%s%s", 
               get_file_color(file->stat_info.st_mode, file->name),
               file->name,
               COLOR_RESET);
    } else {
        printf("%s", file->name);
    }
    
    // 파일 타입 표시자 추가 (-F 옵션)
    if (opts->classify) {
        char indicator = get_file_indicator(file->stat_info.st_mode);
        if (indicator) printf("%c", indicator);
    }
    
    printf("\n");
}

// 간단한 형식으로 파일명 출력
void print_simple_format(const file_info* file, const ls_options* opts) {
    // 파일명 출력 (컬러 적용)
    if (opts->show_size) {
        printf("%4ld ", (long)(file->stat_info.st_blocks / 2));
    }
    
    if (opts->use_color) {
        printf("%s%s%s", 
               get_file_color(file->stat_info.st_mode, file->name),
               file->name,
               COLOR_RESET);
    } else {
        printf("%s", file->name);
    }
    
    // 파일 타입 표시자 추가 (-F 옵션)
    if (opts->classify) {
        char indicator = get_file_indicator(file->stat_info.st_mode);
        if (indicator) printf("%c", indicator);
    }
    
    printf("  ");
}

// 파일 목록 정렬을 위한 비교 함수
int compare_files(const void* a, const void* b) {
    const file_info* file_a = (const file_info*)a;
    const file_info* file_b = (const file_info*)b;
    return strcmp(file_a->name, file_b->name);
}

// 파일이 표시되어야 하는지 확인
int should_show_file(const char* filename, const ls_options* opts) {
    // -a 옵션: 모든 파일 표시
    if (opts->show_all) {
        return 1;
    }
    
    // -A 옵션: . 및 .. 제외하고 숨김 파일 표시
    if (opts->show_almost_all) {
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            return 0;
        }
        return 1;
    }
    
    // 기본: 숨김 파일 제외
    return filename[0] != '.';
}

void list_directory(const char* directory, const ls_options* opts, int is_recursive_call) {
    DIR* dir = opendir(directory);
    if (!dir) {
        perror("ls");
        return;
    }
    
    if (is_recursive_call) {
        printf("\n%s:\n", directory);
    }
    
    file_info files[1000];
    int file_count = 0;
    long total_blocks = 0;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && file_count < 1000) {
        if (!should_show_file(entry->d_name, opts)) {
            continue;
        }
        
        strcpy(files[file_count].name, entry->d_name);
        
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
        if (lstat(full_path, &files[file_count].stat_info) == -1) {
            perror("stat");
            continue;
        }
        
        total_blocks += files[file_count].stat_info.st_blocks / 2;
        file_count++;
    }
    
    closedir(dir);
    
    qsort(files, file_count, sizeof(file_info), compare_files);
    
    if (opts->long_format && file_count > 0) {
        printf("total %ld\n", total_blocks);
    }
    
    for (int i = 0; i < file_count; i++) {
        if (opts->long_format) {
            print_long_format(&files[i], opts);
        } else {
            print_simple_format(&files[i], opts);
        }
    }
    
    if (!opts->long_format && file_count > 0) {
        printf("\n");
    }
    
    if (opts->recursive) {
        for (int i = 0; i < file_count; i++) {
            if (S_ISDIR(files[i].stat_info.st_mode) && 
                strcmp(files[i].name, ".") != 0 && 
                strcmp(files[i].name, "..") != 0) {
                
                char subdir_path[512];
                snprintf(subdir_path, sizeof(subdir_path), "%s/%s", directory, files[i].name);
                list_directory(subdir_path, opts, 1);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // 옵션 초기화 (기본적으로 컬러 출력 활성화)
    ls_options opts = {0, 0, 0, 0, 1, 0, 0};
    const char* directory = ".";  // 기본 디렉토리는 현재 디렉토리
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 옵션 처리
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        opts.show_all = 1;
                        break;
                    case 'A':
                        opts.show_almost_all = 1;
                        break;
                    case 'l':
                        opts.long_format = 1;
                        break;
                    case 'F':
                        opts.classify = 1;
                        break;
                    case 's':
                        opts.show_size = 1;
                        break;
                    case 'R':
                        opts.recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "ls: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
        } else {
            // 디렉토리 경로
            directory = argv[i];
        }
    }
    
    list_directory(directory, &opts, 0);
    
    return 0;
}
