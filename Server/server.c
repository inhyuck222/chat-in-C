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
	clientNode* link[3];
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

DWORD WINAPI serverProcess(LPVOID arg){
	clientNode* client = (clientNode*)arg;

	int retval;
	char buf[BUFSIZE+1];
	int numOfLink = 0;
	client->addrlen = sizeof(client->clientaddr);
	getpeername(client->client_sock, (SOCKADDR *)(&(client->clientaddr)), &(client->addrlen));

	while(1){
		retval = recv(client->client_sock, buf, BUFSIZE, 0);

		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		} else if(retval == 0)break;

		buf[retval] = '\0';
		//retval = send(client->link->client_sock, buf, strlen(buf), 0);
		while(client->link[numOfLink] != NULL && numOfLink < 2){
			retval = send(client->link[numOfLink++]->client_sock, buf, strlen(buf), 0);
		}
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
		numOfLink = 0;
		printf("send end\n");
	}
	closesocket(client->client_sock);
	return 0;
}

int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)return 1;

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
	clientNode* clients[3];
	HANDLE chatThread[3];
	int numOfClient=0;
	int j=0;
	int numOfLink = 0;
//	int numOfClient = 0;
	for(int a=0; a<3; a++)clients[a] = NULL;
	
	while(1){
		// accept()
		clients[numOfClient] = (clientNode*)malloc(sizeof(clientNode));
		clients[numOfClient]->addrlen = sizeof(SOCKADDR_IN);
		clients[numOfClient]->client_sock = accept(listen_sock, (SOCKADDR *)&(clients[numOfClient]->clientaddr), &clients[numOfClient]->addrlen);
//		numOfClient++;//클라이언트 개수 증가

		while(clients[j] != NULL && j < 3){
			for(int k=0; k<3; k++){
				if(clients[k] != NULL && clients[j]->client_sock != clients[k]->client_sock){
					clients[j]->link[numOfLink] = clients[k];
					numOfLink++;
				}
			}
			numOfLink = 0;
			j++;
		}
		j=0;

		if(clients[numOfClient]->client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clients[numOfClient]->clientaddr.sin_addr), ntohs(clients[numOfClient]->clientaddr.sin_port));
		chatThread[numOfClient] = CreateThread(NULL, 0, serverProcess, (LPVOID)clients[numOfClient], 0, NULL);
		if(chatThread[numOfClient] == NULL) { closesocket(clients[numOfClient]->client_sock); }
		else { CloseHandle(chatThread[numOfClient]); }
		numOfClient++;
	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
