#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// --- 전역 변수 정의 ---
static User g_users[MAX_USERS];
static int g_user_count = 0;

// ID로 사용자를 찾아 인덱스를 반환, 없으면 -1 반환
static int find_user_by_id(const char *id)
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

// 파일에서 모든 사용자 정보를 읽어와 g_users 배열에 로드
void load_users_from_file()
{
    FILE *file = fopen(USER_FILE, "r");
    if (!file)
    {
        printf("[서버] users.txt 파일이 없어 새로 시작합니다.\n");
        return;
    }

    g_user_count = 0; // 배열 초기화
    while (g_user_count < MAX_USERS &&
           fscanf(file, "%31[^:]:%19[^\n]\n", g_users[g_user_count].id, g_users[g_user_count].pw) == 2)
    {
        g_user_count++;
    }
    fclose(file);
    printf("[서버] %d명의 사용자 정보를 로드했습니다.\n", g_user_count);
}

// g_users 배열의 모든 사용자 정보를 파일에 저장
void save_users_to_file()
{
    FILE *file = fopen(USER_FILE, "w");
    if (!file)
    {
        perror("[서버] 사용자 파일 저장 실패");
        return;
    }
    for (int i = 0; i < g_user_count; i++)
    {
        fprintf(file, "%s:%s\n", g_users[i].id, g_users[i].pw);
    }
    fclose(file);
}

// 로그인 정보 확인
bool check_login(const char *id, const char *pw)
{
    int user_idx = find_user_by_id(id);
    if (user_idx != -1 && strcmp(g_users[user_idx].pw, pw) == 0)
    {
        return true;
    }
    return false;
}

// 신규 사용자 등록
bool register_user(const char *id, const char *pw)
{
    if (g_user_count >= MAX_USERS || find_user_by_id(id) != -1)
    {
        return false; // 사용자 수 초과 또는 ID 중복
    }
    strncpy(g_users[g_user_count].id, id, USER_ID_MAX - 1);
    g_users[g_user_count].id[USER_ID_MAX - 1] = '\0';

    strncpy(g_users[g_user_count].pw, pw, USER_PW_MAX - 1);
    g_users[g_user_count].pw[USER_PW_MAX - 1] = '\0';

    g_user_count++;
    save_users_to_file();
    return true;
}

// 사용자 삭제
bool delete_user(const char *id)
{
    int user_idx = find_user_by_id(id);
    if (user_idx == -1)
    {
        return false; // 사용자가 존재하지 않음
    }

    // 연관된 메모 파일 삭제 (R002, R007)
    char memo_filename[100];
    sprintf(memo_filename, "data/%s_memos.txt", id);
    if (remove(memo_filename) == 0)
    {
        printf("[서버] %s 파일이 삭제되었습니다.\n", memo_filename);
    }
    else
    {
        // 파일이 없거나 삭제할 수 없는 경우, 오류 메시지를 출력할 수 있지만 필수는 아님
        printf("[서버] %s 파일을 찾을 수 없거나 삭제할 수 없습니다.\n", memo_filename);
    }

    // 배열의 뒷부분을 앞으로 한 칸씩 당겨서 덮어쓰기
    for (int i = user_idx; i < g_user_count - 1; i++)
    {
        g_users[i] = g_users[i + 1];
    }
    g_user_count--;

    save_users_to_file();
    return true;
}

// 사용자 비밀번호 수정
bool update_user_password(const char *id, const char *old_pw, const char *new_pw)
{
    // 1. 사용자가 존재하는지, 그리고 기존 비밀번호가 맞는지 확인
    if (!check_login(id, old_pw))
    {
        return false; // 사용자가 없거나 기존 비밀번호가 틀림
    }

    // 2. 새 비밀번호로 교체
    int user_idx = find_user_by_id(id);
    if (user_idx == -1)
    {
        return false; // 사용자가 존재하지 않음
    }

    strncpy(g_users[user_idx].pw, new_pw, USER_PW_MAX - 1);
    g_users[user_idx].pw[USER_PW_MAX - 1] = '\0';

    save_users_to_file();
    return true;
}
