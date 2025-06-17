// src/memo_command.h

#ifndef MEMO_COMMAND_H
#define MEMO_COMMAND_H

// 클라이언트로부터 받은 메모 관련 명령을 처리하고 응답 문자열을 생성
// - request: 클라이언트가 보낸 요청 (예: "MEMO_ADD:user_id:content")
// - reply: 서버가 클라이언트에게 보낼 응답을 저장할 버퍼
// - reply_size: 응답 버퍼의 크기
void handle_memo_command(const char *request, char *reply, int reply_size);

#endif
