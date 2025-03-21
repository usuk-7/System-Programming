#include <stdio.h>

int main() {
    char name[50];
    int student_id;

    printf("학번을 입력하세요: ");
    scanf("%d", &student_id);
    printf("이름을 입력하세요: ");
    scanf("%s", name);
    printf("학번: %d\n", student_id);
    printf("이름: %s\n", name);
    
    return 0;
}
