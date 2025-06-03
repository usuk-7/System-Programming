#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    FILE** files;
    int file_count = argc - 1;
    char line[4096];
    int i;
    
    if (file_count > 0) {
        files = malloc(file_count * sizeof(FILE*));
        if (!files) {
            fprintf(stderr, "tee: 메모리 할당 실패\n");
            return 1;
        }
        
        for (i = 0; i < file_count; i++) {
            files[i] = fopen(argv[i + 1], "w");
            if (!files[i]) {
                fprintf(stderr, "tee: %s: 파일을 열 수 없습니다\n", argv[i + 1]);
                for (int j = 0; j < i; j++) {
                    fclose(files[j]);
                }
                free(files);
                return 1;
            }
        }
    }
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        printf("%s", line);
        
        for (i = 0; i < file_count; i++) {
            fputs(line, files[i]);
        }
    }
    
    for (i = 0; i < file_count; i++) {
        fclose(files[i]);
    }
    
    if (file_count > 0) {
        free(files);
    }
    
    return 0;
} 