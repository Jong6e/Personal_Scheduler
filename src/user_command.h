// src/user_command.h

#ifndef USER_COMMAND_H
#define USER_COMMAND_H

// ✅ 클라이언트로부터 받은 사용자 관련 명령을 처리하고 응답 문자열을 생성합니다.
// - request: 클라이언트가 보낸 요청 문자열 (예: "LOGIN:id:pw")
// - reply: 서버가 클라이언트에게 보낼 응답 문자열을 저장할 버퍼
void handle_user_command(const char *request, char *reply);

#endif // USER_COMMAND_H
