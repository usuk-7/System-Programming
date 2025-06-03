#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

/*
 * Linux whoami 명령어 구현 (간단 버전)
 * 
 * 기능:
 * - 현재 실행 중인 프로세스의 유효 사용자 이름을 출력
 */

int main() {
    // 현재 프로세스의 유효 사용자 ID(UID) 가져오기
    uid_t uid = geteuid();
    
    // UID를 사용하여 사용자 정보 구조체 가져오기
    // getpwuid() 함수는 UID에 해당하는 passwd 구조체를 반환
    struct passwd *user_info = getpwuid(uid);
    
    // 사용자 정보를 가져오는데 실패한 경우 에러 처리
    if (user_info == NULL) {
        perror("사용자 정보를 가져올 수 없습니다");
        return 1;
    }
    
    // passwd 구조체의 pw_name 필드에서 사용자 이름 출력
    // pw_name: 사용자의 로그인 이름 (예: "stud", "root" 등)
    printf("%s\n", user_info->pw_name);
    
    return 0;
}
