#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

void print_user_info(uid_t uid) {
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        printf("uid=%u(%s)", uid, pw->pw_name);
    } else {
        printf("uid=%u", uid);
    }
}

void print_group_info(gid_t gid) {
    struct group* gr = getgrgid(gid);
    if (gr) {
        printf("gid=%u(%s)", gid, gr->gr_name);
    } else {
        printf("gid=%u", gid);
    }
}

void print_groups_info() {
    gid_t groups[256];
    int ngroups = 256;
    
    if (getgroups(ngroups, groups) == -1) {
        perror("getgroups");
        return;
    }
    
    ngroups = getgroups(0, NULL);
    if (ngroups <= 0) {
        return;
    }
    
    if (getgroups(ngroups, groups) == -1) {
        perror("getgroups");
        return;
    }
    
    printf(" groups=");
    for (int i = 0; i < ngroups; i++) {
        if (i > 0) {
            printf(",");
        }
        
        struct group* gr = getgrgid(groups[i]);
        if (gr) {
            printf("%u(%s)", groups[i], gr->gr_name);
        } else {
            printf("%u", groups[i]);
        }
    }
}

void show_id_info(const char* username) {
    uid_t uid;
    gid_t gid;
    
    if (username) {
        struct passwd* pw = getpwnam(username);
        if (!pw) {
            fprintf(stderr, "id: '%s': 해당 사용자가 없습니다\n", username);
            return;
        }
        uid = pw->pw_uid;
        gid = pw->pw_gid;
    } else {
        uid = getuid();
        gid = getgid();
    }
    
    print_user_info(uid);
    printf(" ");
    print_group_info(gid);
    
    if (!username) {
        print_groups_info();
    } else {
        gid_t groups[256];
        int ngroups = 256;
        
        if (getgrouplist(username, gid, groups, &ngroups) != -1) {
            printf(" groups=");
            for (int i = 0; i < ngroups; i++) {
                if (i > 0) {
                    printf(",");
                }
                
                struct group* gr = getgrgid(groups[i]);
                if (gr) {
                    printf("%u(%s)", groups[i], gr->gr_name);
                } else {
                    printf("%u", groups[i]);
                }
            }
        }
    }
    
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        show_id_info(NULL);
    } else if (argc == 2) {
        show_id_info(argv[1]);
    } else {
        fprintf(stderr, "id: 사용법: id [사용자명]\n");
        return 1;
    }
    
    return 0;
} 