#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compare_files(const char* file1_name, const char* file2_name) {
    FILE* file1;
    FILE* file2;
    int ch1, ch2;
    long byte_pos = 1;
    long line_num = 1;
    int result = 0;
    
    // 첫 번째 파일 열기
    if (strcmp(file1_name, "-") == 0) {
        file1 = stdin;
    } else {
        file1 = fopen(file1_name, "rb");
        if (!file1) {
            fprintf(stderr, "cmp: %s: 파일을 열 수 없습니다\n", file1_name);
            return 2;
        }
    }
    
    // 두 번째 파일 열기
    if (strcmp(file2_name, "-") == 0) {
        file2 = stdin;
    } else {
        file2 = fopen(file2_name, "rb");
        if (!file2) {
            fprintf(stderr, "cmp: %s: 파일을 열 수 없습니다\n", file2_name);
            if (file1 != stdin) fclose(file1);
            return 2;
        }
    }
    
    // 표준입력을 두 번 사용할 수 없음
    if (file1 == stdin && file2 == stdin) {
        fprintf(stderr, "cmp: 표준입력을 두 번 사용할 수 없습니다\n");
        return 2;
    }
    
    // 바이트 단위로 비교
    while (1) {
        ch1 = fgetc(file1);
        ch2 = fgetc(file2);
        
        // 두 파일 모두 끝에 도달
        if (ch1 == EOF && ch2 == EOF) {
            break;
        }
        
        // 한 파일만 끝에 도달 (길이가 다름)
        if (ch1 == EOF) {
            printf("cmp: EOF on %s\n", file1_name);
            result = 1;
            break;
        }
        
        if (ch2 == EOF) {
            printf("cmp: EOF on %s\n", file2_name);
            result = 1;
            break;
        }
        
        // 바이트가 다름
        if (ch1 != ch2) {
            printf("%s %s differ: byte %ld, line %ld\n", 
                   file1_name, file2_name, byte_pos, line_num);
            result = 1;
            break;
        }
        
        // 줄 번호 증가
        if (ch1 == '\n') {
            line_num++;
        }
        
        byte_pos++;
    }
    
    // 파일 닫기
    if (file1 != stdin) {
        fclose(file1);
    }
    if (file2 != stdin) {
        fclose(file2);
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "사용법: cmp 파일1 파일2\n");
        return 2;
    }
    
    return compare_files(argv[1], argv[2]);
} 