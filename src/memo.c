// src/memo.c

#include "memo.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// 전역 변수
static MemoNode *g_memo_list_head = NULL;
static int g_next_memo_id = 1;
const char *DATA_DIR = "data/memo/";

// 현재 날짜와 시간 가져오기
static void get_current_datetime(char *datetime_str, int size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(datetime_str, size, "%Y-%m-%d %H:%M:%S", t);
}

// 메모 노드 생성
static MemoNode *create_memo_node(const Memo *memo_data)
{
    MemoNode *new_node = (MemoNode *)malloc(sizeof(MemoNode));
    if (!new_node)
    {
        perror("Failed to allocate memory for new memo node");
        return NULL;
    }
    new_node->memo = *memo_data;
    new_node->next = NULL;
    return new_node;
}

// 메모 노드 추가
static void append_memo_node(MemoNode *node_to_add)
{
    if (!g_memo_list_head)
    {
        g_memo_list_head = node_to_add;
    }
    else
    {
        MemoNode *current = g_memo_list_head;
        while (current->next)
        {
            current = current->next;
        }
        current->next = node_to_add;
    }
}

// 메모 노드 찾기
static MemoNode *find_memo_node(int memo_id, const char *user_id)
{
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        if (current->memo.id == memo_id && strcmp(current->memo.user_id, user_id) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// UTF-8 소문자 변환
static void to_lowercase_utf8(const char *src, char *dest, size_t dest_size)
{
    strncpy(dest, src, dest_size);
    if (dest_size > 0)
    {
        dest[dest_size - 1] = '\0';
    }
    for (int i = 0; dest[i] != '\0'; i++)
    {
        dest[i] = tolower((unsigned char)dest[i]);
    }
}

// 메모 초기화
void memo_init()
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s*_memos.txt", DATA_DIR);
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("No memo files found. Initializing empty memo list.\n");
        return;
    }
    do
    {
        // 디렉토리가 아닌 경우에만 처리
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // 파일 이름에서 사용자 ID 추출
            char user_id[MAX_USER_ID_LEN] = {0};
            char *suffix = "_memos.txt";
            char *suffix_ptr = strstr(findFileData.cFileName, suffix);
            if (suffix_ptr)
            {
                size_t user_id_len = suffix_ptr - findFileData.cFileName;
                strncpy(user_id, findFileData.cFileName, user_id_len);
                user_id[user_id_len] = '\0';
            }
            else
            {
                continue;
            }
            // 파일 경로 생성
            char full_path[MAX_PATH];
            snprintf(full_path, MAX_PATH, "%s%s", DATA_DIR, findFileData.cFileName);
            FILE *file = fopen(full_path, "r");
            if (!file)
                continue;
            // 파일에서 메모 데이터 읽기
            Memo memo;
            while (fscanf(file, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]\n", &memo.id, memo.created_at, memo.updated_at, memo.title, memo.content) == 5)
            {
                strncpy(memo.user_id, user_id, MAX_USER_ID_LEN - 1);
                memo.user_id[MAX_USER_ID_LEN - 1] = '\0';
                // 메모 노드 생성
                MemoNode *new_node = create_memo_node(&memo);
                if (new_node)
                {
                    append_memo_node(new_node);
                    // 다음 메모 ID 업데이트
                    if (memo.id >= g_next_memo_id)
                    {
                        g_next_memo_id = memo.id + 1;
                    }
                }
            }
            fclose(file);
        }
        // 다음 파일 찾기
    } while (FindNextFile(hFind, &findFileData) != 0);
    // 파일 핸들 닫기
    FindClose(hFind);
    printf("Memo data loaded. Next memo ID is %d\n", g_next_memo_id);
}

// 메모 정리
void memo_cleanup()
{
    // 메모 노드 정리
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        // 다음 노드 저장
        MemoNode *next = current->next;
        free(current);
        current = next;
    }
    // 메모 노드 헤드 초기화
    g_memo_list_head = NULL;
}

// 모든 메모를 파일에 저장
void memo_save_all_to_files()
{
    // 사용자별 메모 목록 생성
    char users[100][MAX_USER_ID_LEN];
    int user_count = 0;
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        // 이미 처리한 사용자인지 확인
        bool found = false;
        for (int i = 0; i < user_count; i++)
        {
            if (strcmp(users[i], current->memo.user_id) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found && user_count < 100)
        {
            // 새로운 사용자 추가
            strncpy(users[user_count], current->memo.user_id, MAX_USER_ID_LEN - 1);
            users[user_count][MAX_USER_ID_LEN - 1] = '\0';
            user_count++;
        }
        current = current->next;
    }
    // 각 사용자별 메모 목록 저장
    for (int i = 0; i < user_count; i++)
    {
        char filepath[MAX_PATH];
        // 파일 경로 생성
        snprintf(filepath, MAX_PATH, "%s%s_memos.txt", DATA_DIR, users[i]);
        // 파일 열기
        FILE *file = fopen(filepath, "w");
        if (!file)
            continue;
        // 메모 목록 저장
        current = g_memo_list_head;
        while (current)
        {
            if (strcmp(current->memo.user_id, users[i]) == 0)
            {
                fprintf(file, "%d\t%s\t%s\t%s\t%s\n", current->memo.id, current->memo.created_at, current->memo.updated_at, current->memo.title, current->memo.content);
            }
            current = current->next;
        }
        fclose(file);
    }
}

// 메모 추가
bool memo_add(const char *user_id, const char *title, const char *content)
{
    // 새 메모 노드 생성
    Memo new_memo;
    // 새 메모 ID 생성
    new_memo.id = g_next_memo_id++;
    // 사용자 ID 복사
    strncpy(new_memo.user_id, user_id, MAX_USER_ID_LEN);
    new_memo.user_id[MAX_USER_ID_LEN - 1] = '\0';
    // 제목 복사
    strncpy(new_memo.title, title, MAX_MEMO_TITLE_LEN);
    new_memo.title[MAX_MEMO_TITLE_LEN - 1] = '\0';
    strncpy(new_memo.content, content, MAX_MEMO_CONTENT_LEN);
    new_memo.content[MAX_MEMO_CONTENT_LEN - 1] = '\0';
    // 현재 날짜와 시간 설정
    get_current_datetime(new_memo.created_at, MAX_DATETIME_LEN);
    strcpy(new_memo.updated_at, new_memo.created_at);
    // 메모 노드 생성
    MemoNode *new_node = create_memo_node(&new_memo);
    if (!new_node)
        return false;
    // 메모 노드 추가
    append_memo_node(new_node);
    // 모든 메모를 파일에 저장
    memo_save_all_to_files(); // 즉시 저장
    return true;
}

// 메모 삭제
bool memo_delete(int memo_id, const char *user_id)
{
    // 메모 노드 탐색
    MemoNode *current = g_memo_list_head;
    MemoNode *prev = NULL;
    // 메모 노드 삭제
    while (current)
    {
        if (current->memo.id == memo_id && strcmp(current->memo.user_id, user_id) == 0)
        {
            // 이전 노드가 있는 경우
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                g_memo_list_head = current->next;
            }
            free(current);
            memo_save_all_to_files(); // 즉시 저장
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

// 회원 탈퇴 시, 해당 사용자의 모든 메모 데이터를 삭제
bool memo_delete_by_user_id(const char *user_id)
{
    MemoNode *current = g_memo_list_head;
    MemoNode *prev = NULL;
    bool changed = false;
    while (current != NULL)
    {
        // 사용자 ID 비교
        if (strcmp(current->memo.user_id, user_id) == 0)
        {
            // 삭제할 노드 저장
            MemoNode *node_to_delete = current;
            // 이전 노드가 없는 경우
            if (prev == NULL)
            {
                g_memo_list_head = current->next;
            }
            // 이전 노드가 있는 경우
            else
            {
                prev->next = current->next;
            }
            // 현재 노드 이동
            current = current->next;
            // 노드 메모리 해제
            free(node_to_delete);
            // 변경 표시
            changed = true;
        }
        else
        {
            // 이전 노드 업데이트
            prev = current;
            current = current->next;
        }
    }
    // 파일 경로 생성
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s%s_memos.txt", DATA_DIR, user_id);
    // 파일 삭제
    if (remove(filepath) == 0)
    {
        printf("[정보] %s 파일이 성공적으로 삭제되었습니다.\n", filepath);
    }
    else
    {
        printf("[정보] %s 파일을 찾을 수 없거나 삭제할 수 없습니다.\n", filepath);
    }
    return changed;
}

// 메모 수정
bool memo_update(int memo_id, const char *user_id, const char *new_content)
{
    MemoNode *node_to_update = find_memo_node(memo_id, user_id);
    // 메모 노드 업데이트
    if (node_to_update)
    {
        // 내용 복사
        strncpy(node_to_update->memo.content, new_content, MAX_MEMO_CONTENT_LEN);
        node_to_update->memo.content[MAX_MEMO_CONTENT_LEN - 1] = '\0';
        // 수정 시간 업데이트
        get_current_datetime(node_to_update->memo.updated_at, MAX_DATETIME_LEN);
        memo_save_all_to_files(); // 즉시 저장
        return true;
    }
    return false;
}

// 사용자의 메모 목록 출력
bool memo_list_for_user(const char *user_id, char *output, int output_size)
{
    int offset = 0;
    bool found = false;
    offset += snprintf(output + offset, output_size - offset, "OK:[%s님의 메모 목록]\n", user_id);
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        // 사용자 ID 비교
        if (strcmp(current->memo.user_id, user_id) == 0)
        {
            offset += snprintf(output + offset, output_size - offset, "  - [%d] %s\n", current->memo.id, current->memo.title);
            found = true;
        }
        current = current->next;
    }
    if (!found)
    {
        snprintf(output, output_size, "OK:작성된 메모가 없습니다.");
    }
    return true;
}

// 메모 조회
bool memo_get_by_id(int memo_id, const char *user_id, char *output, int output_size)
{
    MemoNode *node = find_memo_node(memo_id, user_id);
    if (node)
    {
        Memo *m = &node->memo;
        snprintf(output, output_size, "OK:%d\t%s\t%s\t%s\t%s", m->id, m->created_at, m->updated_at, m->title, m->content);
        return true;
    }
    snprintf(output, output_size, "FAIL:메모 ID %d를 찾을 수 없습니다.", memo_id);
    return false;
}

// 월별 메모 목록 출력
bool memo_list_by_month(const char *user_id, int year, int month, char *output, int output_size)
{
    output[0] = '\0';
    int offset = 0;
    bool found = false;
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        // 사용자 ID 비교
        if (strcmp(current->memo.user_id, user_id) == 0)
        {
            // 메모 날짜 분리
            int memo_year, memo_month;
            if (sscanf(current->memo.created_at, "%d-%d", &memo_year, &memo_month) == 2)
            {
                // 년도와 월 비교
                if (memo_year == year && memo_month == month)
                {
                    // 메모 출력
                    offset += snprintf(output + offset, output_size - offset, "%d\t%s\t%s\t%s\n", current->memo.id, current->memo.created_at, current->memo.updated_at, current->memo.title);
                    found = true;
                }
            }
        }
        current = current->next;
    }
    if (!found)
    {
        // 메모 없음
        snprintf(output, output_size, "OK");
    }
    return true;
}

// 메모 검색
bool memo_search(const char *user_id, const char *field, const char *keyword, char *output, int output_size)
{
    // 출력 초기화
    output[0] = '\0';
    int offset = 0;
    // 검색 결과 표시
    bool found = false;
    // 키워드 소문자 변환
    char lower_keyword[MAX_MEMO_CONTENT_LEN];
    to_lowercase_utf8(keyword, lower_keyword, sizeof(lower_keyword));
    // 키워드가 없으면 종료
    if (strlen(lower_keyword) == 0)
    {
        // 검색 결과 없음
        snprintf(output, output_size, "OK");
        return true;
    }
    // 메모 노드 탐색
    MemoNode *current = g_memo_list_head;
    while (current)
    {
        // 사용자 ID 비교
        if (strcmp(current->memo.user_id, user_id) == 0)
        {
            // 제목 또는 내용 검색
            bool match = false;
            char lower_buffer[MAX_MEMO_CONTENT_LEN];
            // 제목 검색
            if (strcmp(field, "title") == 0 || strcmp(field, "all") == 0)
            {
                to_lowercase_utf8(current->memo.title, lower_buffer, sizeof(lower_buffer));
                if (strstr(lower_buffer, lower_keyword))
                {
                    match = true;
                }
            }
            // 내용 검색
            if (!match && (strcmp(field, "content") == 0 || strcmp(field, "all") == 0))
            {
                to_lowercase_utf8(current->memo.content, lower_buffer, sizeof(lower_buffer));
                if (strstr(lower_buffer, lower_keyword))
                {
                    match = true;
                }
            }
            if (match)
            {
                // 메모 출력
                offset += snprintf(output + offset, output_size - offset, "%d\t%s\t%s\t%s\n", current->memo.id, current->memo.created_at, current->memo.updated_at, current->memo.title);
                found = true;
            }
        }
        current = current->next;
    }
    if (!found)
    {
        // 검색 결과 없음
        snprintf(output, output_size, "OK");
    }
    return true;
}
