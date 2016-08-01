#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 1024

typedef struct //소켓정보 구조체
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct // 소켓버퍼정보 구조체
{
	OVERLAPPED overlapped;
	char buffer[BUFSIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

typedef struct
{
	BOOL opFlag;	// 존재 유무
	int uCount;		// 유저 수
	int stNum;		// 스테이지
} ROOMDATA;

typedef struct
{
	int uCount;		// 유저 수
	int rCount;		// 방 개수
	ROOMDATA rInfo[750];	// 방 정보
} LOBBY_DATA;

unsigned int __stdcall CompletionThread(LPVOID pComPort);
void ErrorHandling(char *message);

LOBBY_DATA lobInfo;

int main(int argc, char** argv)
{
	WSADATA wsaData;
	HANDLE hCompletionPort;
	SYSTEM_INFO SystemInfo;
	SOCKADDR_IN servAddr;
	LPPER_IO_DATA PerIoData;
	LPPER_HANDLE_DATA PerHandleData;
	SOCKET hServSock;
	int RecvBytes;
	int i, Flags;

	lobInfo.uCount = 0;
	lobInfo.rCount = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&SystemInfo);	// 시스템 정보를 얻음. CPU 개수만큼 쓰레드 만들기 위해 필요(dwNumberOfProcessors), 노트북에선 8개 생성됨을 확인 --

	// 쓰레드를 CPU 개수만큼 생성
	for (i = 0; i < SystemInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, CompletionThread, (LPVOID)hCompletionPort, 0, NULL);

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi("2738"));

	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);

	while (TRUE)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);

		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);

		lobInfo.uCount++;
		printf("접속 인원 : %d\n", lobInfo.uCount);

		// 연결된 클라의 소켓과 주소 정보 설정, PerHandleData = 클라정보
		PerHandleData = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		PerHandleData->hClntSock = hClntSock;
		memcpy(&(PerHandleData->clntAddr), &clntAddr, addrLen);

		// CP 연결(CompletionPort)
		CreateIoCompletionPort((HANDLE)hClntSock, hCompletionPort, (DWORD)PerHandleData, 0);

		// 다시 구조체 초기화
		PerIoData = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
		PerIoData->wsaBuf.len = BUFSIZE;
		PerIoData->wsaBuf.buf = PerIoData->buffer;

		Flags = 0;

		WSARecv(PerHandleData->hClntSock, // 데이터 입력소켓.
			&(PerIoData->wsaBuf),  // 데이터 입력 버퍼포인터.
			1,       // 데이터 입력 버퍼의 수.
			(LPDWORD)&RecvBytes,
			(LPDWORD)&Flags,
			&(PerIoData->overlapped), // OVERLAPPED 구조체 포인터.
			NULL
			);
	}
	return 0;
}

//입출력 완료에 따른 쓰레드의 행동 정의
unsigned int __stdcall CompletionThread(LPVOID pComPort)
{
	HANDLE hCompletionPort = (HANDLE)pComPort;
	DWORD BytesTransferred;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_DATA PerIoData;
	DWORD flags;

	char makeRoom = 1;
	char joinRoom = 2;
	char exitRoom = 3;
	char pReady = 4;
	char pStart = 5;

	while (1)
	{
		// IO 정보
		GetQueuedCompletionStatus(hCompletionPort,    // Completion Port
			&BytesTransferred,   // 전송된 바이트수
			(LPDWORD)&PerHandleData,
			(LPOVERLAPPED*)&PerIoData, // OVERLAPPED 구조체 포인터.
			INFINITE
		);

		// 연결 종료시
		if (BytesTransferred == 0) //EOF 전송시.
		{
			lobInfo.uCount--;
			printf("접속 인원 : %d\n", lobInfo.uCount);
			closesocket(PerHandleData->hClntSock);
			free(PerHandleData);
			free(PerIoData);
			continue;
		}

		PerIoData->wsaBuf.buf[BytesTransferred] = '\0';
		printf("Recv[%s]\n", PerIoData->wsaBuf.buf);

		int buf = (atoi)(PerIoData->wsaBuf.buf);

		if (buf == makeRoom)
		{
			printf("Make Room");
		}
		else if (buf == joinRoom)
		{
			printf("Join Room");
		}
		else if (buf == exitRoom)
		{
		printf("Exit Room");
		}
		else if (buf == pReady)
		{
			printf("Ready");
		}
		else if (buf == pStart)
		{
			printf("Start");
		}
		

		// Send
		PerIoData->wsaBuf.len = BytesTransferred;
		WSASend(PerHandleData->hClntSock, &(PerIoData->wsaBuf), 1, NULL, 0, NULL, NULL);

		// Receive
		memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
		PerIoData->wsaBuf.len = BUFSIZE;
		PerIoData->wsaBuf.buf = PerIoData->buffer;
		flags = 0;
		WSARecv(PerHandleData->hClntSock,
			&(PerIoData->wsaBuf),
			1,
			NULL,
			&flags,
			&(PerIoData->overlapped),
			NULL
			);
	}
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}