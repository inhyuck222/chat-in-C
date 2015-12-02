//SERVER
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>


#define SERVERPORT 9000
#define BUFSIZE    512

typedef struct clientNode{
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	clientNode* link;
}clientNode;

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 클라이언트와 데이터 통신
DWORD WINAPI SendClient(LPVOID arg)
{
	//SOCKADDR_IN clientaddr[2];
	//int addrlen[2];
	clientNode* client = (clientNode*)arg;
	printf("%d\n", client->client_sock);
	int retval;
	char buf1[BUFSIZE+1];

	// 클라이언트 정보 얻기
	//addrlen[0] = sizeof(clientaddr[0]);
	//getpeername(client_sock[1], (SOCKADDR *)&clientaddr[1], &addrlen[1]);
	client->addrlen = sizeof(client->addrlen);
	getpeername(client->client_sock, (SOCKADDR *)(&(client->clientaddr)), &(client->addrlen));
	int len;

	while(1){
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if(fgets(buf1, BUFSIZE+1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = strlen(buf1);
		if(buf1[len-1] == '\n')buf1[len-1] = '\0';
		if(strlen(buf1) == 0)break;

		// 데이터 보내기
		//retval = send(client_sock[1], buf1, strlen(buf1), 0);
		retval = send(client->client_sock, buf1, strlen(buf1), 0);
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
	}

	// closesocket()
	closesocket(client->client_sock);
	/*
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	*/
	return 0;
}

DWORD WINAPI RecvClient(LPVOID arg)
{
	//SOCKADDR_IN clientaddr;
	//int addrlen;
	//char buf1[BUFSIZE+1];
	clientNode* client = (clientNode*)arg;
	int retval;
	char buf2[BUFSIZE+1];

	// 클라이언트 정보 얻기
	//addrlen = sizeof(clientaddr);
	//getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	client->addrlen = sizeof(client->addrlen);
	getpeername(client->client_sock, (SOCKADDR *)(&(client->clientaddr)), &(client->addrlen));


	while(1){
		// 데이터 받기
		//retval = recv(client_sock, buf2, BUFSIZE, 0);
		retval = recv(client->client_sock, buf2, BUFSIZE, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		} else if(retval == 0)break;

		// 받은 데이터 출력
		buf2[retval] = '\0';
		printf("[TCP/%s:%d] %s\n", inet_ntoa(client->clientaddr.sin_addr), ntohs(client->clientaddr.sin_port), buf2);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	/*
	SOCKET client_sock[2];
	SOCKADDR_IN clientaddr[2];
	int addrlen[2];
	*/
	clientNode* clients[100];
	HANDLE sendThread[100];
	HANDLE recvThread[100];
	int i=0;
	//while(1){printf("d");}

	while(1){
		// accept()
		/*
		addrlen[i] = sizeof(clientaddr);
		client_sock[i] =accept(listen_sock, (SOCKADDR *)&clientaddr[i], &addrlen[i]);
		*/
		clients[i] = (clientNode*)malloc(sizeof(clientNode));
		clients[i]->addrlen = sizeof(SOCKADDR_IN);
		clients[i]->client_sock = accept(listen_sock, (SOCKADDR *)&(clients[i]->clientaddr), &clients[i]->addrlen);

		if(clients[i]->client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clients[i]->clientaddr.sin_addr), ntohs(clients[i]->clientaddr.sin_port));

		// 스레드 생성
		sendThread[i] = CreateThread(NULL, 0, SendClient, (LPVOID)clients[i], 0, NULL);
		recvThread[i] = CreateThread(NULL, 0, RecvClient, (LPVOID)&clients[i], 0, NULL);
		if(sendThread == NULL && recvThread == NULL) { closesocket(clients[i]->client_sock); }
		//else { CloseHandle(recvThread[0]);CloseHandle(sendThread[0]);CloseHandle(recvThread[1]);CloseHandle(sendThread[1]); }
		i++;
	}
	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
