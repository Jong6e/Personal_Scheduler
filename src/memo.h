// src/memo.h

#ifndef MEMO_H
#define MEMO_H

#include <stdbool.h>

#define MAX_USER_ID_LEN 50        // 최대 사용자 ID 길이
#define MAX_MEMO_TITLE_LEN 100    // 최대 메모 제목 길이
#define MAX_MEMO_CONTENT_LEN 1024 // 최대 메모 내용 길이
#define MAX_DATETIME_LEN 20       // 최대 날짜/시간 문자열 길이

// 메모 정보를 담는 구조체
typedef struct
{
    int id;                             // 각 메모의 고유 ID (전체 메모에서 고유)
    char user_id[MAX_USER_ID_LEN];      // 이 메모를 소유한 사용자 ID
    char title[MAX_MEMO_TITLE_LEN];     // 메모 제목
    char content[MAX_MEMO_CONTENT_LEN]; // 메모 내용
    char created_at[MAX_DATETIME_LEN];  // 생성 날짜/시간
    char updated_at[MAX_DATETIME_LEN];  // 수정 날짜/시간
} Memo;

// 메모를 연결 리스트로 관리하기 위한 노드 구조체
typedef struct MemoNode
{
    Memo memo;             // 메모 정보
    struct MemoNode *next; // 다음 메모 노드 포인터
} MemoNode;

void memo_init();    // 메모 초기화
void memo_cleanup(); // 메모 정리

bool memo_list_for_user(const char *user_id, char *output, int output_size);                                  // 사용자별 메모 목록 출력
bool memo_get_by_id(int memo_id, const char *user_id, char *output, int output_size);                         // 메모 조회
bool memo_add(const char *user_id, const char *title, const char *content);                                   // 메모 추가
bool memo_update(int memo_id, const char *user_id, const char *new_content);                                  // 메모 수정
bool memo_delete(int memo_id, const char *user_id);                                                           // 메모 삭제
bool memo_delete_by_user_id(const char *user_id);                                                             // 회원 탈퇴 시, 해당 사용자의 모든 메모 데이터를 삭제
bool memo_list_by_month(const char *user_id, int year, int month, char *output, int output_size);             // 월별 메모 목록 출력
bool memo_search(const char *user_id, const char *field, const char *keyword, char *output, int output_size); // 메모 검색
void memo_save_all_to_files();                                                                                // 모든 메모를 파일에 저장

// ID로 메모리에서 직접 메모 구조체 포인터를 찾는 함수
const Memo *memo_get_by_id_internal(int memo_id, const char *user_id);

// 특정 사용자의 모든 메모를 배열 형태로 가져오는 함수
int memo_get_all_for_user_internal(const char *user_id, Memo *memo_array, int max_count);

#endif
