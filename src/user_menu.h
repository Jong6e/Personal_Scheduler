// src/user_menu.h

#ifndef USER_MENU_H
#define USER_MENU_H

#include <winsock2.h>

// ✅ 프로그램의 안전한 종료를 처리하는 함수
void exit_program(SOCKET sock);

// ✅ 로그인 전 사용자 메뉴를 표시하고 상호작용하는 메인 루프
// - 로그인, 회원가입 등의 기능을 처리합니다.
// - 로그인 성공 시, 다음 메뉴(메모 관리)로 넘어갑니다.
void user_menu_loop(SOCKET sock);

#endif // USER_MENU_H
