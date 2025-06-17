// src/user.h

#ifndef USER_H
#define USER_H

#include <stdbool.h>

// 사용자 ID/PW 길이 제한
#define MIN_ID_LEN 1
#define MAX_ID_LEN 20
#define MIN_PW_LEN 4
#define MAX_PW_LEN 20
#define MAX_USER 100

// 사용자 정보를 담는 구조체
typedef struct
{
    char id[MAX_ID_LEN];
    char pw[MAX_PW_LEN];
} User;

// 사용자를 연결 리스트로 관리하기 위한 노드 구조체
typedef struct UserNode
{
    User user;
    struct UserNode *next;
} UserNode;

// 함수 선언
void user_init();    // 사용자 목록 초기화
void user_cleanup(); // 동적 할당된 메모리 해제
// 사용자 추가
bool user_add(const char *id, const char *pw);                 // ID로 사용자 찾기
User *user_find_by_id(const char *id);                         // ID로 사용자 삭제
bool user_delete_by_id(const char *id);                        // 비밀번호 변경
bool user_update_password(const char *id, const char *new_pw); // 파일에 사용자 목록 저장
void user_save_to_file();                                      // 파일에 사용자 목록 저장

#endif
