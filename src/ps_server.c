// src/ps_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>
#include <locale.h>
#include "user.h"
#include "user_command.h"
#include "memo.h"
#include "memo_command.h"
#include <signal.h>
#include <direct.h>
#include <sys/stat.h>

#pragma comment(lib, "ws2_32.lib") // 소켓 라이브러리

#define PORT 12345      // 포트 번호
#define BUF_SIZE 2048   // 버퍼 크기
#define MAX_CLIENTS 100 // 최대 클라이언트 수

// 전역 서버 소켓
static SOCKET g_serv_sock = INVALID_SOCKET; // 서버 소켓

// 클라이언트 연결과 통신 처리
DWORD WINAPI handle_client(LPVOID client_socket);

// Ctrl+C 신호 처리
void signal_handler(int signum) // 신호 처리
{
    // Ctrl+C 신호 수신 시
    if (signum == SIGINT)
    {
        // 종료 메시지 출력
        printf("\n[서버] 종료 신호(Ctrl+C) 수신. 서버를 안전하게 종료합니다...\n");

        // 사용자 정보 파일에 저장
        printf("[서버] 모든 사용자 정보를 파일에 저장 중...\n");
        user_save_to_file();
        // 모든 메모 정보 파일에 저장
        printf("[서버] 모든 메모 정보를 파일에 저장 중...\n");
        memo_save_all_to_files();

        // 리소스 정리
        if (g_serv_sock != INVALID_SOCKET) // 서버 소켓 유효 시
        {
            // 서버 소켓 닫기
            closesocket(g_serv_sock);
        }
        // 사용자 정보 정리
        user_cleanup();
        // 메모 정보 정리
        memo_cleanup();
        // 소켓 라이브러리
        WSACleanup();

        printf("[서버] 모든 리소스가 정리되었습니다. 프로그램을 종료합니다.\n");
        exit(0);
    }
}

// 데이터 저장을 위한 디렉터리를 생성하는 함수
void create_data_directories()
{
    struct stat st = {0};

    // 'data' 디렉터리 확인 및 생성
    if (stat("data", &st) == -1)
    {
        if (_mkdir("data") == 0)
        {
            printf("'data' 디렉터리를 생성했습니다.\n");
        }
        else
        {
            perror("'data' 디렉터리 생성 실패");
        }
    }

    // 'data/memo' 디렉터리 확인 및 생성
    if (stat("data/memo", &st) == -1)
    {
        if (_mkdir("data/memo") == 0)
        {
            printf("'data/memo' 디렉터리를 생성했습니다.\n");
        }
        else
        {
            perror("'data/memo' 디렉터리 생성 실패");
        }
    }
}

int main()
{
    create_data_directories(); // 데이터 디렉터리 생성

    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);

    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);

    user_init();
    memo_init();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }
    // 서버 소켓 생성
    g_serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // 서버 소켓 바인딩
    bind(g_serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(g_serv_sock, MAX_CLIENTS);

    printf("[서버] 클라이언트 연결 대기 중... (Ctrl+C로 종료)\n");

    // 클라이언트 연결 대기
    while (true)
    {
        // 클라이언트 주소 구조체
        struct sockaddr_in client_addr;
        // 클라이언트 주소 크기
        int client_addr_size = sizeof(client_addr);
        // 클라이언트 소켓 생성
        SOCKET client_sock = accept(g_serv_sock, (struct sockaddr *)&client_addr, &client_addr_size);
        // 클라이언트 소켓 생성 실패 시
        if (client_sock == INVALID_SOCKET)
        {
            // 시그널 핸들러에서 소켓이 닫힌 경우 accept가 실패할 수 있음 (Ctrl+C 신호 수신 시)
            if (WSAGetLastError() == WSAEINTR)
            {
                break; // 루프 종료
            }
            printf("[서버] accept() 실패\n");
            continue;
        }

        // 클라이언트 처리 스레드 생성
        CreateThread(NULL, 0, handle_client, (LPVOID)client_sock, 0, NULL);
    }

    // 서버 소켓 닫기
    closesocket(g_serv_sock);
    // 사용자 정보 정리
    user_cleanup();
    // 메모 정보 정리
    memo_cleanup();
    // 소켓 라이브러리
    WSACleanup();
    return 0;
}

// 클라이언트 처리 스레드
DWORD WINAPI handle_client(LPVOID client_socket)
{
    // 클라이언트 소켓
    SOCKET sock = (SOCKET)(ULONG_PTR)client_socket;
    // 버퍼
    char buffer[BUF_SIZE], reply[BUF_SIZE];

    printf("[서버] 클라이언트 %llu 연결됨\n", sock);

    // 클라이언트 연결 유지
    while (true)
    {
        // 클라이언트로부터 데이터 수신
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        // 수신 실패 시
        if (bytes <= 0)
        {
            printf("[서버] 클라이언트 %llu 연결 해제됨\n", sock);
            break;
        }
        // 버퍼 끝에 null 문자 추가
        buffer[bytes] = '\0';
        // 수신 데이터 출력
        printf("[서버] 클라이언트 %llu 수신: %s\n", sock, buffer);

        // "EXIT" 명령어 처리
        if (strcmp(buffer, "EXIT") == 0)
        {
            printf("[서버] 클라이언트 %llu 정상 종료 요청\n", sock);
            // 클라이언트에게 정상적으로 응답을 보내고 루프를 탈출
            snprintf(reply, sizeof(reply), "OK:서버와 연결을 종료합니다.");
            // 응답 전송
            send(sock, reply, strlen(reply), 0);
            break;
        }

        // 명령어에 따라 핸들러 분기
        if (strncmp(buffer, "MEMO_", 5) == 0 ||
            strncmp(buffer, "DOWNLOAD_ALL", 12) == 0 ||
            strncmp(buffer, "DOWNLOAD_SINGLE", 15) == 0)
        {
            // 메모 명령어 처리
            handle_memo_command(buffer, reply, sizeof(reply));
        }
        else
        {
            // 사용자 명령어 처리
            handle_user_command(buffer, reply, sizeof(reply));
        }

        // 응답 전송
        send(sock, reply, strlen(reply), 0);
    }

    // 클라이언트 소켓 닫기
    closesocket(sock);
    return 0;
}
