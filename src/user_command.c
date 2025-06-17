// src/user_command.c

#include "user_command.h"
#include "user.h"
#include "memo.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#define DELIMITER ":"

// ID 유효성 검사 (영문/숫자 조합, 길이 제한)
static bool is_valid_id(const char *id)
{
    // 길이 검사
    size_t len = strlen(id);
    if (len < MIN_ID_LEN || len > MAX_ID_LEN)
        return false;

    // 영문/숫자 조합 검사
    bool has_alpha = false, has_digit = false;
    for (size_t i = 0; id[i]; i++)
    {
        if (!isalnum(id[i])) // 영문자나 숫자가 아닌 문자가 포함된 경우
            return false;
        if (isalpha(id[i]))
            has_alpha = true;
        if (isdigit(id[i]))
            has_digit = true;
    }

    // 영문과 숫자를 모두 포함해야 함
    return has_alpha && has_digit;
}

// 비밀번호 유효성 검사 함수 (공백 없음, 길이 제한)
static bool is_valid_password(const char *pw)
{
    // 길이 검사
    size_t len = strlen(pw);
    if (len < MIN_PW_LEN || len > MAX_PW_LEN)
        return false;

    // 공백 검사
    for (size_t i = 0; pw[i]; i++)
    {
        if (isspace(pw[i]))
            return false;
    }

    return true;
}

// 사용자 명령 처리
void handle_user_command(const char *request, char *reply, int reply_size)
{
    // 요청 복사
    char *request_copy = strdup(request);
    // 복사 실패 시
    if (request_copy == NULL)
    {
        snprintf(reply, reply_size, "FAIL:서버 메모리 할당 오류");
        return;
    }
    // 명령어 파싱
    char *command = strtok(request_copy, DELIMITER);
    // 명령어 없음
    if (command == NULL)
    {
        snprintf(reply, reply_size, "FAIL:잘못된 요청 형식입니다.");
        free(request_copy);
        return;
    }

    // 로그인
    if (strcmp(command, "LOGIN") == 0)
    {
        // 아이디 파싱
        char *id = strtok(NULL, DELIMITER);
        // 비밀번호 파싱
        char *pw = strtok(NULL, DELIMITER);
        // 아이디 또는 비밀번호 없음
        if (id == NULL || pw == NULL)
        {
            snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 누락되었습니다.");
        }
        else
        {
            // 사용자 조회
            User *user = user_find_by_id(id);
            // 사용자 존재 및 비밀번호 일치 시
            if (user && strcmp(user->pw, pw) == 0)
            {
                // 로그인 성공
                snprintf(reply, reply_size, "OK:로그인 성공");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 틀렸습니다.");
            }
        }
    }
    // 회원가입
    else if (strcmp(command, "REGISTER") == 0)
    {
        // 아이디 파싱
        char *id = strtok(NULL, DELIMITER);
        // 비밀번호 파싱
        char *pw = strtok(NULL, DELIMITER);
        // 아이디 또는 비밀번호 없음
        if (id == NULL || pw == NULL)
        {
            snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 누락되었습니다.");
        }
        else
        {
            // 아이디 유효성 검사
            if (!is_valid_id(id))
            {
                snprintf(reply, reply_size, "FAIL:아이디는 영문/숫자 조합으로 %d~%d자여야 합니다.", MIN_ID_LEN, MAX_ID_LEN);
            }
            // 비밀번호 유효성 검사
            else if (!is_valid_password(pw))
            {
                snprintf(reply, reply_size, "FAIL:비밀번호는 공백 없이 %d~%d자여야 합니다.", MIN_PW_LEN, MAX_PW_LEN);
            }
            // 사용자 추가
            else if (user_add(id, pw))
            {
                // 사용자 목록 파일 저장
                user_save_to_file();
                snprintf(reply, reply_size, "OK:회원가입 성공");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:이미 존재하는 아이디이거나 오류가 발생했습니다.");
            }
        }
    }
    // 회원탈퇴
    else if (strcmp(command, "DELETE_USER") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *pw = strtok(NULL, DELIMITER);
        if (id == NULL || pw == NULL)
        {
            snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 누락되었습니다.");
        }
        else
        {
            User *user = user_find_by_id(id);
            if (user && strcmp(user->pw, pw) == 0)
            {
                // 성공 시 파일 저장을 위해 id를 복사해둠
                char user_id_copy[MAX_ID_LEN];
                strncpy(user_id_copy, id, sizeof(user_id_copy) - 1);
                user_id_copy[sizeof(user_id_copy) - 1] = '\0';

                if (user_delete_by_id(user_id_copy))
                {
                    user_save_to_file();                  // 사용자 목록 파일 저장
                    memo_delete_by_user_id(user_id_copy); // 사용자의 메모도 함께 삭제
                    snprintf(reply, reply_size, "OK:회원 탈퇴 성공");
                }
                else
                {
                    snprintf(reply, reply_size, "FAIL:회원 탈퇴 중 오류 발생");
                }
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 틀렸습니다.");
            }
        }
    }
    // 비밀번호 변경
    else if (strcmp(command, "UPDATE_PW") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *old_pw = strtok(NULL, DELIMITER);
        char *new_pw = strtok(NULL, DELIMITER);
        if (id == NULL || old_pw == NULL || new_pw == NULL)
        {
            snprintf(reply, reply_size, "FAIL:필수 정보가 누락되었습니다.");
        }
        else
        {
            User *user = user_find_by_id(id);
            if (user && strcmp(user->pw, old_pw) == 0)
            {
                // 새 비밀번호 유효성 검사
                if (!is_valid_password(new_pw))
                {
                    snprintf(reply, reply_size, "FAIL:비밀번호는 공백 없이 %d~%d자여야 합니다.", MIN_PW_LEN, MAX_PW_LEN);
                }
                else if (user_update_password(id, new_pw))
                {
                    user_save_to_file(); // 변경사항을 즉시 파일에 저장
                    snprintf(reply, reply_size, "OK:비밀번호 변경 성공");
                }
                else
                {
                    snprintf(reply, reply_size, "FAIL:비밀번호 변경 중 오류 발생");
                }
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:기존 비밀번호가 틀렸거나 사용자가 존재하지 않습니다.");
            }
        }
    }
    // 예외처리
    else
    {
        snprintf(reply, reply_size, "FAIL:알 수 없는 USER 명령입니다: %s", command);
    }
    free(request_copy);
}
