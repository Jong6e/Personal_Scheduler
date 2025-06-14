#ifndef USER_H
#define USER_H

#include <stdbool.h>

#define MAX_USERS 100              // 최대 사용자 수
#define USER_ID_MAX 32             // ID 최대 길이
#define USER_PW_MAX 20             // PW 최대 길이
#define USER_FILE "data/users.txt" // 사용자 정보 파일

// 사용자 정보를 담는 구조체
typedef struct
{
    char id[USER_ID_MAX];
    char pw[USER_PW_MAX];
} User;

// --- 함수 선언 ---

// 파일에서 모든 사용자 정보를 읽어와 전역 배열에 로드합니다.
void load_users_from_file();

// 전역 배열의 모든 사용자 정보를 파일에 저장합니다.
void save_users_to_file();

// 로그인 정보를 확인합니다.
// 성공 시 true, 실패 시 false를 반환합니다.
bool check_login(const char *id, const char *pw);

// 신규 사용자를 등록합니다.
// 성공 시 true, 아이디 중복 시 false를 반환합니다.
bool register_user(const char *id, const char *pw);

// 등록된 사용자를 삭제합니다.
// 성공 시 true, 실패 시 false를 반환합니다.
bool delete_user(const char *id);

// 사용자의 비밀번호를 수정합니다.
// 성공 시 true, 실패 시 false를 반환합니다.
bool update_user_password(const char *id, const char *old_pw, const char *new_pw);

#endif // USER_H
