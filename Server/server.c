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
	int clientNum;
	int addrlen;
	clientNode* link[5];
}clientNode;

typedef struct roomNode{
	clientNode* roomClient[5];
	int roomNumber;
	int numOfCinRoom;
}roomNode;

DWORD WINAPI serverProcess(LPVOID arg);
DWORD WINAPI roomProcess(LPVOID arg);
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

DWORD WINAPI roomProcess(LPVOID arg){
	roomNode* room = (roomNode*)arg;
	clientNode* client = room->roomClient[0];
	HANDLE chatThread;
	int numOfClient = 0;
	int retval;
	for(int a=0; a<3; a++){
		chatThread = CreateThread(NULL, 0, serverProcess, (LPVOID)room->roomClient[a], 0, NULL);
		if(chatThread == NULL) { closesocket(room->roomClient[a]->client_sock); }
		else { CloseHandle(chatThread); }
	}

	/*while(1){
		
		printf("R : send start\n");
		//if(room->roomClient[numOfClient] != NULL)
		retval = send(client->client_sock, check, strlen(check), 0);
		printf("R : send end\n");
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
		printf("R : recv start\n");
		retval = recv(client->client_sock, check, BUFSIZE, 0);
		printf("R : recv end\n");
		chatThread[numOfClient] = CreateThread(NULL, 0, serverProcess, (LPVOID)room->roomClient[numOfClient], 0, NULL);
		if(chatThread[numOfClient] == NULL) { closesocket(room->roomClient[numOfClient]->client_sock); }
		else { CloseHandle(chatThread[numOfClient]); }
		numOfClient++;
		if(numOfClient == 5)break;
		
	}*/
	return 0;
}


DWORD WINAPI serverProcess(LPVOID arg){
	clientNode* client = (clientNode*)arg;

	int retval;
	char buf[BUFSIZE+1];
	int numOfLink = 0;
	client->addrlen = sizeof(client->clientaddr);
	getpeername(client->client_sock, (SOCKADDR *)(&(client->clientaddr)), &(client->addrlen));


	while(1){
		printf("P : recv start(%d)\n",client->clientNum);
		retval = recv(client->client_sock, buf, BUFSIZE, 0);
		printf("P : recv end(%d)\n",client->clientNum);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		} else if(retval == 0)break;

		buf[retval] = '\0';
		printf("P : send start(%d)\n",client->clientNum);
		//retval = send(client->link->client_sock, buf, strlen(buf), 0);
		while(client->link[numOfLink] != NULL && numOfLink < 4){
			retval = send(client->link[numOfLink++]->client_sock, buf, strlen(buf), 0);
			printf("P : send end(%d)\n",client->clientNum);
		}
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
		numOfLink = 0;
		
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
	clientNode* clients[25];
	roomNode* room[5];
	HANDLE chatThread[5];
	int numOfClient=0;
	int numOfThread=0;
	int j=0;
	int numOfLink = 0;
	int selectRoomNum;
	char sendRoom[33] = "방을 선택하세요(0~4)\n";	
	char recvRoomNum[BUFSIZE+1];
	bool isCreated[5];
	for(int a=0; a<5; a++)isCreated[a] = false;
	for(int a=0; a<25; a++)clients[a] = NULL;
		
	for(int a=0; a<5; a++){
		room[a] = (roomNode*)malloc(sizeof(roomNode));
		for(int b=0; b<5; b++)room[a]->roomClient[b] = NULL;
		room[a]->roomNumber = a;
		room[a]->numOfCinRoom = 0;
	}

	while(1){
		// accept()
		clients[numOfClient] = (clientNode*)malloc(sizeof(clientNode));
		clients[numOfClient]->clientNum = numOfClient;
		for(int a=0; a<5; a++)clients[numOfClient]->link[a] = NULL;
		clients[numOfClient]->addrlen = sizeof(SOCKADDR_IN);
		clients[numOfClient]->client_sock = accept(listen_sock, (SOCKADDR *)&(clients[numOfClient]->clientaddr), &clients[numOfClient]->addrlen);
		if(clients[numOfClient]->client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}

		retval = send(clients[numOfClient]->client_sock, sendRoom, strlen(sendRoom), 0);
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		retval = recv(clients[numOfClient]->client_sock, recvRoomNum, BUFSIZE, 0);
		recvRoomNum[retval] = '\0';
		selectRoomNum = recvRoomNum[0] - '0';

		for(int a=0; a<5; a++){
			if(room[a]->roomNumber == selectRoomNum){
				room[a]->roomClient[room[a]->numOfCinRoom++] = clients[numOfClient];
				while(room[a]->roomClient[j] != NULL && j < 5){
					for(int k=0; k<5; k++){
						if(room[a]->roomClient[k] != NULL && room[a]->roomClient[j]->client_sock != room[a]->roomClient[k]->client_sock){
							room[a]->roomClient[j]->link[numOfLink] = room[a]->roomClient[k];
							numOfLink++;
						}
					}
					numOfLink = 0;
					j++;
				}
				j=0;
			}
		}

		/*
		while(clients[j] != NULL && j < 5){
		for(int k=0; k<5; k++){
		if(clients[k] != NULL && clients[j]->client_sock != clients[k]->client_sock){
		clients[j]->link[numOfLink] = clients[k];
		numOfLink++;
		}
		}
		numOfLink = 0;
		j++;
		}
		j=0;
		*/

		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clients[numOfClient]->clientaddr.sin_addr), ntohs(clients[numOfClient]->clientaddr.sin_port));

		/*
		if(isCreated[selectRoomNum] != true && numOfThread < 5){
			isCreated[selectRoomNum] = true;
			chatThread[numOfThread++] = CreateThread(NULL, 0, roomProcess, (LPVOID)room[selectRoomNum], 0, NULL);
		}
		*/
		if(room[selectRoomNum]->numOfCinRoom == 3 && numOfThread < 5){
			//isCreated[selectRoomNum] = true;
			chatThread[numOfThread++] = CreateThread(NULL, 0, roomProcess, (LPVOID)room[selectRoomNum], 0, NULL);
		}
		if(chatThread[numOfThread] == NULL) { closesocket(clients[numOfClient]->client_sock); }
//		else { CloseHandle(chatThread[numOfThread-1]); }

		numOfClient++;
	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
