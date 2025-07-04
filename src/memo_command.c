// src/memo_command.c

#include "memo_command.h"
#include "memo.h"
#include "export_util.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define DELIMITER ":"

// 문자열 앞쪽의 모든 비정상 문자(공백, 제어문자, 깨진 바이트)를 제거하는 강화된 함수
static char *trim_leading_garbage(char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    // 정상적인 출력 가능 문자를 만날 때까지 포인터를 이동
    while (*str != '\0')
    {
        unsigned char c = *str;

        // 표준 공백 문자인 경우 건너뛰기
        if (isspace(c))
        {
            str++;
            continue;
        }

        // ASCII 제어 문자인 경우 건너뛰기 (0x20 미만)
        if (c < 0x20)
        {
            str++;
            continue;
        }

        // UTF-8 깨진 문자(U+FFFD)도 예방적으로 확인
        if (c == 0xEF && (unsigned char)str[1] == 0xBF && (unsigned char)str[2] == 0xBD)
        {
            str += 3;
            continue;
        }

        // 위 조건에 해당하지 않는 '정상적인' 문자를 만나면 루프 종료
        break;
    }
    return str;
}

// 클라이언트로부터 받은 메모 관련 명령을 처리하고 응답 문자열을 생성
void handle_memo_command(const char *request, char *reply, int reply_size)
{
    // strdup를 사용하여 원본 요청 문자열을 복사
    char *request_copy = strdup(request);
    if (request_copy == NULL)
    {
        snprintf(reply, reply_size, "FAIL:메모리 할당 오류");
        return;
    }
    // 명령어 파싱
    char *command = strtok(request_copy, DELIMITER);
    if (!command)
    {
        snprintf(reply, reply_size, "FAIL:잘못된 요청입니다.");
        free(request_copy);
        return;
    }
    // 사용자 ID 파싱
    char *user_id = strtok(NULL, DELIMITER);
    if (!user_id)
    {
        snprintf(reply, reply_size, "FAIL:사용자 ID가 필요합니다.");
        free(request_copy);
        return;
    }
    // 메모 목록 조회
    if (strcmp(command, "MEMO_LIST") == 0)
    {
        if (!memo_list_for_user(user_id, reply, reply_size))
        {
            snprintf(reply, reply_size, "FAIL:메모 목록을 불러오는 데 실패했습니다.");
        }
    }
    // 메모 목록 조회 (월별)
    else if (strcmp(command, "MEMO_LIST_BY_MONTH") == 0)
    {
        // 연도 파싱
        char *year_str = strtok(NULL, DELIMITER);
        // 월 파싱
        char *month_str = strtok(NULL, DELIMITER);
        // 연도와 월이 모두 있는 경우
        if (year_str && month_str)
        {
            // 연도 변환
            int year = atoi(year_str);
            // 월 변환
            int month = atoi(month_str);
            // 메모 목록 조회
            if (!memo_list_by_month(user_id, year, month, reply, reply_size))
            {
                // 실패 메시지는 함수 내에서 생성됨
            }
        }
        // 연도와 월이 모두 없는 경우
        else
        {
            snprintf(reply, reply_size, "FAIL:연도와 월 정보가 필요합니다.");
        }
    }
    // 메모 추가
    else if (strcmp(command, "MEMO_ADD") == 0)
    {
        // 제목 파싱
        char *title = strtok(NULL, DELIMITER);
        // 제목이 있는 경우
        if (title)
        {
            char *content = title + strlen(title) + 1;

            // 제목과 내용의 앞쪽 공백 및 깨진 문자 제거
            title = trim_leading_garbage(title);
            content = trim_leading_garbage(content);

            if (content && *content && memo_add(user_id, title, content))
            {
                memo_save_all_to_files(); // 변경사항 즉시 저장
                snprintf(reply, reply_size, "OK:메모가 성공적으로 추가되었습니다.");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모 추가에 실패했습니다. (내용 누락 등)");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:메모 제목이 필요합니다.");
        }
    }
    // 메모 상세 조회
    else if (strcmp(command, "MEMO_VIEW") == 0)
    {
        // 메모 ID 파싱
        char *memo_id_str = strtok(NULL, DELIMITER);
        // 메모 ID가 있는 경우
        if (memo_id_str)
        {
            // 메모 ID 변환
            int memo_id = atoi(memo_id_str);
            // 메모 상세 조회
            if (!memo_get_by_id(memo_id, user_id, reply, reply_size))
            {
                // 실패 메시지는 함수에서 생성
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:조회할 메모 ID가 필요합니다.");
        }
    }
    // 메모 수정
    else if (strcmp(command, "MEMO_UPDATE") == 0)
    {
        // 메모 ID 파싱
        char *memo_id_str = strtok(NULL, DELIMITER);
        // 메모 ID가 있는 경우
        if (memo_id_str)
        {
            // 메모 ID 변환
            char *content = memo_id_str + strlen(memo_id_str) + 1;
            int memo_id = atoi(memo_id_str);

            // 내용의 앞쪽 공백 및 깨진 문자 제거
            content = trim_leading_garbage(content);

            if (content && *content && memo_update(memo_id, user_id, content))
            {
                memo_save_all_to_files(); // 변경사항 즉시 저장
                snprintf(reply, reply_size, "OK:메모가 성공적으로 수정되었습니다.");
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모 수정에 실패했습니다. (내용 누락 등)");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:수정할 메모 ID가 필요합니다.");
        }
    }
    // 메모 삭제
    else if (strcmp(command, "MEMO_DELETE") == 0)
    {
        // 메모 ID 파싱
        char *memo_id_str = strtok(NULL, DELIMITER);
        // 메모 ID가 있는 경우
        if (memo_id_str)
        {
            // 메모 ID 변환
            int memo_id = atoi(memo_id_str);
            if (memo_delete(memo_id, user_id))
            {
                memo_save_all_to_files();
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
    // 메모 검색
    else if (strcmp(command, "MEMO_SEARCH") == 0)
    {
        // 검색 필드 파싱
        char *field = strtok(NULL, DELIMITER);
        // 검색 필드가 있는 경우
        if (field)
        {
            // 검색어 파싱
            char *keyword = field + strlen(field) + 1;
            if (*keyword && !memo_search(user_id, field, keyword, reply, reply_size))
            {
                // 실패 메시지는 함수 내부에서 생성됨
            }
            else if (!*keyword)
            {
                snprintf(reply, reply_size, "FAIL:검색어가 필요합니다.");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:검색 필드가 필요합니다.");
        }
    }
    // 메모 다운로드
    else if (strcmp(command, "DOWNLOAD_SINGLE") == 0)
    {
        // 메모 ID 파싱
        char *memo_id_str = strtok(NULL, DELIMITER);
        // 포맷 파싱
        char *format = strtok(NULL, DELIMITER);
        // 메모 ID와 포맷이 모두 있는 경우
        if (memo_id_str && format)
        {
            // 메모 ID 변환
            int memo_id = atoi(memo_id_str);
            // 메모 조회
            const Memo *memo = memo_get_by_id_internal(memo_id, user_id);
            // 메모가 있는 경우
            if (memo)
            {
                // 메모 내보내기
                char *exported_string = export_single_memo_to_string(memo, format);
                // 내보내기 성공 시
                if (exported_string)
                {
                    snprintf(reply, reply_size, "OK:%s", exported_string);
                    free(exported_string);
                }
                else
                {
                    snprintf(reply, reply_size, "FAIL:지원하지 않는 포맷입니다.");
                }
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:메모를 찾을 수 없습니다.");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:메모 ID 또는 포맷이 누락되었습니다.");
        }
    }
    // 전체 메모 다운로드
    else if (strcmp(command, "DOWNLOAD_ALL") == 0)
    {
        // 포맷 파싱
        char *format = strtok(NULL, DELIMITER);
        // 포맷이 있는 경우
        if (format)
        {
            // 최대 1024개 메모 가정
            Memo user_memos[1024];
            // 메모 개수 조회
            int memo_count = memo_get_all_for_user_internal(user_id, user_memos, 1024);
            // 메모가 있는 경우
            if (memo_count > 0)
            {
                // 메모 내보내기
                char *exported_string = export_all_memos_to_string(user_memos, memo_count, format);
                // 내보내기 성공 시
                if (exported_string)
                {
                    snprintf(reply, reply_size, "OK:%s", exported_string);
                    free(exported_string);
                }
                else
                {
                    snprintf(reply, reply_size, "FAIL:지원하지 않는 포맷입니다.");
                }
            }
            else
            {
                snprintf(reply, reply_size, "FAIL:다운로드할 메모가 없습니다.");
            }
        }
        else
        {
            snprintf(reply, reply_size, "FAIL:포맷이 누락되었습니다.");
        }
    }
    // 알 수 없는 명령어
    else
    {
        snprintf(reply, reply_size, "FAIL:알 수 없는 MEMO 명령입니다: %s", command);
    }
    free(request_copy);
}
