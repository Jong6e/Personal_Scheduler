// src/user.c

#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define USERS_FILE "data/users.txt"

// 연결 리스트의 헤드 포인터
static UserNode *user_list_head = NULL;

// 새로운 UserNode를 생성하고 초기화
UserNode *create_user_node(const char *id, const char *pw)
{
    // 사용자 노드 메모리 할당
    UserNode *newNode = (UserNode *)malloc(sizeof(UserNode));
    if (newNode == NULL)
    {
        perror("[에러] 사용자 노드 메모리 할당 실패");
        return NULL;
    }
    // 사용자 노드 초기화
    strncpy(newNode->user.id, id, MAX_ID_LEN - 1);
    newNode->user.id[MAX_ID_LEN - 1] = '\0';
    // 사용자 비밀번호 복사
    strncpy(newNode->user.pw, pw, MAX_PW_LEN - 1);
    newNode->user.pw[MAX_PW_LEN - 1] = '\0';
    // 다음 노드 초기화
    newNode->next = NULL;
    return newNode;
}

// 파일에서 사용자 정보를 읽어와 연결 리스트를 초기화
void user_init()
{
    FILE *file = fopen(USERS_FILE, "r");
    if (file == NULL)
    {
        printf("[정보] %s 파일이 없어 새로 시작합니다.\n", USERS_FILE);
        // 파일을 쓰기 모드로 열어 새로 생성
        FILE *createFile = fopen(USERS_FILE, "w");
        if (createFile)
        {
            fclose(createFile);
        }
        return;
    }

    // 사용자 정보 파일 읽기
    char id[MAX_ID_LEN], pw[MAX_PW_LEN];
    // 파일에서 사용자 정보 읽기
    while (fscanf(file, "%49[^:]:%49[^\n]\n", id, pw) == 2)
    {
        // 사용자 정보 추가
        user_add(id, pw);
    }

    fclose(file);
    printf("[정보] 사용자 정보를 파일에서 로드했습니다.\n");
}

// 연결 리스트의 모든 노드 메모리를 해제
void user_cleanup()
{
    // 연결 리스트의 모든 노드 메모리 해제
    UserNode *current = user_list_head;
    UserNode *next_node;
    while (current != NULL)
    {
        // 다음 노드 저장
        next_node = current->next;
        free(current);
        current = next_node;
    }
    // 연결 리스트 헤드 초기화
    user_list_head = NULL;
    printf("[정보] 모든 사용자 정보 메모리를 해제했습니다.\n");
}

// ID로 사용자를 찾아 User 구조체 포인터를 반환
User *user_find_by_id(const char *id)
{
    // 연결 리스트의 모든 노드 탐색
    UserNode *current = user_list_head;
    while (current != NULL)
    {
        // ID 비교
        if (strcmp(current->user.id, id) == 0)
        {
            return &(current->user);
        }
        current = current->next;
    }
    return NULL;
}

// 새로운 사용자를 연결 리스트에 추가
bool user_add(const char *id, const char *pw)
{
    // ID 중복 확인
    if (user_find_by_id(id) != NULL)
    {
        return false;
    }

    // 사용자 노드 생성
    UserNode *newNode = create_user_node(id, pw);
    if (newNode == NULL)
    {
        return false;
    }

    // 리스트의 끝에 추가
    if (user_list_head == NULL)
    {
        user_list_head = newNode;
    }
    else
    {
        // 리스트의 끝에 추가
        UserNode *current = user_list_head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newNode;
    }

    // 변경사항은 서버 종료 시점에 일괄 저장
    return true;
}

// ID로 사용자를 찾아 연결 리스트에서 삭제
bool user_delete_by_id(const char *id)
{
    // 연결 리스트의 모든 노드 탐색
    UserNode *current = user_list_head;
    UserNode *prev = NULL;

    while (current != NULL && strcmp(current->user.id, id) != 0)
    {
        prev = current;
        current = current->next;
    }

    // 사용자를 찾지 못한 경우
    if (current == NULL)
    {
        return false;
    }

    // 찾은 노드를 리스트에서 제거
    if (prev == NULL)
    { // 첫 번째 노드인 경우
        user_list_head = current->next;
    }
    else
    {
        prev->next = current->next;
    }
    free(current);

    // 변경사항은 서버 종료 시점에 일괄 저장
    return true;
}

// 사용자의 비밀번호를 변경
bool user_update_password(const char *id, const char *new_pw)
{
    // ID로 사용자를 찾아 비밀번호 변경
    User *user = user_find_by_id(id);
    if (user == NULL)
    {
        return false;
    }
    // 비밀번호 변경
    strncpy(user->pw, new_pw, MAX_PW_LEN - 1);
    user->pw[MAX_PW_LEN - 1] = '\0';

    // 변경사항은 서버 종료 시점에 일괄 저장
    return true;
}

// 현재 연결 리스트의 모든 사용자 정보를 파일에 저장
void user_save_to_file()
{
    // 사용자 정보 파일 열기
    FILE *file = fopen(USERS_FILE, "w");
    if (file == NULL)
    {
        perror("[에러] 사용자 파일 저장 실패");
        return;
    }

    // 연결 리스트의 모든 노드 탐색
    UserNode *current = user_list_head;
    while (current != NULL)
    {
        // 사용자 정보 파일에 저장
        fprintf(file, "%s:%s\n", current->user.id, current->user.pw);
        current = current->next;
    }
    // 파일 닫기
    fclose(file);
}
