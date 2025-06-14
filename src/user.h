#ifndef USER_H
#define USER_H

#include <stdbool.h>

#define MAX_USERS 100              // 최대 사용자 수
#define USER_ID_MAX 32             // ID 최대 길이
#define USER_PW_MAX 32             // PW 최대 길이
#define USER_FILE "data/users.txt" // 사용자 정보 파일

// ✅ 사용자 정보를 담는 구조체
typedef struct
{
    char id[USER_ID_MAX];
    char pw[USER_PW_MAX];
} User;

// --- 전역 변수 선언 (user.c에서 정의) ---
extern User g_users[MAX_USERS]; // 모든 사용자 정보를 저장하는 전역 배열
extern int g_user_count;        // 현재 등록된 사용자 수

// --- 함수 선언 ---

// ✅ 파일에서 모든 사용자 정보를 읽어와 g_users 배열에 로드합니다.
void load_users_from_file();

// ✅ g_users 배열의 모든 사용자 정보를 파일에 저장합니다.
bool save_users_to_file();

// ✅ 로그인 정보를 확인합니다.
// - 성공 시 true, 실패 시 false 반환
bool check_login(const char *id, const char *pw);

// ✅ 신규 사용자를 등록합니다.
// - 성공 시 true, ID 중복 시 false 반환
bool register_user(const char *id, const char *pw);

// ✅ 사용자를 삭제합니다.
// - 성공 시 true, 사용자 없거나 PW 불일치 시 false 반환
bool delete_user(const char *id, const char *pw);

// ✅ 사용자의 비밀번호를 수정합니다.
// - 성공 시 true, 사용자 없거나 기존 PW 불일치 시 false 반환
bool update_password(const char *id, const char *old_pw, const char *new_pw);

#endif // USER_H
