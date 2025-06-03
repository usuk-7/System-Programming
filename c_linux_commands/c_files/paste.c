#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int serial_mode;  // -s 옵션
    char delimiter;   // 구분자 (기본값: 탭)
} paste_options;

// 한 파일의 모든 줄을 하나로 합치기 (-s 옵션)
int paste_serial(const char* filename, paste_options* opts, int is_first) {
    FILE* file;
    char line[4096];
    int first_line = 1;
    
    // 파일 열기
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "paste: %s: 파일을 열 수 없습니다\n", filename);
            return 1;
        }
    }
    
    // 파일을 줄 단위로 읽고 하나의 줄로 합치기
    while (fgets(line, sizeof(line), file) != NULL) {
        // 줄 끝의 개행 문자 제거
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // 첫 번째 줄이 아니면 구분자 출력
        if (!first_line) {
            printf("%c", opts->delimiter);
        }
        
        printf("%s", line);
        first_line = 0;
    }
    
    printf("\n");
    
    if (file != stdin) {
        fclose(file);
    }
    
    return 0;
}

// 여러 파일의 줄들을 병렬로 합치기 (기본 모드)
int paste_parallel(char** filenames, int file_count, paste_options* opts) {
    FILE** files;
    char** lines;
    int i;
    int any_data;
    
    // 파일 포인터 배열 할당
    files = malloc(file_count * sizeof(FILE*));
    if (!files) {
        fprintf(stderr, "paste: 메모리 할당 실패\n");
        return 1;
    }
    
    // 줄 버퍼 배열 할당
    lines = malloc(file_count * sizeof(char*));
    if (!lines) {
        fprintf(stderr, "paste: 메모리 할당 실패\n");
        free(files);
        return 1;
    }
    
    // 각 줄에 대한 버퍼 할당
    for (i = 0; i < file_count; i++) {
        lines[i] = malloc(4096);
        if (!lines[i]) {
            fprintf(stderr, "paste: 메모리 할당 실패\n");
            for (int j = 0; j < i; j++) {
                free(lines[j]);
            }
            free(lines);
            free(files);
            return 1;
        }
    }
    
    // 모든 파일 열기
    for (i = 0; i < file_count; i++) {
        if (strcmp(filenames[i], "-") == 0) {
            files[i] = stdin;
        } else {
            files[i] = fopen(filenames[i], "r");
            if (!files[i]) {
                fprintf(stderr, "paste: %s: 파일을 열 수 없습니다\n", filenames[i]);
                // 이전에 열린 파일들 닫기
                for (int j = 0; j < i; j++) {
                    if (files[j] != stdin) {
                        fclose(files[j]);
                    }
                }
                // 메모리 해제
                for (int j = 0; j < file_count; j++) {
                    free(lines[j]);
                }
                free(lines);
                free(files);
                return 1;
            }
        }
    }
    
    // 모든 파일에서 동시에 줄 읽기
    do {
        any_data = 0;
        
        // 각 파일에서 한 줄씩 읽기
        for (i = 0; i < file_count; i++) {
            if (fgets(lines[i], 4096, files[i]) != NULL) {
                // 줄 끝의 개행 문자 제거
                size_t len = strlen(lines[i]);
                if (len > 0 && lines[i][len-1] == '\n') {
                    lines[i][len-1] = '\0';
                }
                any_data = 1;
            } else {
                // 파일 끝에 도달하면 빈 문자열
                lines[i][0] = '\0';
            }
        }
        
        // 읽은 데이터가 있으면 출력
        if (any_data) {
            for (i = 0; i < file_count; i++) {
                if (i > 0) {
                    printf("%c", opts->delimiter);
                }
                printf("%s", lines[i]);
            }
            printf("\n");
        }
        
    } while (any_data);
    
    // 파일들 닫기
    for (i = 0; i < file_count; i++) {
        if (files[i] != stdin) {
            fclose(files[i]);
        }
    }
    
    // 메모리 해제
    for (i = 0; i < file_count; i++) {
        free(lines[i]);
    }
    free(lines);
    free(files);
    
    return 0;
}

void print_usage() {
    printf("사용법: paste [옵션] 파일1 [파일2 ...]\n");
    printf("옵션:\n");
    printf("  -s          각 파일의 모든 줄을 하나의 줄로 합침 (serial mode)\n");
    printf("\n예제:\n");
    printf("  paste file1.txt file2.txt   두 파일의 줄을 병렬로 합침\n");
    printf("  paste -s file1.txt          file1.txt의 모든 줄을 하나로 합침\n");
    printf("  paste -s file1.txt file2.txt 각 파일을 별도의 줄로 합침\n");
    printf("  cat file.txt | paste -       표준입력에서 읽어서 처리\n");
}

int main(int argc, char* argv[]) {
    paste_options opts = {0};
    char** filenames = NULL;
    int file_count = 0;
    int i;
    
    // 기본값 설정
    opts.delimiter = '\t';  // 탭 문자
    
    // 파일명 배열 할당
    filenames = malloc((argc - 1) * sizeof(char*));
    if (!filenames) {
        fprintf(stderr, "paste: 메모리 할당 실패\n");
        return 1;
    }
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            opts.serial_mode = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            free(filenames);
            return 0;
        } else if (argv[i][0] == '-' && strlen(argv[i]) > 1) {
            fprintf(stderr, "paste: 알 수 없는 옵션: %s\n", argv[i]);
            print_usage();
            free(filenames);
            return 2;
        } else {
            // 파일명 추가
            filenames[file_count++] = argv[i];
        }
    }
    
    // 파일이 지정되지 않으면 표준 입력 사용
    if (file_count == 0) {
        filenames[0] = "-";
        file_count = 1;
    }
    
    // 실행 모드에 따라 처리
    if (opts.serial_mode) {
        // Serial mode: 각 파일을 하나의 줄로 합치기
        for (i = 0; i < file_count; i++) {
            if (paste_serial(filenames[i], &opts, i == 0) != 0) {
                free(filenames);
                return 1;
            }
        }
    } else {
        // Parallel mode: 여러 파일의 줄들을 병렬로 합치기
        if (paste_parallel(filenames, file_count, &opts) != 0) {
            free(filenames);
            return 1;
        }
    }
    
    free(filenames);
    return 0;
} 