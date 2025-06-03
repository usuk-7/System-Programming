#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

typedef struct {
    char* pattern;
    int ignore_case;     // -i 옵션
} grep_options;

int search_file(const char* filename, grep_options* opts, int multiple_files) {
    FILE* file;
    char line[4096];
    int found = 0;
    regex_t regex;
    int regex_flags = REG_NOSUB;
    
    // 대소문자 무시 옵션 설정
    if (opts->ignore_case) {
        regex_flags |= REG_ICASE;
    }
    
    // 정규표현식 컴파일
    int ret = regcomp(&regex, opts->pattern, regex_flags);
    if (ret != 0) {
        char error_msg[256];
        regerror(ret, &regex, error_msg, sizeof(error_msg));
        fprintf(stderr, "grep: 잘못된 정규표현식: %s\n", error_msg);
        return -1;
    }
    
    // 파일 열기
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "grep: %s: 파일을 열 수 없습니다\n", filename);
            regfree(&regex);
            return 2;
        }
    }
    
    // 파일을 줄 단위로 읽기
    while (fgets(line, sizeof(line), file) != NULL) {
        // 줄 끝의 개행 문자 제거
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // 패턴 매칭
        if (regexec(&regex, line, 0, NULL, 0) == 0) {
            found = 1;
            
            // 파일명 출력 (여러 파일인 경우)
            if (multiple_files) {
                printf("%s:", filename);
            }
            
            // 매칭된 줄 출력
            printf("%s\n", line);
        }
    }
    
    if (file != stdin) {
        fclose(file);
    }
    
    regfree(&regex);
    return found ? 0 : 1;
}

int main(int argc, char* argv[]) {
    grep_options opts = {0};
    int i;
    int file_start_index = -1;
    int exit_code = 1;
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            opts.ignore_case = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "grep: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            // 첫 번째 비옵션 인수는 패턴
            if (!opts.pattern) {
                opts.pattern = argv[i];
            } else {
                // 파일 시작 위치 기록
                if (file_start_index == -1) {
                    file_start_index = i;
                }
            }
        }
    }
    
    // 패턴이 없으면 에러
    if (!opts.pattern) {
        fprintf(stderr, "grep: 패턴이 필요합니다\n");
        return 2;
    }
    
    // 파일이 지정되지 않은 경우 표준 입력 사용
    if (file_start_index == -1) {
        exit_code = search_file("-", &opts, 0);
    } else {
        // 여러 파일 처리
        int multiple_files = (argc - file_start_index) > 1;
        
        for (i = file_start_index; i < argc; i++) {
            int result = search_file(argv[i], &opts, multiple_files);
            
            if (result == 0) {
                exit_code = 0; // 매칭 발견
            } else if (result == 2) {
                exit_code = 2; // 에러 발생
            }
        }
    }
    
    return exit_code;
} 