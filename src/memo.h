// src/memo.h

#ifndef MEMO_H
#define MEMO_H

#include <stdbool.h>

#define USER_ID_MAX 32       // 사용자 ID 최대 길이
#define MEMO_TITLE_MAX 40    // 제목 최대 길이 (본문 앞 10글자 + ... 고려)
#define MEMO_CONTENT_MAX 500 // 본문 최대 길이
#define DATETIME_MAX 20      // "YYYY-MM-DD HH:MM:SS" 형식

// 메모 정보를 담는 구조체 (R011)
typedef struct
{
    int id;
    char title[MEMO_TITLE_MAX];
    char content[MEMO_CONTENT_MAX];
    char created_at[DATETIME_MAX];
    char updated_at[DATETIME_MAX];
} Memo;

// 특정 사용자의 모든 메모 목록을 문자열 형태로 가져옵니다.
bool list_memos_for_user(const char *user_id, char *output, int output_size);

// 특정 사용자의 단일 메모를 상세 조회합니다.
bool get_memo_by_id(const char *user_id, int memo_id, char *output, int output_size);

// 신규 메모를 추가합니다.
bool add_memo_for_user(const char *user_id, const char *title, const char *content);

// 특정 메모를 수정합니다.
bool update_memo_for_user(const char *user_id, int memo_id, const char *new_content);

// 특정 메모를 삭제합니다.
bool delete_memo_for_user(const char *user_id, int memo_id);

// 특정 메모의 원본 내용을 가져옵니다. (수정용)
bool get_raw_memo_content(const char *user_id, int memo_id, char *content_output, int content_size);

#endif // MEMO_H
