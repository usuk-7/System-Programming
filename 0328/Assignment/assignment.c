#include <stdio.h>

int main() {
    char a;

    while (1) {
        printf("문자 입력 : ");
        scanf(" %c", &a);

        if (a == '0') break;
        else if (a >= 'A' && a <= 'Z') {
            printf("%c의 소문자는 %c입니다.\n", a, a + 32);
        }
        else if (a >= 'a' && a <= 'z') {
            printf("%c의 대문자는 %c입니다.\n", a, a - 32);
        }
    }
    return 0;
}
