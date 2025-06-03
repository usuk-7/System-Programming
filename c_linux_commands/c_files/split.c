#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int lines_per_file;  // -l 옵션
    char* prefix;        // 출력 파일 접두사
} split_options;

// 파일명 생성 (xaa, xab, xac, ...)
void generate_filename(char* filename, const char* prefix, int file_num) {
    int suffix_len = 2; // aa, ab, ac 등
    char suffix[3] = {0};
    
    // 알파벳 조합 생성 (aa, ab, ac, ..., az, ba, bb, ...)
    suffix[0] = 'a' + (file_num / 26);
    suffix[1] = 'a' + (file_num % 26);
    
    snprintf(filename, 256, "%s%s", prefix, suffix);
}

int split_file(const char* input_filename, split_options* opts) {
    FILE* input_file;
    FILE* output_file = NULL;
    char line[4096];
    char output_filename[256];
    int current_line_count = 0;
    int file_count = 0;
    int total_lines = 0;
    
    // 입력 파일 열기
    if (strcmp(input_filename, "-") == 0) {
        input_file = stdin;
    } else {
        input_file = fopen(input_filename, "r");
        if (!input_file) {
            fprintf(stderr, "split: %s: 파일을 열 수 없습니다\n", input_filename);
            return 1;
        }
    }
    
    // 첫 번째 출력 파일 생성
    generate_filename(output_filename, opts->prefix, file_count);
    output_file = fopen(output_filename, "w");
    if (!output_file) {
        fprintf(stderr, "split: %s: 파일을 생성할 수 없습니다\n", output_filename);
        if (input_file != stdin) fclose(input_file);
        return 1;
    }
    
    printf("출력 파일 생성: %s\n", output_filename);
    
    // 파일을 줄 단위로 읽고 분할
    while (fgets(line, sizeof(line), input_file) != NULL) {
        // 현재 출력 파일에 줄 쓰기
        fputs(line, output_file);
        current_line_count++;
        total_lines++;
        
        // 지정된 줄 수에 도달하면 새 파일 생성
        if (current_line_count >= opts->lines_per_file) {
            fclose(output_file);
            file_count++;
            current_line_count = 0;
            
            // 다음 출력 파일 생성
            generate_filename(output_filename, opts->prefix, file_count);
            output_file = fopen(output_filename, "w");
            if (!output_file) {
                fprintf(stderr, "split: %s: 파일을 생성할 수 없습니다\n", output_filename);
                if (input_file != stdin) fclose(input_file);
                return 1;
            }
            printf("출력 파일 생성: %s\n", output_filename);
        }
    }
    
    // 파일 닫기
    if (output_file) {
        fclose(output_file);
    }
    
    if (input_file != stdin) {
        fclose(input_file);
    }
    
    // 마지막 파일이 비어있으면 삭제
    if (current_line_count == 0 && file_count > 0) {
        if (remove(output_filename) == 0) {
            printf("빈 파일 삭제: %s\n", output_filename);
        }
        file_count--;
    }
    
    printf("총 %d줄을 %d개 파일로 분할했습니다\n", total_lines, file_count + 1);
    
    return 0;
}

void print_usage() {
    printf("사용법: split [옵션] [파일] [접두사]\n");
    printf("옵션:\n");
    printf("  -l 줄수     각 출력 파일당 줄 수 (기본값: 1000)\n");
    printf("\n예제:\n");
    printf("  split file.txt              file.txt를 1000줄씩 xaa, xab, ... 로 분할\n");
    printf("  split -l 100 file.txt       file.txt를 100줄씩 분할\n");
    printf("  split -l 50 file.txt part   file.txt를 50줄씩 partaa, partab, ... 로 분할\n");
    printf("  cat file.txt | split -l 200 표준입력을 200줄씩 분할\n");
}

int main(int argc, char* argv[]) {
    split_options opts = {0};
    char* input_file = NULL;
    int i;
    
    // 기본값 설정
    opts.lines_per_file = 1000;
    opts.prefix = "x";
    
    // 명령행 인수 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "split: -l 옵션에는 줄 수가 필요합니다\n");
                return 2;
            }
            opts.lines_per_file = atoi(argv[++i]);
            if (opts.lines_per_file <= 0) {
                fprintf(stderr, "split: 잘못된 줄 수: %s\n", argv[i]);
                return 2;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "split: 알 수 없는 옵션: %s\n", argv[i]);
            print_usage();
            return 2;
        } else {
            if (!input_file) {
                input_file = argv[i];
            } else {
                // 접두사 지정
                opts.prefix = argv[i];
            }
        }
    }
    
    // 입력 파일이 지정되지 않으면 표준 입력 사용
    if (!input_file) {
        input_file = "-";
    }
    
    return split_file(input_file, &opts);
} 