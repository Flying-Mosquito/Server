#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 1024

typedef struct //�������� ����ü
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct // ���Ϲ������� ����ü
{
	OVERLAPPED overlapped;
	char buffer[BUFSIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

typedef struct
{
	BOOL opFlag;	// ���� ����
	int uCount;		// ���� ��
	int stNum;		// ��������
} ROOMDATA;

typedef struct
{
	int uCount;		// ���� ��
	int rCount;		// �� ����
	ROOMDATA rInfo[750];	// �� ����
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
	GetSystemInfo(&SystemInfo);	// �ý��� ������ ����. CPU ������ŭ ������ ����� ���� �ʿ�(dwNumberOfProcessors), ��Ʈ�Ͽ��� 8�� �������� Ȯ�� --

	// �����带 CPU ������ŭ ����
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
		printf("���� �ο� : %d\n", lobInfo.uCount);

		// ����� Ŭ���� ���ϰ� �ּ� ���� ����, PerHandleData = Ŭ������
		PerHandleData = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		PerHandleData->hClntSock = hClntSock;
		memcpy(&(PerHandleData->clntAddr), &clntAddr, addrLen);

		// CP ����(CompletionPort)
		CreateIoCompletionPort((HANDLE)hClntSock, hCompletionPort, (DWORD)PerHandleData, 0);

		// �ٽ� ����ü �ʱ�ȭ
		PerIoData = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
		PerIoData->wsaBuf.len = BUFSIZE;
		PerIoData->wsaBuf.buf = PerIoData->buffer;

		Flags = 0;

		WSARecv(PerHandleData->hClntSock, // ������ �Է¼���.
			&(PerIoData->wsaBuf),  // ������ �Է� ����������.
			1,       // ������ �Է� ������ ��.
			(LPDWORD)&RecvBytes,
			(LPDWORD)&Flags,
			&(PerIoData->overlapped), // OVERLAPPED ����ü ������.
			NULL
			);
	}
	return 0;
}

//����� �Ϸῡ ���� �������� �ൿ ����
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
		// IO ����
		GetQueuedCompletionStatus(hCompletionPort,    // Completion Port
			&BytesTransferred,   // ���۵� ����Ʈ��
			(LPDWORD)&PerHandleData,
			(LPOVERLAPPED*)&PerIoData, // OVERLAPPED ����ü ������.
			INFINITE
		);

		// ���� �����
		if (BytesTransferred == 0) //EOF ���۽�.
		{
			lobInfo.uCount--;
			printf("���� �ο� : %d\n", lobInfo.uCount);
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
			// lobInfo.rCount++;
			// lobInfo.roomInfo.opFlag++;
			// lobInfo.roomInfo.uCount++;
		}
		else if (buf == joinRoom)
		{
			printf("Join Room");
			// lobInfo.roomInfo.uCount++;
		}
		else if (buf == exitRoom)
		{
			printf("Exit Room");
			// lobInfo.roomInfo.uCount--;
		}
		else if (buf == pReady)
		{
			printf("Ready");
			// if (lobInfo.roomInfo.uCount == 4)
			//		lobInfo.roomInfo.stNum;
		}
		else if (buf == pStart)
		{
			printf("Start");
			// Send GameIP
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