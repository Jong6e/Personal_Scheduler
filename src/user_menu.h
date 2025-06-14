// src/user_menu.h

#ifndef USER_MENU_H
#define USER_MENU_H

#include <winsock2.h>

// 프로그램을 종료합니다.
void exit_program(SOCKET sock);

// 로그인 이전 사용자를 위한 메뉴입니다. (로그인, 회원가입, 종료)
void user_menu_loop(SOCKET sock);

#endif // USER_MENU_H
