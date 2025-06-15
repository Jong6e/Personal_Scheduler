// src/ps_client.c

#include <stdio.h>
#include <winsock2.h>
#include <locale.h>
#include "user_menu.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 12345

int main()
{
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[클라이언트] 서버 연결 실패\n");
        WSACleanup();
        system("pause");
        return 1;
    }

    printf("[클라이언트] 서버에 연결되었습니다.\n");
    Sleep(500);

    // 모든 UI와 로직은 user_menu_loop가 담당
    user_menu_loop(sock);

    closesocket(sock);
    WSACleanup();
    return 0;
}
