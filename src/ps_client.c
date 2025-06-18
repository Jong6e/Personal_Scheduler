// src/ps_client.c

#include <stdio.h>
#include <winsock2.h>
#include <locale.h>
#include "user_menu.h"
#include <direct.h>   // for _mkdir
#include <sys/stat.h> // for stat

#pragma comment(lib, "ws2_32.lib") // 소켓 라이브러리

#define SERVER_IP "127.0.0.1" // 서버 IP 주소
#define PORT 12345            // 포트 번호

// 'downloads' 디렉터리를 생성하는 함수
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
    // 콘솔 출력 코드 페이지 설정
    SetConsoleOutputCP(CP_UTF8);
    // 콘솔 입력 코드 페이지 설정
    SetConsoleCP(CP_UTF8);

    create_downloads_directory(); // 다운로드 디렉터리 생성

    // 소켓 라이브러리 초기화
    WSADATA wsa;
    // 소켓 라이브러리 초기화 실패 시
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    // 서버 주소 구조체
    struct sockaddr_in serv_addr = {0};
    // 서버 주소 구조체 초기화
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
