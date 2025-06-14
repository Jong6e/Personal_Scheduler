#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// --- 전역 변수 정의 ---
User g_users[MAX_USERS];
int g_user_count = 0;

// --- 내부 사용 함수 ---

// ✅ ID로 사용자를 찾아 인덱스를 반환, 없으면 -1 반환
static int find_user_index(const char *id)
{
    for (int i = 0; i < g_user_count; i++)
    {
        if (strcmp(g_users[i].id, id) == 0)
        {
            return i;
        }
    }
    return -1;
}

// --- 외부 공개 함수 ---

// ✅ 파일에서 모든 사용자 정보를 읽어와 g_users 배열에 로드
void load_users_from_file()
{
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp)
    {
        // 파일이 없으면 그냥 넘어감. 나중에 첫 등록 시 생성됨.
        return;
    }

    while (g_user_count < MAX_USERS &&
           fscanf(fp, "%31[^:]:%31[^\n]\n", g_users[g_user_count].id, g_users[g_user_count].pw) == 2)
    {
        g_user_count++;
    }
    fclose(fp);
    printf("[서버] %d명의 사용자 정보를 로드했습니다.\n", g_user_count);
}

// ✅ g_users 배열의 모든 사용자 정보를 파일에 저장
bool save_users_to_file()
{
    FILE *fp = fopen(USER_FILE, "w");
    if (!fp)
    {
        perror("[서버] 사용자 파일 저장 실패");
        return false;
    }
    for (int i = 0; i < g_user_count; i++)
    {
        fprintf(fp, "%s:%s\n", g_users[i].id, g_users[i].pw);
    }
    fclose(fp);
    return true;
}

// ✅ 로그인 정보 확인
bool check_login(const char *id, const char *pw)
{
    int index = find_user_index(id);
    if (index != -1 && strcmp(g_users[index].pw, pw) == 0)
    {
        return true;
    }
    return false;
}

// ✅ 신규 사용자 등록
bool register_user(const char *id, const char *pw)
{
    if (g_user_count >= MAX_USERS || find_user_index(id) != -1)
    {
        return false; // 사용자 수 초과 또는 ID 중복
    }
    strncpy(g_users[g_user_count].id, id, USER_ID_MAX);
    strncpy(g_users[g_user_count].pw, pw, USER_PW_MAX);
    g_user_count++;
    return save_users_to_file();
}

// ✅ 사용자 삭제
bool delete_user(const char *id, const char *pw)
{
    int index = find_user_index(id);
    if (index == -1 || strcmp(g_users[index].pw, pw) != 0)
    {
        return false; // 사용자 없거나 PW 불일치
    }

    // 배열 중간의 요소를 삭제하고 뒤의 요소들을 한 칸씩 당김
    for (int i = index; i < g_user_count - 1; i++)
    {
        g_users[i] = g_users[i + 1];
    }
    g_user_count--;
    return save_users_to_file();
}

// ✅ 사용자 비밀번호 수정
bool update_password(const char *id, const char *old_pw, const char *new_pw)
{
    int index = find_user_index(id);
    if (index == -1 || strcmp(g_users[index].pw, old_pw) != 0)
    {
        return false; // 사용자 없거나 기존 PW 불일치
    }
    strncpy(g_users[index].pw, new_pw, USER_PW_MAX);
    return save_users_to_file();
}
