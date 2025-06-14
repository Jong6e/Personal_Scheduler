// src/memo_command.c

#include "memo_command.h"
#include "memo.h"
#include "user.h" // For USER_ID_MAX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DELIMITER ":"

// 클라이언트로부터 받은 메모 관련 명령을 처리하고 응답 문자열을 생성합니다.
void handle_memo_command(const char *request, char *reply, int reply_size)
{
    char request_copy[1024];
    strncpy(request_copy, request, sizeof(request_copy) - 1);
    request_copy[sizeof(request_copy) - 1] = '\0';

    char *command = strtok(request_copy, DELIMITER);
    if (!command)
    {
        snprintf(reply, reply_size, "FAIL:잘못된 요청입니다.");
        return;
    }

    char *user_id = strtok(NULL, DELIMITER);
    if (!user_id)
    {
        snprintf(reply, reply_size, "FAIL:사용자 ID가 필요합니다.");
        return;
    }

    // MEMO_LIST
    if (strcmp(command, "MEMO_LIST") == 0)
    {
        if (list_memos_for_user(user_id, reply, reply_size))
        {
            // list_memos_for_user가 직접 응답 문자열(헤더 포함)을 생성
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:메모 목록을 불러오는 데 실패했습니다.");
        }
    }
    // MEMO_ADD
    else if (strcmp(command, "MEMO_ADD") == 0)
    {
        char *title = strtok(NULL, DELIMITER);
        char *content = strtok(NULL, ""); // title 다음의 모든 것을 content로

        if (user_id && title != NULL && content != NULL && add_memo_for_user(user_id, title, content))
        {
            snprintf(reply, reply_size, "OK:메모가 성공적으로 추가되었습니다.");
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:메모 추가에 실패했습니다.");
        }
    }
    // MEMO_VIEW
    else if (strcmp(command, "MEMO_VIEW") == 0)
    {
        char *memo_id_str = strtok(NULL, DELIMITER);
        if (memo_id_str)
        {
            int memo_id = atoi(memo_id_str);
            if (!get_memo_by_id(user_id, memo_id, reply, reply_size))
            {
                // 실패 메시지는 get_memo_by_id에서 생성
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:조회할 메모 ID가 필요합니다.");
        }
    }
    // MEMO_GET_CONTENT (수정용)
    else if (strcmp(command, "MEMO_GET_CONTENT") == 0)
    {
        char *memo_id_str = strtok(NULL, DELIMITER);
        if (memo_id_str)
        {
            int memo_id = atoi(memo_id_str);
            char content[MEMO_CONTENT_MAX];
            if (get_raw_memo_content(user_id, memo_id, content, sizeof(content)))
            {
                snprintf(reply, reply_size, "OK:%s", content);
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모 내용을 가져오는 데 실패했습니다.");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:가져올 메모 ID가 필요합니다.");
        }
    }
    // MEMO_UPDATE
    else if (strcmp(command, "MEMO_UPDATE") == 0)
    {
        char *memo_id_str = strtok(NULL, DELIMITER);
        char *content = strtok(NULL, "");
        if (memo_id_str && content)
        {
            int memo_id = atoi(memo_id_str);
            if (update_memo_for_user(user_id, memo_id, content))
            {
                snprintf(reply, reply_size, "OK:메모가 성공적으로 수정되었습니다.");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모 수정에 실패했습니다 (ID 불일치 등).");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:수정할 메모 ID와 내용이 필요합니다.");
        }
    }
    // MEMO_DELETE
    else if (strcmp(command, "MEMO_DELETE") == 0)
    {
        char *memo_id_str = strtok(NULL, DELIMITER);
        if (memo_id_str)
        {
            int memo_id = atoi(memo_id_str);
            if (delete_memo_for_user(user_id, memo_id))
            {
                snprintf(reply, reply_size, "OK:메모가 성공적으로 삭제되었습니다.");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모 삭제에 실패했습니다 (ID 불일치 등).");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:삭제할 메모 ID가 필요합니다.");
        }
    }
    else
    {
        snprintf(reply, reply_size, "FAIL:알 수 없는 MEMO 명령입니다: %s", command);
    }
}
