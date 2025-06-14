#include "user_command.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define REPLY_BUF_SIZE 1024

// ✅ 클라이언트로부터 받은 사용자 관련 명령을 처리하고 응답 문자열을 생성합니다.
void handle_user_command(const char *request, char *reply)
{
    char buffer[REPLY_BUF_SIZE];
    strncpy(buffer, request, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *cmd = strtok(buffer, ":");
    if (!cmd)
    {
        snprintf(reply, REPLY_BUF_SIZE, "FAIL:명령어가 비어있습니다.");
        return;
    }

    if (strcmp(cmd, "LOGIN") == 0)
    {
        char *id = strtok(NULL, ":");
        char *pw = strtok(NULL, "\n");
        if (id && pw && check_login(id, pw))
        {
            snprintf(reply, REPLY_BUF_SIZE, "OK:로그인 성공.");
        }
        else
        {
            snprintf(reply, REPLY_BUF_SIZE, "FAIL:ID 또는 비밀번호가 잘못되었습니다.");
        }
    }
    else if (strcmp(cmd, "REGISTER") == 0)
    {
        char *id = strtok(NULL, ":");
        char *pw = strtok(NULL, "\n");
        if (id && pw && register_user(id, pw))
        {
            snprintf(reply, REPLY_BUF_SIZE, "OK:회원가입 완료.");
        }
        else
        {
            snprintf(reply, REPLY_BUF_SIZE, "FAIL:이미 존재하는 ID이거나 등록에 실패했습니다.");
        }
    }
    else if (strcmp(cmd, "DELETE") == 0)
    {
        char *id = strtok(NULL, ":");
        char *pw = strtok(NULL, "\n");
        if (id && pw && delete_user(id, pw))
        {
            snprintf(reply, REPLY_BUF_SIZE, "OK:계정 삭제 완료.");
        }
        else
        {
            snprintf(reply, REPLY_BUF_SIZE, "FAIL:비밀번호가 일치하지 않거나 없는 사용자입니다.");
        }
    }
    else if (strcmp(cmd, "UPDATE_PW") == 0)
    {
        char *id = strtok(NULL, ":");
        char *old_pw = strtok(NULL, ":");
        char *new_pw = strtok(NULL, "\n");
        if (id && old_pw && new_pw && update_password(id, old_pw, new_pw))
        {
            snprintf(reply, REPLY_BUF_SIZE, "OK:비밀번호 변경 완료.");
        }
        else
        {
            snprintf(reply, REPLY_BUF_SIZE, "FAIL:기존 비밀번호가 일치하지 않거나 없는 사용자입니다.");
        }
    }
    else
    {
        snprintf(reply, REPLY_BUF_SIZE, "FAIL:알 수 없는 사용자 관련 명령입니다.");
    }
}
