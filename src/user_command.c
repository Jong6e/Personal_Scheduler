#include "user_command.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define REPLY_BUF_SIZE 1024
#define DELIMITER ":"

// 클라이언트로부터 받은 사용자 관련 명령을 처리하고 응답 문자열을 생성합니다.
void handle_user_command(const char *request, char *reply, int reply_size)
{
    char *request_copy = strdup(request);
    char *command = strtok(request_copy, DELIMITER);

    if (command == NULL)
    {
        snprintf(reply, reply_size, "FAIL:잘못된 요청입니다.");
        free(request_copy);
        return;
    }

    if (strcmp(command, "LOGIN") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *pw = strtok(NULL, DELIMITER);
        if (id && pw && check_login(id, pw))
        {
            snprintf(reply, reply_size, "OK:로그인 성공");
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:아이디 또는 비밀번호가 틀렸습니다.");
        }
    }
    else if (strcmp(command, "REGISTER") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *pw = strtok(NULL, DELIMITER);
        if (id && pw && register_user(id, pw))
        {
            snprintf(reply, reply_size, "OK:회원가입 성공");
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:이미 존재하는 아이디입니다.");
        }
    }
    else if (strcmp(command, "DELETE_USER") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *pw = strtok(NULL, DELIMITER);

        if (id && pw && check_login(id, pw))
        {
            if (delete_user(id))
            {
                snprintf(reply, reply_size, "OK:회원 탈퇴 성공");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:회원 탈퇴 중 오류 발생");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:비밀번호가 틀렸습니다.");
        }
    }
    else if (strcmp(command, "UPDATE_PW") == 0)
    {
        char *id = strtok(NULL, DELIMITER);
        char *old_pw = strtok(NULL, DELIMITER);
        char *new_pw = strtok(NULL, DELIMITER);

        if (id && old_pw && new_pw && update_user_password(id, old_pw, new_pw))
        {
            snprintf(reply, reply_size, "OK:비밀번호 변경 성공");
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:기존 비밀번호가 틀렸거나 오류가 발생했습니다.");
        }
    }
    else
    {
        snprintf(reply, reply_size, "FAIL:알 수 없는 USER 명령입니다: %s", command);
    }

    free(request_copy);
}
