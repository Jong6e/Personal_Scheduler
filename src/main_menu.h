// src/main_menu.h

#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <winsock2.h>

// 로그인 후 메인 메뉴(사용자 메뉴)를 표시하고 상호작용하는 루프
void main_menu_loop(SOCKET sock, const char *user_id);

#endif
