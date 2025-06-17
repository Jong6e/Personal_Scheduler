// src/ps_client.c

#include <stdio.h>
#include <winsock2.h>
#include <locale.h>
#include "user_menu.h"
#include <direct.h>
#include <sys/stat.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 12345

// 'downloads' 디렉터리 생성
void create_downloads_directory()
{
    struct stat st = {0};
    if (stat("downloads", &st) == -1)
    {
        if (_mkdir("downloads") != 0)
        {
            perror("'downloads' 디렉터리 생성 실패");
        }
    }
}

int main()
{
    // 로케일 설정
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 다운로드 디렉터리 생성
    create_downloads_directory();

    // 소켓 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    // 서버 주소 설정
    struct sockaddr_in serv_addr = {0};
    // 소켓 주소 설정
    serv_addr.sin_family = AF_INET;
    // 서버 IP 주소 설정
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // 서버 포트 설정
    serv_addr.sin_port = htons(PORT);

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[클라이언트] 서버 연결 실패\n");
        WSACleanup();
        system("pause");
        return 1;
    }

    printf("[클라이언트] 서버에 연결되었습니다.\n");
    Sleep(500);

    // 메뉴 루프
    user_menu_loop(sock);

    closesocket(sock);
    WSACleanup();
    return 0;
}
