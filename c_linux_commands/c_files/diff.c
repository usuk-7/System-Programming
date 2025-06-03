#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int ignore_case;  // -i 옵션
} diff_options;

// 대소문자를 무시하는 문자열 비교
int strcasecmp_custom(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower(*s1) - tolower(*s2);
}

// 문자열 비교 (옵션에 따라)
int compare_lines(const char* line1, const char* line2, diff_options* opts) {
    if (opts->ignore_case) {
        return strcasecmp_custom(line1, line2);
    } else {
        return strcmp(line1, line2);
    }
}

// 파일을 줄 단위로 읽어서 배열에 저장
char** read_file_lines(const char* filename, int* line_count) {
    FILE* file;
    char** lines = NULL;
    char buffer[4096];
    int capacity = 100;
    *line_count = 0;
    
    // 파일 열기
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "diff: %s: 파일을 열 수 없습니다\n", filename);
            return NULL;
        }
    }
    
    // 초기 메모리 할당
    lines = malloc(capacity * sizeof(char*));
    if (!lines) {
        fprintf(stderr, "diff: 메모리 할당 실패\n");
        if (file != stdin) fclose(file);
        return NULL;
    }
    
    // 파일을 줄 단위로 읽기
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // 줄 끝의 개행 문자 제거
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // 메모리 부족시 확장
        if (*line_count >= capacity) {
            capacity *= 2;
            char** temp = realloc(lines, capacity * sizeof(char*));
            if (!temp) {
                fprintf(stderr, "diff: 메모리 할당 실패\n");
                for (int i = 0; i < *line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
                if (file != stdin) fclose(file);
                return NULL;
            }
            lines = temp;
        }
        
        // 줄 복사해서 저장
        lines[*line_count] = malloc(strlen(buffer) + 1);
        if (!lines[*line_count]) {
            fprintf(stderr, "diff: 메모리 할당 실패\n");
            for (int i = 0; i < *line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            if (file != stdin) fclose(file);
            return NULL;
        }
        strcpy(lines[*line_count], buffer);
        (*line_count)++;
    }
    
    if (file != stdin) {
        fclose(file);
    }
    
    return lines;
}

// 간단한 diff 알고리즘 (LCS 기반)
void compare_files(const char* file1_name, const char* file2_name, diff_options* opts) {
    char** lines1;
    char** lines2;
    int count1, count2;
    int i, j;
    int differences = 0;
    
    // 파일들을 읽기
    lines1 = read_file_lines(file1_name, &count1);
    if (!lines1) return;
    
    lines2 = read_file_lines(file2_name, &count2);
    if (!lines2) {
        for (i = 0; i < count1; i++) {
            free(lines1[i]);
        }
        free(lines1);
        return;
    }
    
    // 간단한 줄 단위 비교
    i = 0; // file1의 인덱스
    j = 0; // file2의 인덱스
    
    while (i < count1 || j < count2) {
        if (i >= count1) {
            // file1이 끝났지만 file2에 더 있음 (추가된 줄들)
            if (!differences) {
                printf("%da%d", count1, j + 1);
                if (j + 1 < count2) printf(",%d", count2);
                printf("\n");
                differences = 1;
            }
            while (j < count2) {
                printf("> %s\n", lines2[j]);
                j++;
            }
            break;
        } else if (j >= count2) {
            // file2가 끝났지만 file1에 더 있음 (삭제된 줄들)
            if (!differences) {
                printf("%d", i + 1);
                if (i + 1 < count1) printf(",%d", count1);
                printf("d%d\n", count2);
                differences = 1;
            }
            while (i < count1) {
                printf("< %s\n", lines1[i]);
                i++;
            }
            break;
        } else if (compare_lines(lines1[i], lines2[j], opts) == 0) {
            // 같은 줄
            i++;
            j++;
        } else {
            // 다른 줄 - 간단한 휴리스틱으로 처리
            if (!differences) {
                printf("%dc%d\n", i + 1, j + 1);
                differences = 1;
            }
            printf("< %s\n", lines1[i]);
            printf("---\n");
            printf("> %s\n", lines2[j]);
            i++;
            j++;
        }
    }
    
    // 메모리 해제
    for (i = 0; i < count1; i++) {
        free(lines1[i]);
    }
    free(lines1);
    
    for (i = 0; i < count2; i++) {
        free(lines2[i]);
    }
    free(lines2);
}

int main(int argc, char* argv[]) {
    diff_options opts = {0};
    int i;
    char* file1 = NULL;
    char* file2 = NULL;
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            opts.ignore_case = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "diff: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            if (!file1) {
                file1 = argv[i];
            } else if (!file2) {
                file2 = argv[i];
            } else {
                fprintf(stderr, "diff: 너무 많은 인수\n");
                return 2;
            }
        }
    }
    
    // 파일 인수 확인
    if (!file1 || !file2) {
        fprintf(stderr, "사용법: diff [-i] 파일1 파일2\n");
        return 2;
    }
    
    // 표준입력을 두 번 사용할 수 없음
    if (strcmp(file1, "-") == 0 && strcmp(file2, "-") == 0) {
        fprintf(stderr, "diff: 표준입력을 두 번 사용할 수 없습니다\n");
        return 2;
    }
    
    compare_files(file1, file2, &opts);
    
    return 0;
} 