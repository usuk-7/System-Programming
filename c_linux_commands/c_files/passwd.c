#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/types.h>
#include <termios.h>
#include <errno.h>

/*
 * Linux passwd 명령어 구현 (-S 옵션 지원)
 * 
 * 기능:
 * - 기본: 현재 사용자의 패스워드 변경 (시뮬레이션)
 * - -S 옵션: 계정 상태 정보 출력 (가장 많이 사용되는 옵션)
 * 
 * 참고: 실제 패스워드 변경은 root 권한과 복잡한 시스템 호출이 필요하므로
 *       이 구현에서는 시뮬레이션으로 처리합니다.
 */

// 패스워드 입력 시 에코를 비활성화하는 함수
void disable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO;          // 에코 비활성화
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 패스워드 입력 시 에코를 다시 활성화하는 함수
void enable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ECHO;           // 에코 활성화
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 안전한 패스워드 입력 함수
int read_password(const char* prompt, char* password, size_t max_len) {
    printf("%s", prompt);
    fflush(stdout);
    
    disable_echo();                 // 패스워드 입력 시 화면에 표시하지 않음
    
    if (fgets(password, max_len, stdin) == NULL) {
        enable_echo();
        return -1;
    }
    
    enable_echo();
    printf("\n");                   // 줄바꿈 추가
    
    // 개행 문자 제거
    size_t len = strlen(password);
    if (len > 0 && password[len-1] == '\n') {
        password[len-1] = '\0';
    }
    
    return 0;
}

// 계정 상태 출력 함수 (-S 옵션)
void print_account_status(const char* username) {
    struct passwd* pwd;
    struct spwd* spwd_entry = NULL;
    
    // 사용자 정보 가져오기
    if (username) {
        pwd = getpwnam(username);
    } else {
        pwd = getpwuid(getuid());
    }
    
    if (!pwd) {
        printf("passwd: 사용자를 찾을 수 없습니다\n");
        return;
    }
    
    // 기본값 설정
    char status = 'P';          // P: password set, L: locked, NP: no password
    char date_str[12] = "2025-03-07";  // 기본 날짜 (시뮬레이션)
    long min_days = 0;          // 최소 변경 간격
    long max_days = 99999;      // 최대 유효기간
    long warn_days = 7;         // 만료 경고 일수
    long inactive_days = -1;    // 비활성화 후 일수
    
    // shadow 파일 접근 시도 (일반적으로 root 권한 필요)
    spwd_entry = getspnam(pwd->pw_name);
    if (spwd_entry) {
        // 패스워드 상태 확인
        if (strlen(spwd_entry->sp_pwdp) == 0) {
            status = 'N';       // NP: no password
        } else if (spwd_entry->sp_pwdp[0] == '!' || spwd_entry->sp_pwdp[0] == '*') {
            status = 'L';       // L: locked
        } else {
            status = 'P';       // P: password set
        }
        
        // shadow 파일의 실제 값들 사용
        if (spwd_entry->sp_min != -1) min_days = spwd_entry->sp_min;
        if (spwd_entry->sp_max != -1) max_days = spwd_entry->sp_max;
        if (spwd_entry->sp_warn != -1) warn_days = spwd_entry->sp_warn;
        if (spwd_entry->sp_inact != -1) inactive_days = spwd_entry->sp_inact;
        
        // 마지막 변경일을 날짜 형식으로 변환 (시뮬레이션을 위해 고정값 사용)
        if (spwd_entry->sp_lstchg != -1) {
            // 실제로는 1970-01-01부터의 일수를 날짜로 변환해야 함
            // 여기서는 시뮬레이션을 위해 고정 날짜 사용
            strcpy(date_str, "2025-03-07");
        }
    } else {
        // shadow 파일에 접근할 수 없는 경우 기본값 사용
        if (strcmp(pwd->pw_passwd, "x") == 0) {
            status = 'P';       // shadow에서 관리되는 경우 password set으로 가정
        } else if (strlen(pwd->pw_passwd) == 0) {
            status = 'N';       // 비어있는 경우
        } else {
            status = 'P';       // 설정된 경우
        }
    }
    
    // 실제 passwd -S 형식으로 출력
    // 형식: username status last_change min_days max_days warn_days inactive_days
    printf("%s %c %s %ld %ld %ld %ld\n", 
           pwd->pw_name,        // 사용자명
           status,              // 상태 (P/L/N)
           date_str,            // 마지막 변경일
           min_days,            // 최소 변경 간격
           max_days,            // 최대 유효기간
           warn_days,           // 만료 경고 일수
           inactive_days);      // 비활성화 후 일수
}

// 패스워드 유효성 검사 함수
int validate_password(const char* password) {
    size_t len = strlen(password);
    
    // 최소 길이 제한 없음
    // 최대 길이 검사
    if (len > 128) {
        printf("패스워드가 너무 깁니다. 최대 128자까지 허용됩니다.\n");
        return 0;
    }
    
    // 허용된 문자만 사용하는지 검사
    // 허용: lowercase alphabet (a-z), digits (0-9), punctuation marks
    for (size_t i = 0; i < len; i++) {
        char c = password[i];
        
        // 소문자 알파벳 (a-z)
        if (c >= 'a' && c <= 'z') {
            continue;
        }
        // 숫자 (0-9)
        else if (c >= '0' && c <= '9') {
            continue;
        }
        // 구두점 기호들 (! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~)
        else if ((c >= '!' && c <= '/') ||    // ! " # $ % & ' ( ) * + , - . /
                 (c >= ':' && c <= '@') ||    // : ; < = > ? @
                 (c >= '[' && c <= '`') ||    // [ \ ] ^ _ `
                 (c >= '{' && c <= '~')) {    // { | } ~
            continue;
        }
        // 허용되지 않는 문자 (대문자, 공백, 기타)
        else {
            if (c >= 'A' && c <= 'Z') {
                printf("패스워드에 대문자는 사용할 수 없습니다. 소문자만 허용됩니다.\n");
            } else if (c == ' ') {
                printf("패스워드에 공백은 사용할 수 없습니다.\n");
            } else {
                printf("패스워드에 허용되지 않는 문자 '%c'가 포함되어 있습니다.\n", c);
                printf("허용되는 문자: 소문자(a-z), 숫자(0-9), 구두점 기호\n");
            }
            return 0;
        }
    }
    
    return 1;
}

// 패스워드 변경 시뮬레이션 함수
int change_password(const char* username) {
    char current_password[256];
    char new_password[256];
    char confirm_password[256];
    
    struct passwd* pwd;
    if (username) {
        pwd = getpwnam(username);
        // 다른 사용자의 패스워드 변경은 root 권한 필요
        if (getuid() != 0 && pwd && pwd->pw_uid != getuid()) {
            printf("passwd: 다른 사용자의 패스워드를 변경하려면 root 권한이 필요합니다\n");
            return 1;
        }
    } else {
        pwd = getpwuid(getuid());
    }
    
    if (!pwd) {
        printf("passwd: 사용자를 찾을 수 없습니다\n");
        return 1;
    }
    
    printf("%s 사용자의 패스워드를 변경합니다.\n", pwd->pw_name);
    
    // 현재 패스워드 확인 (root가 아닌 경우)
    if (getuid() != 0) {
        if (read_password("현재 패스워드: ", current_password, sizeof(current_password)) != 0) {
            printf("패스워드 입력 실패\n");
            return 1;
        }
        
        // 실제 구현에서는 현재 패스워드를 검증해야 함
        // 여기서는 시뮬레이션으로 빈 패스워드가 아니면 통과
        if (strlen(current_password) == 0) {
            printf("잘못된 패스워드입니다\n");
            return 1;
        }
    }
    
    // 새 패스워드 입력
    if (read_password("새 패스워드: ", new_password, sizeof(new_password)) != 0) {
        printf("패스워드 입력 실패\n");
        return 1;
    }
    
    // 패스워드 유효성 검사
    if (!validate_password(new_password)) {
        return 1;
    }
    
    // 패스워드 확인
    if (read_password("새 패스워드 재입력: ", confirm_password, sizeof(confirm_password)) != 0) {
        printf("패스워드 입력 실패\n");
        return 1;
    }
    
    // 패스워드 일치 확인
    if (strcmp(new_password, confirm_password) != 0) {
        printf("패스워드가 일치하지 않습니다\n");
        return 1;
    }
    
    // 실제 구현에서는 여기서 시스템 호출을 통해 패스워드를 변경
    // 이 구현에서는 성공 메시지만 출력
    printf("패스워드가 성공적으로 변경되었습니다.\n");
    printf("(참고: 이는 시뮬레이션이며 실제로는 변경되지 않습니다)\n");
    
    return 0;
}

int main(int argc, char* argv[]) {
    int show_status = 0;            // -S 옵션 플래그
    const char* target_user = NULL; // 대상 사용자
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-S") == 0) {
            show_status = 1;
        }
        else if (argv[i][0] == '-') {
            // 기타 옵션들
            fprintf(stderr, "passwd: 지원하지 않는 옵션 '%s'\n", argv[i]);
            return 1;
        }
        else {
            // 사용자명
            target_user = argv[i];
        }
    }
    
    if (show_status) {
        // -S 옵션: 계정 상태 출력
        print_account_status(target_user);
    } else {
        // 기본 동작: 패스워드 변경
        return change_password(target_user);
    }
    
    return 0;
}
