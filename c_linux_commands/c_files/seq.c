#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int start, end, i;
    
    if (argc == 2) {
        start = 1;
        end = atoi(argv[1]);
    } else if (argc == 3) {
        start = atoi(argv[1]);
        end = atoi(argv[2]);
    } else {
        fprintf(stderr, "seq: 잘못된 인수 개수\n");
        return 1;
    }
    
    if (start <= end) {
        for (i = start; i <= end; i++) {
            printf("%d\n", i);
        }
    } else {
        for (i = start; i >= end; i--) {
            printf("%d\n", i);
        }
    }
    
    return 0;
} 