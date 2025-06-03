#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int reverse;  // -r 옵션
} sort_options;

// 정순 비교 함수
int compare_asc(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// 역순 비교 함수
int compare_desc(const void* a, const void* b) {
    return strcmp(*(const char**)b, *(const char**)a);
}

int sort_file(const char* filename, sort_options* opts) {
    FILE* file;
    char** lines = NULL;
    char buffer[4096];
    int line_count = 0;
    int capacity = 100;
    
    // 파일 열기
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "sort: %s: 파일을 열 수 없습니다\n", filename);
            return 1;
        }
    }
    
    // 초기 메모리 할당
    lines = malloc(capacity * sizeof(char*));
    if (!lines) {
        fprintf(stderr, "sort: 메모리 할당 실패\n");
        if (file != stdin) fclose(file);
        return 1;
    }
    
    // 파일을 줄 단위로 읽기
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // 줄 끝의 개행 문자 제거
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // 메모리 부족시 확장
        if (line_count >= capacity) {
            capacity *= 2;
            char** temp = realloc(lines, capacity * sizeof(char*));
            if (!temp) {
                fprintf(stderr, "sort: 메모리 할당 실패\n");
                // 할당된 메모리 해제
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
                if (file != stdin) fclose(file);
                return 1;
            }
            lines = temp;
        }
        
        // 줄 복사해서 저장
        lines[line_count] = malloc(strlen(buffer) + 1);
        if (!lines[line_count]) {
            fprintf(stderr, "sort: 메모리 할당 실패\n");
            // 할당된 메모리 해제
            for (int i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            if (file != stdin) fclose(file);
            return 1;
        }
        strcpy(lines[line_count], buffer);
        line_count++;
    }
    
    if (file != stdin) {
        fclose(file);
    }
    
    // 정렬 수행
    if (line_count > 0) {
        if (opts->reverse) {
            qsort(lines, line_count, sizeof(char*), compare_desc);
        } else {
            qsort(lines, line_count, sizeof(char*), compare_asc);
        }
        
        // 정렬된 결과 출력
        for (int i = 0; i < line_count; i++) {
            printf("%s\n", lines[i]);
        }
    }
    
    // 메모리 해제
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
    
    return 0;
}

int main(int argc, char* argv[]) {
    sort_options opts = {0};
    int i;
    int file_start_index = -1;
    int exit_code = 0;
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            opts.reverse = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "sort: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            // 파일 시작 위치 기록
            if (file_start_index == -1) {
                file_start_index = i;
            }
        }
    }
    
    // 파일이 지정되지 않은 경우 표준 입력 사용
    if (file_start_index == -1) {
        exit_code = sort_file("-", &opts);
    } else {
        // 여러 파일을 하나로 합쳐서 정렬
        FILE* temp_file = tmpfile();
        if (!temp_file) {
            fprintf(stderr, "sort: 임시 파일 생성 실패\n");
            return 1;
        }
        
        // 모든 파일의 내용을 임시 파일에 복사
        for (i = file_start_index; i < argc; i++) {
            FILE* input_file;
            char buffer[4096];
            
            if (strcmp(argv[i], "-") == 0) {
                input_file = stdin;
            } else {
                input_file = fopen(argv[i], "r");
                if (!input_file) {
                    fprintf(stderr, "sort: %s: 파일을 열 수 없습니다\n", argv[i]);
                    fclose(temp_file);
                    return 1;
                }
            }
            
            while (fgets(buffer, sizeof(buffer), input_file) != NULL) {
                fputs(buffer, temp_file);
            }
            
            if (input_file != stdin) {
                fclose(input_file);
            }
        }
        
        // 임시 파일을 처음부터 읽어서 정렬
        rewind(temp_file);
        
        char** lines = NULL;
        char buffer[4096];
        int line_count = 0;
        int capacity = 100;
        
        // 초기 메모리 할당
        lines = malloc(capacity * sizeof(char*));
        if (!lines) {
            fprintf(stderr, "sort: 메모리 할당 실패\n");
            fclose(temp_file);
            return 1;
        }
        
        // 임시 파일을 줄 단위로 읽기
        while (fgets(buffer, sizeof(buffer), temp_file) != NULL) {
            // 줄 끝의 개행 문자 제거
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
            }
            
            // 메모리 부족시 확장
            if (line_count >= capacity) {
                capacity *= 2;
                char** temp = realloc(lines, capacity * sizeof(char*));
                if (!temp) {
                    fprintf(stderr, "sort: 메모리 할당 실패\n");
                    for (int j = 0; j < line_count; j++) {
                        free(lines[j]);
                    }
                    free(lines);
                    fclose(temp_file);
                    return 1;
                }
                lines = temp;
            }
            
            // 줄 복사해서 저장
            lines[line_count] = malloc(strlen(buffer) + 1);
            if (!lines[line_count]) {
                fprintf(stderr, "sort: 메모리 할당 실패\n");
                for (int j = 0; j < line_count; j++) {
                    free(lines[j]);
                }
                free(lines);
                fclose(temp_file);
                return 1;
            }
            strcpy(lines[line_count], buffer);
            line_count++;
        }
        
        fclose(temp_file);
        
        // 정렬 수행
        if (line_count > 0) {
            if (opts.reverse) {
                qsort(lines, line_count, sizeof(char*), compare_desc);
            } else {
                qsort(lines, line_count, sizeof(char*), compare_asc);
            }
            
            // 정렬된 결과 출력
            for (int j = 0; j < line_count; j++) {
                printf("%s\n", lines[j]);
            }
        }
        
        // 메모리 해제
        for (int j = 0; j < line_count; j++) {
            free(lines[j]);
        }
        free(lines);
    }
    
    return exit_code;
} 