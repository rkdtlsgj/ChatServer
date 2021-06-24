#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include <wchar.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include <list>

#include "CRingBuffer.h"
#include "CPacket.h"
#include "Protocol.h"
#include "Client.h"


using namespace std;
struct  stSession
{
	SOCKADDR_IN sockAddr;
	SOCKET socket;
	
	CRingBuffer cSendQ;
	CRingBuffer cRecvQ;
};

struct stClient
{
	WCHAR cClientName[dfNICK_MAX_LEN];
	DWORD dwClinetNumber;
	DWORD dwRoomNumber;
};

struct stRoom
{
	DWORD dwRoomNumber;
	WCHAR cRoomName[dfROOM_MAX_NAME];

	list<DWORD> UserList;
	
};

SOCKET listen_sock;
unordered_map<DWORD, stSession* > g_SessionInfo;
unordered_map<DWORD, stClient* > g_ClientInfo;

unordered_map<DWORD, stRoom* > g_RoomInfo;

DWORD g_dwKey_UserNumber = 1;
DWORD g_dwKey_RoomNumber = 1;

BOOL InitServer();
BOOL OpenServer();
void NetworkProcess();
void SelectSocket(DWORD* dwpUserNumber, SOCKET* pUserSocket, FD_SET* pReadSet, FD_SET* pWriteSet);

void NetAccept_Proc();
void NetRecv_Proc(DWORD dwUserNumber);
void NetSend_Proc(DWORD dwUserNumber);
void Send_ResRoomCreate(DWORD dwUserNumber, BYTE byResult, stRoom* pRoom);
void MakePacket_RoomCreate(st_PACKET_HEADER* pHeader, CPacket* pPacket, BYTE byResult, stRoom* pRoom);


int CompleteRecvPacket(stSession* pClinet, DWORD dwUserNumber);
void SendUnicast(stSession* pSession, st_PACKET_HEADER* pHeader, CPacket* pPakcet);
void SendBroadcast(st_PACKET_HEADER* pHeader, CPacket* pPakcet);
void SendBroadcast_Room(stRoom* pRoom, stClient* pClient, st_PACKET_HEADER* pHeader, CPacket* pPakcet);
void SendBroadcast(stSession* pExSession, st_PACKET_HEADER* pHeader, CPacket* pPakcet);

BOOL netPacket_ReqRoomCreate(DWORD dwUserNumber, CPacket* pPacket);

BOOL packetProc_ReqLogin(stClient* pClient, stSession* pSession, CPacket* pPacket);
BOOL packetProc_ResLogin(stClient* pClient, stSession* pSession, BYTE byResult);

BOOL packetProc_ReqRoomList(stSession* pSession, CPacket* pPakcet);
BOOL packetProc_ResRoomList(stSession* pSession);

BOOL packetProc_ReqEcho(stSession* pSession, CPacket* pPacket);
BOOL packetProc_ResEcho(stSession* pSession, CPacket* pPacket);

BOOL packetProc_ReqRoomEnter(stClient* pClient, stSession* pSession, CPacket* pPacket);
BOOL packetProc_ResRoomEnter(stClient* pClient, stSession* pSession, stRoom* pRoom, BYTE byResult);
void MakePacket_RoomEnter(stClient* pClient,st_PACKET_HEADER* pHeader, CPacket* pPacket, BYTE byResult, stRoom* pRoom);

BOOL packetProc_ReqChat(DWORD dwUserNumber, stSession* pSession, CPacket* pPacket);
BOOL packetProc_ResChat(DWORD dwUserNumber, stSession* pSession, CPacket* pPacket);

BOOL packetProc_ResUserEnter(stClient* pClient, stRoom* pRoom);

stClient* FindClient(DWORD dwUserNumber);
stSession* FindSession(DWORD dwUserNumber);

BYTE MakeCheckSum(CPacket* pPacket, WORD dwType);
void AddClient(stClient* pClient, stSession* pSession);
void AddRoom(stRoom* pRoom);



void main()
{
	if (InitServer() == false)
		return;

	if (OpenServer() == false)
		return;

	while (1)
	{
		NetworkProcess();
	}

	return;
}

BOOL InitServer()
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return FALSE;

	//SOCKET 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		return FALSE;

	//Bind
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(dfNETWORK_PORT);
	retval = bind(listen_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
		return FALSE;

	wprintf(L"Init Server\n");
	return TRUE;
}

BOOL OpenServer()
{
	int retval;

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)	return FALSE;

	wprintf(L"Open Server\n");
	return TRUE;
}

void NetworkProcess()
{
	DWORD userNumber[FD_SETSIZE];
	SOCKET userSocket[FD_SETSIZE];
	int sockCount = 0;

	unordered_map < DWORD, stSession*>::iterator userIter;

	FD_SET readSet;
	FD_SET writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	memset(userNumber, -1, sizeof(DWORD) * FD_SETSIZE);
	memset(userSocket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);

	
	FD_SET(listen_sock, &readSet);


	userNumber[sockCount] = sockCount;
	userSocket[sockCount] = listen_sock;	

	sockCount++;

	for (userIter = g_SessionInfo.begin(); userIter != g_SessionInfo.end(); ++userIter)
	{		
		if (userIter->second->socket != INVALID_SOCKET)
		{

			userNumber[sockCount] = g_ClientInfo[userIter->first]->dwClinetNumber;
			userSocket[sockCount] = userIter->second->socket;

			if (userIter->second->cSendQ.GetUseSize() > 0)
			{
				FD_SET(userIter->second->socket, &writeSet);
			}

			FD_SET(userIter->second->socket, &readSet);

			sockCount++;


			if (FD_SETSIZE <= sockCount)
			{
				SelectSocket(userNumber, userSocket, &readSet, &writeSet);
				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				memset(userNumber, -1, sizeof(DWORD) * FD_SETSIZE);
				memset(userSocket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
				sockCount = 0;
			}
		}
	}

	if (sockCount > 0)
	{
		SelectSocket(userNumber, userSocket, &readSet, &writeSet);
	}
}

void SelectSocket(DWORD* dwpUserNumber, SOCKET* pUserSocket, FD_SET* pReadSet, FD_SET* pWriteSet)
{
	timeval Time;
	int iResult;

	Time.tv_sec = 0;
	Time.tv_usec = 0;

	iResult = select(0, pReadSet, pWriteSet, 0, &Time);

	if (0 < iResult)
	{
		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (pUserSocket[i] == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pUserSocket[i], pWriteSet))
			{
				//전송
				NetSend_Proc(dwpUserNumber[i]);
			}

			if (FD_ISSET(pUserSocket[i], pReadSet))
			{
				//리슨소켓
				if (dwpUserNumber[i] == 0)					
				{
					//Accept
					NetAccept_Proc();
				}
				else
				{
					//Recv
					NetRecv_Proc(dwpUserNumber[i]);
				}

			}
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		//에러처리?
	}
}

void NetRecv_Proc(DWORD dwUserNumber)
{
	stSession* pSession = FindSession(dwUserNumber);
	if (pSession == NULL)
		return;

	int iResult = recv(pSession->socket, pSession->cRecvQ.GetRearBufferPtr(), pSession->cRecvQ.DirectEnqueueSize(), 0);
	//int iResult = recv(pClient->socket, RecvTest, dfRECV_BUFF, 0);

	if (iResult == SOCKET_ERROR || iResult == 0)
	{
		closesocket(pSession->socket);
		wprintf(L" Socket Close User No : %d\n", g_ClientInfo[dwUserNumber]->dwClinetNumber);
		return;
	}

	//pClient->cRecvQ.Enqueue(RecvTest, iResult);

	pSession->cRecvQ.MoveRear(iResult);

	if (0 < iResult)
	{		
		while (1)
		{
			
			iResult = CompleteRecvPacket(pSession, dwUserNumber);

			if (iResult == 1)
				break;

			if (iResult == -1)
			{
				wprintf(L" Packet Error User No : %d\n", g_ClientInfo[dwUserNumber]->dwClinetNumber);
				return;
			}
		}
	}
}

void NetSend_Proc(DWORD dwUserNumber)
{
	stSession* pClient = FindSession(dwUserNumber);
	if (pClient == NULL)
		return;


	int iSendSize = pClient->cSendQ.DirectDequeueSize();

	if (iSendSize <= 0)
		return;


	int iResult = send(pClient->socket, pClient->cSendQ.GetFrontBufferPtr(), iSendSize, 0);

	if (iResult == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		if (dwError == WSAEWOULDBLOCK)
		{
			wprintf(L"Socket WOULDBLOCK user Number : %d\n", dwUserNumber);
			return;
		}

		closesocket(pClient->socket);
		wprintf(L" Socker Close User No : %d\n", g_ClientInfo[dwUserNumber]->dwClinetNumber);

		return;
	}
	else
	{
		pClient->cSendQ.MoveFront(iResult);
	}

	return;

}

BOOL PacketProc(stSession* pSession, DWORD dwUserNumber, WORD wMsgType, CPacket* pPacket)
{
	//wprintf(L"Packet Recv User Number : %d / Type : %d\n", pClient->dwClinetNumber, wMsgType);

	stClient* pClient = FindClient(dwUserNumber);
	switch (wMsgType)
	{
	case df_REQ_LOGIN:
		return packetProc_ReqLogin(pClient, pSession, pPacket);
		break;
	case df_REQ_ROOM_LIST:
		return packetProc_ReqRoomList(pSession, pPacket);
		break;
	case df_REQ_ROOM_CREATE:
		return netPacket_ReqRoomCreate(dwUserNumber, pPacket);
		break;
	case df_REQ_ROOM_ENTER:
		return packetProc_ReqRoomEnter(pClient,pSession, pPacket);
		break;
	case df_REQ_CHAT:
		return packetProc_ReqChat(dwUserNumber, pSession, pPacket);
		break;
	case df_REQ_ROOM_LEAVE:
		break;
	case df_REQ_STRESS_ECHO:
		return packetProc_ReqEcho(pSession, pPacket);
		break;
	}

	return TRUE;
}

void NetAccept_Proc()
{
	int addrlen = sizeof(SOCKADDR_IN);
	stSession* pSession = new stSession;
	stClient* pClient = new stClient;

	pClient->dwClinetNumber = g_dwKey_UserNumber;
	memset(pClient->cClientName, '\0', dfNICK_MAX_LEN *2);

	pSession->socket = accept(listen_sock, (SOCKADDR*)&pSession->sockAddr, &addrlen);
	if (pSession->socket == INVALID_SOCKET)
	{
		wprintf(L"Accept Error\n");
		return;
	}

	AddClient(pClient, pSession);

	WCHAR wcAddrs[20];
	InetNtop(AF_INET, &pSession->sockAddr.sin_addr, wcAddrs, sizeof(wcAddrs));


	wprintf(L"[Accept] - %s:%d Number : %d\n", wcAddrs, ntohs(pSession->sockAddr.sin_port), pClient->dwClinetNumber);
}

BOOL netPacket_ReqRoomCreate(DWORD dwUserNumber, CPacket* pPacket)
{
	WCHAR szRoomTitle[dfROOM_MAX_NAME] = { 0, };
	WORD wTitleSize;


	*pPacket >> wTitleSize;
	pPacket->GetData((char*)szRoomTitle, wTitleSize);


	stRoom* pRoom = new stRoom;
	memset(pRoom->cRoomName, 0, sizeof(WCHAR) * dfROOM_MAX_NAME);

	pRoom->dwRoomNumber = g_dwKey_RoomNumber;
	wcscpy_s(pRoom->cRoomName, szRoomTitle);

	AddRoom(pRoom);

	wprintf(L"[ROOM Create Req] User Number : %d / Room Name : %s / Totle Room : %d\n", FindClient(dwUserNumber)->dwClinetNumber, pRoom->cRoomName, g_RoomInfo.size());

	Send_ResRoomCreate(dwUserNumber, df_RESULT_ROOM_CREATE_OK, pRoom);

	return TRUE;
}

void Send_ResRoomCreate(DWORD dwUserNumber, BYTE byResult, stRoom* pRoom)
{
	st_PACKET_HEADER stHeader;
	CPacket packet;


	MakePacket_RoomCreate(&stHeader, &packet, byResult, pRoom);


	if (byResult == df_RESULT_ROOM_CREATE_OK)
	{
		SendBroadcast(&stHeader, &packet);
	}
	else
	{
		SendUnicast(FindSession(dwUserNumber),&stHeader, &packet);
	}
}

void MakePacket_RoomCreate(st_PACKET_HEADER* pHeader, CPacket* pPacket, BYTE byResult, stRoom* pRoom)
{
	WORD wTitleSize;

	pPacket->Clear();
	pPacket->PutData((char*)&byResult, sizeof(BYTE));

	if (byResult == df_RESULT_ROOM_CREATE_OK)
	{
		*pPacket << pRoom->dwRoomNumber;
		wTitleSize = wcslen(pRoom->cRoomName) * sizeof(WCHAR);
		*pPacket << wTitleSize;
		pPacket->PutData((char*)pRoom->cRoomName, wTitleSize);
	}

	pHeader->byCode = dfPACKET_CODE;
	pHeader->byCheckSum = MakeCheckSum(pPacket, df_RES_ROOM_CREATE);
	pHeader->wMsgType = df_RES_ROOM_CREATE;
	pHeader->wPayloadSize = pPacket->GetDataSize();
}

BOOL packetProc_ReqLogin(stClient* pClient, stSession* pSession, CPacket* pPacket)
{
	unordered_map<DWORD, stClient*>::iterator clientIter;
	BYTE byResult = df_RESULT_LOGIN_OK;

	WCHAR wNickName[dfNICK_MAX_LEN];
	pPacket->GetData((char*)&wNickName, dfNICK_MAX_LEN * 2);

	for (clientIter = g_ClientInfo.begin(); clientIter != g_ClientInfo.end(); ++clientIter)
	{
		if (wcscmp(clientIter->second->cClientName, wNickName)== 0)
			byResult = df_RESULT_LOGIN_DNICK;
	}

	if (byResult == 1)	
		memcpy(pClient->cClientName, wNickName, dfNICK_MAX_LEN * 2);

	return packetProc_ResLogin(pClient, pSession,byResult);
}

BOOL packetProc_ResLogin(stClient* pClient,stSession* pSession, BYTE byResult)
{
	st_PACKET_HEADER header;
	CPacket cPacket;

	cPacket << byResult;
	cPacket << pClient->dwClinetNumber;

	header.byCode = dfPACKET_CODE;
	header.wMsgType = df_RES_LOGIN;
	header.wPayloadSize = (WORD)cPacket.GetDataSize();
	header.byCheckSum = MakeCheckSum(&cPacket,df_RES_LOGIN);

	SendUnicast(pSession, &header, &cPacket);
	return TRUE;
}
BOOL packetProc_ReqChat(DWORD dwUserNumber, stSession* pSession, CPacket* pPacket)
{
	return packetProc_ResChat(dwUserNumber, pSession, pPacket);
}

BOOL packetProc_ResChat(DWORD dwUserNumber,stSession* pSession, CPacket* pPacket)
{
	st_PACKET_HEADER header;
	CPacket cPacket;

	WORD wMsgSize = 0;
	WCHAR wcMsg[dfRECV_BUFF];
	memset(wcMsg, '\0', dfRECV_BUFF);

	*pPacket >> wMsgSize;
	pPacket->GetData((char*)wcMsg, wMsgSize);


	cPacket << dwUserNumber;
	cPacket << wMsgSize;
	cPacket.PutData((char*)wcMsg, wMsgSize);

	header.byCode = dfPACKET_CODE;
	header.wMsgType = df_RES_CHAT;
	header.wPayloadSize = (WORD)cPacket.GetDataSize();
	header.byCheckSum = MakeCheckSum(&cPacket, df_RES_CHAT);

	unordered_map < DWORD, stClient*>::iterator userIter = g_ClientInfo.find(dwUserNumber);
	unordered_map < DWORD, stRoom*>::iterator pRoomInfoIter = g_RoomInfo.find(userIter->second->dwRoomNumber);
	SendBroadcast_Room(pRoomInfoIter->second, userIter->second, &header, &cPacket);

	return TRUE;
}

BOOL packetProc_ReqRoomEnter(stClient* pClient,stSession* pSession, CPacket* pPacket)
{
	DWORD dwRoomNumber = -1;
	*pPacket >> dwRoomNumber;

	BYTE byResult = df_RESULT_ROOM_ENTER_OK;
	if (g_RoomInfo[dwRoomNumber] == NULL)
	{
		byResult = df_RESULT_ROOM_ENTER_NOT;
	}
	else if (g_RoomInfo[dwRoomNumber]->UserList.size() >= dfROOM_MAX_COUNT)
	{
		byResult = df_RESULT_ROOM_ENTER_MAX;
	}

	wprintf(L"[ROOM Enter Req] User Number : %d / Room Number : %d\n", pClient->dwClinetNumber, dwRoomNumber);

	if (packetProc_ResRoomEnter(pClient, pSession, g_RoomInfo[dwRoomNumber], byResult) == TRUE)
	{
		packetProc_ResUserEnter(pClient, g_RoomInfo[dwRoomNumber]);
	}

	return TRUE;
}

BOOL packetProc_ResRoomEnter(stClient* pClient, stSession* pSession, stRoom* pRoom,BYTE byResult)
{
	st_PACKET_HEADER stHeader;
	CPacket packet;

	MakePacket_RoomEnter(pClient ,&stHeader, &packet, byResult, pRoom);

	SendUnicast(pSession, &stHeader, &packet);

	return TRUE;
}

BOOL packetProc_ResUserEnter(stClient* pClient, stRoom* pRoom)
{
	st_PACKET_HEADER stHeader;
	CPacket packet;


	packet.PutData((char*)pClient->cClientName, dfNICK_MAX_LEN * 2);
	packet << pClient->dwClinetNumber;


	stHeader.byCode = dfPACKET_CODE;
	stHeader.byCheckSum = MakeCheckSum(&packet, df_RES_USER_ENTER);
	stHeader.wMsgType = df_RES_USER_ENTER;
	stHeader.wPayloadSize = packet.GetDataSize();

	SendBroadcast_Room(pRoom, pClient, &stHeader, &packet);

	return TRUE;
}

void MakePacket_RoomEnter(stClient* pClient, st_PACKET_HEADER* pHeader, CPacket* pPacket, BYTE byResult, stRoom* pRoom)
{
	WORD wTitleSize;
	
	pPacket->PutData((char*)& byResult, sizeof(BYTE));

	if (byResult == df_RESULT_ROOM_ENTER_OK)
	{
		pClient->dwRoomNumber = pRoom->dwRoomNumber;
		pRoom->UserList.push_front(pClient->dwClinetNumber);

		*pPacket << pRoom->dwRoomNumber;
		wTitleSize = wcslen(pRoom->cRoomName) * sizeof(WCHAR);
		*pPacket << wTitleSize;
		pPacket->PutData((char*)pRoom->cRoomName, wTitleSize);
		BYTE usercount = pRoom->UserList.size();
		*pPacket << usercount;
		
		list<DWORD>::iterator roomUserIter;
		for (roomUserIter = pRoom->UserList.begin(); roomUserIter != pRoom->UserList.end(); ++roomUserIter)
		{
			stClient* user = FindClient(*roomUserIter);				
			pPacket->PutData((char*)user->cClientName, dfNICK_MAX_LEN*2);
			*pPacket << user->dwClinetNumber;		
		}		
	}

	pHeader->byCode = dfPACKET_CODE;
	pHeader->byCheckSum = MakeCheckSum(pPacket, df_RES_ROOM_ENTER);
	pHeader->wMsgType = df_RES_ROOM_ENTER;
	pHeader->wPayloadSize = pPacket->GetDataSize();

	wprintf(L"[ROOM Enter Res] RoomName : %s / Room Number : %d\n", pRoom->cRoomName, pRoom->dwRoomNumber);
}

BOOL packetProc_ReqEcho(stSession* pSession, CPacket* pPacket)
{
	return packetProc_ResEcho(pSession, pPacket);
}

BOOL packetProc_ResEcho(stSession* pSession, CPacket* pPacket)
{
	st_PACKET_HEADER header;
	CPacket cPacket;

	WORD size;

	WCHAR wData[900];

	*pPacket >> size;

	pPacket->GetData((char*)&wData, size);

	cPacket << size;
	cPacket.PutData((char*)wData, size);

	header.byCode = dfPACKET_CODE;
	header.wMsgType = df_RES_STRESS_ECHO;
	header.wPayloadSize = (WORD)cPacket.GetDataSize();
	header.byCheckSum = MakeCheckSum(&cPacket, df_RES_STRESS_ECHO);

	SendUnicast(pSession, &header, &cPacket);
	return TRUE;
}


BOOL packetProc_ReqRoomList(stSession* pSession, CPacket* pPakcet)
{
	return packetProc_ResRoomList(pSession);
}


BOOL packetProc_ResRoomList(stSession* pSession)
{
	unordered_map<DWORD, stRoom*>::iterator roomIter;
	list<DWORD>::iterator clientIter;	

	CPacket packet;
	st_PACKET_HEADER header;

	WORD wTitleSize;
	WORD wClientSize;
	WORD wRoomCount = g_RoomInfo.size();
	packet << wRoomCount;

	for (roomIter = g_RoomInfo.begin(); roomIter != g_RoomInfo.end(); ++roomIter)
	{
		packet << roomIter->second->dwRoomNumber;		
		wTitleSize = wcslen(roomIter->second->cRoomName) * sizeof(WCHAR);
		packet << wTitleSize;
		packet.PutData((char*)roomIter->second->cRoomName, wTitleSize);

		for (clientIter = roomIter->second->UserList.begin(); clientIter != roomIter->second->UserList.end(); ++clientIter)
		{
			stClient* client = FindClient(*clientIter);

			wClientSize = wcslen(client->cClientName) * sizeof(WCHAR);
			packet << wTitleSize;
			packet.PutData((char*)client->cClientName, wTitleSize);
		}
	}

	header.byCode = dfPACKET_CODE;
	header.wMsgType = df_RES_ROOM_LIST;
	header.wPayloadSize = (WORD)packet.GetDataSize();
	header.byCheckSum = MakeCheckSum(&packet,df_RES_ROOM_LIST);

	SendUnicast(pSession, &header, &packet);

	return TRUE;
}
int CompleteRecvPacket(stSession* pSession,DWORD dwUserNumber)
{
	st_PACKET_HEADER stHeader;

	int qSize = pSession->cRecvQ.GetUseSize();

	if (sizeof(st_PACKET_HEADER) > qSize)
		return 1;


	pSession->cRecvQ.Peek((char*)&stHeader, sizeof(st_PACKET_HEADER));


	if (stHeader.byCode != dfPACKET_CODE)
		return 0xff;

	if (stHeader.wPayloadSize + sizeof(st_PACKET_HEADER) > (WORD)qSize)
	{
		//아직 패킷을 다  받지 못하였다
		return 1;
	}


	pSession->cRecvQ.MoveFront(sizeof(st_PACKET_HEADER));


	CPacket cPacket;

	if (stHeader.wPayloadSize != pSession->cRecvQ.Dequeue(cPacket.GetBufferPtr(), stHeader.wPayloadSize))
	{
		return -1;
	}

	
	cPacket.MoveWritePos(stHeader.wPayloadSize);

	BYTE byCheckSum = MakeCheckSum(&cPacket, stHeader.wMsgType);
	if (byCheckSum != stHeader.byCheckSum)
	{
		wprintf(L"CheckSum ERROR User No :%d\n ", pSession->sockAddr.sin_addr);
		return -1;
	}

	//패킷처리
	if (PacketProc(pSession, dwUserNumber, stHeader.wMsgType, &cPacket) == false)
		return -1;

	return 0;

}


void SendUnicast(stSession* pSession, st_PACKET_HEADER* pHeader, CPacket* pPakcet)
{
	pSession->cSendQ.Enqueue((char*)pHeader, sizeof(st_PACKET_HEADER));
	pSession->cSendQ.Enqueue((char*)pPakcet->GetBufferPtr(), pPakcet->GetDataSize());
}

void SendBroadcast(st_PACKET_HEADER* pHeader, CPacket* pPakcet)
{
	stSession* pClient;

	unordered_map<DWORD, stSession*>::iterator clientIter;
	for (clientIter = g_SessionInfo.begin(); clientIter != g_SessionInfo.end(); ++clientIter)
	{
		pClient = clientIter->second;
		SendUnicast(pClient, pHeader, pPakcet);
	}
}

void SendBroadcast(stSession* pExSession, st_PACKET_HEADER* pHeader, CPacket* pPakcet)
{
	stSession* pSession;

	unordered_map<DWORD, stSession*>::iterator sessionIter;
	for (sessionIter = g_SessionInfo.begin(); sessionIter != g_SessionInfo.end(); ++sessionIter)
	{
		pSession = sessionIter->second;

		if(pExSession != pSession)
			SendUnicast(pSession, pHeader, pPakcet);
	}
}

void SendBroadcast_Room(stRoom* pRoom,stClient* pClient, st_PACKET_HEADER* pHeader, CPacket* pPakcet)
{
	list<DWORD>::iterator iter;
	DWORD dwSubKey = -1;

	for (iter = pRoom->UserList.begin(); iter != pRoom->UserList.end(); ++iter)
	{
		dwSubKey = FindClient(*iter)->dwClinetNumber;

		if(pClient->dwClinetNumber != dwSubKey)
			SendUnicast(g_SessionInfo[dwSubKey], pHeader, pPakcet);
	}
}

stClient* FindClient(DWORD dwUserNumber)
{
	unordered_map < DWORD, stClient*>::iterator userIter = g_ClientInfo.find(dwUserNumber);

	return userIter->second;
}

stSession* FindSession(DWORD dwUserNumber)
{
	unordered_map < DWORD, stSession*>::iterator userIter = g_SessionInfo.find(dwUserNumber);

	return userIter->second;
}

void AddClient(stClient* pClient,stSession* pSession)
{
	g_ClientInfo.insert(make_pair(g_dwKey_UserNumber, pClient));
	g_SessionInfo.insert(make_pair(g_dwKey_UserNumber, pSession));

	g_dwKey_UserNumber++;
}


void AddRoom(stRoom* pRoom)
{
	g_RoomInfo.insert(make_pair(g_dwKey_RoomNumber, pRoom));
	g_dwKey_RoomNumber++;
}


BYTE MakeCheckSum(CPacket* pPacket, WORD dwType)
{
	int iSize = pPacket->GetDataSize();
	BYTE* pPtr = (BYTE*)pPacket->GetBufferPtr();
	int iCheckSum = dwType;

	for (int i = 0; i < iSize; i++)
	{
		iCheckSum += *pPtr;
		pPtr++;
	}

	return (BYTE)(iCheckSum % 256);
}