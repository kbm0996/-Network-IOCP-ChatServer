#include "ChatServer.h"

CChatServer::CChatServer() :_lUpdateTps(0), _lPlayerCnt(0),_lSessionMissCnt(0), _bShutdown(false), _bControlMode(false)
{
	ZeroMemory(&_szSessionKey, sizeof(_szSessionKey));
	LOG_SET(CSystemLog::en_CONSOLE | CSystemLog::en_FILE, LOG_DEBUG, L"CHAT_SERVER_LOG");
	CNPacket::CNPacket(200);

	_pLoginClient = new CLoginClient();
	if (!pConfig->LoadConfigFile("_ChatServer.cnf"))
		CRASH();
}

CChatServer::~CChatServer()
{
	delete _pLoginClient;
}

bool CChatServer::Start()
{
	_hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hUpdateThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, UpdateThread, this, 0, NULL));
	
	if (!CNetServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, CConfig::_iWorkerThreadNo, false, CConfig::_iClientMax, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
		return false;

	return true;
}

void CChatServer::Stop()
{
	_bShutdown = true;

	SetEvent(_hUpdateEvent);
	WaitForSingleObject(_hUpdateThread, INFINITE);
	CloseHandle(_hUpdateThread);
	CloseHandle(_hUpdateEvent);

	CNetServer::Stop();

	if(_pLoginClient->_bConnect)
		_pLoginClient->Stop();
}

void CChatServer::ServerControl()
{
	//------------------------------------------
	// L : Control Lock / U : Unlock / Q : Quit
	//------------------------------------------
	//  _kbhit() �Լ� ��ü�� ������ ������ ����� Ȥ�� ���̰� �������� �������� ���� �׽�Ʈ�� �ּ�ó�� ����
	// �׷����� GetAsyncKeyState�� �� �� ������ â�� Ȱ��ȭ���� �ʾƵ� Ű�� �ν��� Windowapi�� ��� 
	// ��� �����ϳ� �ֿܼ��� �����

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			_bControlMode = true;

			wprintf(L"[ Control Mode ] \n");
			wprintf(L"Press  L	- Key Lock \n");
			wprintf(L"Press  Q	- Quit \n");
		}

		if (_bControlMode == true)
		{
			if (L'l' == ControlKey || L'L' == ControlKey)
			{
				wprintf(L"Controll Lock. Press U - Control Unlock \n");
				_bControlMode = false;
			}

			if (L'q' == ControlKey || L'Q' == ControlKey)
			{
				_bShutdown = true;
			}
		}
	}
}

void CChatServer::PrintState()
{
	wprintf(L"===========================================\n");
	wprintf(L" Chat Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - Update TPS		: %lld \n", _lUpdateTps);		// Update ó�� �ʴ� Ƚ��
	wprintf(L" - MessagePool Use	: %d \n", _MessagePool.GetUseSize());
	wprintf(L" - MessagePool Alloc	: %d \n", _MessagePool.GetAllocSize());	//  UpdateThread �� ����ü �Ҵ緮
	wprintf(L" - Message Queue	: %d \n", _MessageQ.GetUseSize());		// UpdateThread ť ���� ����
	wprintf(L"\n");
	wprintf(L" - PlayerData_Pool	: %d \n", _PlayerPool.GetAllocSize());		// Player ����ü �Ҵ緮
	wprintf(L" - Player Count		: %lld \n", _lPlayerCnt);	// Contents ��Ʈ Player ����
	wprintf(L"\n");
	wprintf(L" - SessionKey Miss	: %lld \n", _lSessionMissCnt);
	_lUpdateTps = 0;

	CNetServer::PrintState();
}

void CChatServer::Heartbeat()
{
	SetEvent(_hUpdateEvent);
}

CChatServer::st_PLAYER * CChatServer::SearchPlayer(UINT64 iSessionID)
{
	map<UINT64, st_PLAYER*>::iterator iter = _PlayerMap.find(iSessionID);
	if (iter == _PlayerMap.end())
		return nullptr;
	return iter->second;
}

bool CChatServer::GetSectorAround(short shSectorX, short shSectorY, st_SECTOR_AROUND * pSectorAround)
{
	if (shSectorX == -1 || shSectorY == -1)
		return false;

	--shSectorX;
	--shSectorY;

	pSectorAround->iCount = 0;
	for (int iY = 0; iY < 3; ++iY)
	{
		if (shSectorY + iY < 0 || shSectorY + iY >= en_MAX_SECTOR_Y)
			continue;
		for (int iX = 0; iX < 3; ++iX)
		{
			if (shSectorX + iX < 0 || shSectorX + iX >= en_MAX_SECTOR_X)
				continue;

			pSectorAround->Around[pSectorAround->iCount].shSectorX = shSectorX + iX;
			pSectorAround->Around[pSectorAround->iCount].shSectorY = shSectorY + iY;
			++pSectorAround->iCount;
		}
	}
	return true;
}

bool CChatServer::SetSector(st_PLAYER * pPlayer, short shSectorX, short shSectorY)
{
	// ���� ���� �ʰ�
	if (shSectorX >= en_MAX_SECTOR_X || shSectorY >= en_MAX_SECTOR_Y || shSectorX < 0 || shSectorY < 0)
		return false;

	// ���� ���Ϳ��� �����
	if (pPlayer->shSectorX != -1 && pPlayer->shSectorY != -1)
		_Sector[pPlayer->shSectorY][pPlayer->shSectorX].remove(pPlayer);

	// �ű� ���Ϳ� ����
	_Sector[shSectorY][shSectorX].push_back(pPlayer);
	pPlayer->shSectorX = shSectorX;
	pPlayer->shSectorY = shSectorY;

	return true;
}



void CChatServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread���� Free
	pMessage->wType = en_MSG_PACKET;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = pPacket;

	pPacket->AddRef(); // Proc_Packet���� Free

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}

void CChatServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

bool CChatServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CChatServer::OnClientJoin(UINT64 SessionID)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread���� Free
	pMessage->wType = en_MSG_JOIN;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = nullptr;

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}

void CChatServer::OnClientLeave(UINT64 SessionID)
{
	st_MESSAGE *pMessage = _MessagePool.Alloc(); // UpdateThread���� Free
	pMessage->wType = en_MSG_LEAVE;
	pMessage->iSessionID = SessionID;
	pMessage->pPacket = nullptr;

	_MessageQ.Enqueue(pMessage);

	SetEvent(_hUpdateEvent);
}


void CChatServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CChatServer::OnHeartBeat()
{
}

unsigned int CChatServer::UpdateThread(LPVOID pCChatServer)
{
	return ((CChatServer*)pCChatServer)->UpdateThread_Process();
}


unsigned int CChatServer::UpdateThread_Process()
{
	ULONGLONG lHeartBeatTick = GetTickCount64();

	while (!_bShutdown)
	{
		WaitForSingleObject(_hUpdateEvent, INFINITE);
		while (1)
		{
			st_MESSAGE *pMessage = nullptr;
			_MessageQ.Dequeue(pMessage);
			if (pMessage == nullptr)
				break;

			switch (pMessage->wType)
			{
			case en_MSG_JOIN:
				Proc_ClientJoin(pMessage);
				break;
			case en_MSG_LEAVE:
				Proc_ClientLeave(pMessage);
				break;
			case en_MSG_PACKET:
				Proc_Packet(pMessage);
				break;
			}
			if(pMessage->pPacket != nullptr)
				pMessage->pPacket->Free();
			_MessagePool.Free(pMessage);		
		}

		// Heartbeat
		//ULONGLONG lCurTick = GetTickCount64();
		//if (lCurTick > lHeartBeatTick + en_HEART_BEAT_INTERVAL)
		//{
		//	lHeartBeatTick = lCurTick;

		//	// Player Heartbeat
		//	//for (auto Iter = _PlayerMap.begin(); Iter != _PlayerMap.end(); ++Iter)
		//	//{
		//	//	st_PLAYER* pPlayer = Iter->second;
		//	//	if (lCurTick > pPlayer->LastRecvTick + en_HEART_BEAT_INTERVAL)
		//	//	{
		//	//		LOG(L"CHAT_SERVER_LOG", LOG_DEBUG, L"Session %d Disconnect by Heartbeat", pPlayer->iSessionID);
		//	//		DisconnectSession(pPlayer->iSessionID);
		//	//	}
		//	//}

		//	// LoginServer Playerlist Clear
		//	_pLoginClient->ClearOldLoginPlayer();
		//}

		++_lUpdateTps;
	}
	return 0;
}

void CChatServer::Proc_ClientJoin(st_MESSAGE * pMessage)
{
	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	pPlayer->iSessionID = pMessage->iSessionID;
	pPlayer->iAccountNo = 0;
	pPlayer->shSectorX = -1;
	pPlayer->shSectorY = -1;

	pPlayer->LastRecvTick = GetTickCount64();

	_PlayerMap.insert(pair<UINT64, st_PLAYER*>(pPlayer->iSessionID, pPlayer));
}

void CChatServer::Proc_ClientLeave(st_MESSAGE * pMessage)
{
	st_PLAYER* pPlayer = SearchPlayer(pMessage->iSessionID);
	if (pPlayer == nullptr)
		return;

	if (pPlayer->iAccountNo != 0)
		--_lPlayerCnt;

	// ���� ���Ϳ��� �����
	if (pPlayer->shSectorX != -1 && pPlayer->shSectorY != -1)
		_Sector[pPlayer->shSectorY][pPlayer->shSectorX].remove(pPlayer);

	pPlayer->shSectorX = -1;
	pPlayer->shSectorY = -1;

	// Map���� �����
	pPlayer->iAccountNo = 0;
	_PlayerMap.erase(pMessage->iSessionID);
	_PlayerPool.Free(pPlayer);
}

void CChatServer::Proc_Packet(st_MESSAGE * pMessage)
{
	CNPacket *pPacket = pMessage->pPacket;
	if (pPacket == NULL)
		CRASH();
	WORD	wPacketType = -1;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		ReqLogin(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		ReqSectorMove(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		ReqChatMessage(pMessage->iSessionID, pPacket);
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		ReqHeartBeat(pMessage->iSessionID, pPacket);
		break;
	default:
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"PacketType Error : %d", wPacketType);
		DisconnectSession(pMessage->iSessionID);
		break;
	}

}

void CChatServer::Proc_Heartbeat()
{
	for (auto iter = _PlayerMap.begin(); iter != _PlayerMap.end(); ++iter)
	{
		st_PLAYER *pPlayer = iter->second;
		if (GetTickCount64() - pPlayer->LastRecvTick > CConfig::_iIntervalHeatbeat)
			DisconnectSession(pPlayer->iSessionID);
	}
}

void CChatServer::SendPacket_Around(st_PLAYER * pPlayer, CNPacket * pPacket, bool bSendMe)
{
	st_SECTOR_AROUND	stSectorAround;
	if (!GetSectorAround(pPlayer->shSectorX, pPlayer->shSectorY, &stSectorAround))
	{
		LOG(L"SECTOR_LOG", LOG_ERROR, L"Sector Around Find Error [X:%d / Y:%d]", pPlayer->shSectorX, pPlayer->shSectorY);
		return;
	}

	list<st_PLAYER*> *pSectorList;
	list<st_PLAYER*>::iterator Iter_List;
	for (int iCnt = 0; iCnt < stSectorAround.iCount; iCnt++)
	{
		pSectorList = &_Sector[stSectorAround.Around[iCnt].shSectorY][stSectorAround.Around[iCnt].shSectorX];
		for (Iter_List = pSectorList->begin(); Iter_List != pSectorList->end(); ++Iter_List)
		{
			if (bSendMe == FALSE)
			{
				if ((*Iter_List)->iSessionID == pPlayer->iSessionID)
					continue;
			}
			SendPacket((*Iter_List)->iSessionID, pPacket);
		}
	}
}

void CChatServer::ReqLogin(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];
	//	}
	//
	//------------------------------------------------------------
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}
	
	INT64 iAccountNo;
	*pPacket >> iAccountNo;
	pPlayer->iAccountNo = iAccountNo;

	pPacket->GetData(reinterpret_cast<char*>(pPlayer->szID), sizeof(pPlayer->szID));
	pPacket->GetData(reinterpret_cast<char*>(pPlayer->szNickname), sizeof(pPlayer->szNickname));
	pPacket->GetData(reinterpret_cast<char*>(pPlayer->szSessionKey), sizeof(pPlayer->szSessionKey));

	// ���������� ä�� ������ ���������Ƿ� LanClient�� PlayerList���� ����
	if (!_pLoginClient->RemoveLoginPlayer(iAccountNo, pPlayer->szSessionKey))
	{
		++_lSessionMissCnt;
		LOG(L"CHAT_SERVER_LOG", LOG_DEBUG, L"Session %d SessionKey Error", iSessionID);
		DisconnectSession(iSessionID);
		return;
	}
	//memcpy_s(&_szSessionKey, sizeof(_szSessionKey), pPlayer->szSessionKey, sizeof(_szSessionKey));

	++_lPlayerCnt;

	// ���� �޽��� �ð� ����
	pPlayer->LastRecvTick = GetTickCount64();

	CNPacket* Packet = CNPacket::Alloc();
	mpResLogin(Packet, 1, pPlayer->iAccountNo);
	SendPacket(iSessionID, Packet);
	Packet->Free();
}

void CChatServer::ReqSectorMove(UINT64 iSessionID, CNPacket *pPacket)
{
	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}
	INT64	iAccountNo;
	short	shSectorX;
	short	shSectorY;
	*pPacket >> iAccountNo;
	*pPacket >> shSectorX;
	*pPacket >> shSectorY;

	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Packet AccountNo Error Player : %lld != %lld", pPlayer->iAccountNo, iAccountNo);
		DisconnectSession(iSessionID);
		return;
	}
	if (!SetSector(pPlayer, shSectorX, shSectorY))
	{
		LOG(L"CHAT_SERVER_LOG", LOG_ERROR, L"SetSector() Error");
		DisconnectSession(iSessionID);
		return;
	}

	// ���� �޽��� �ð� ����
	pPlayer->LastRecvTick = GetTickCount64();

	CNPacket * Packet = CNPacket::Alloc();
	mpResSectorMove(Packet, iAccountNo, pPlayer->shSectorX, pPlayer->shSectorY);
	SendPacket(iSessionID, Packet);
	Packet->Free();
}

void CChatServer::ReqChatMessage(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}

	INT64	iAccountNo;
	WORD	wMessageLen;
	*pPacket >> iAccountNo;
	*pPacket >> wMessageLen;

	// ���ݿ� ���� ó��
	WCHAR	szMessage[256];
	if (pPacket->GetData((char*)szMessage, wMessageLen) == 0)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Packet Get Message Error");
		DisconnectSession(iSessionID);
		return;
	}
	if (pPacket->GetDataSize() > 0)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Packet Size Error");
		DisconnectSession(iSessionID);
		return;
	}
	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_ERROR, L"Packet AccountNo Error Player : %lld != %lld", pPlayer->iAccountNo, iAccountNo);
		DisconnectSession(iSessionID);
		return;
	}
	szMessage[wMessageLen / 2] = '\0';


	//wprintf(L"%s\n", szMessage);

	pPlayer->LastRecvTick = GetTickCount64();

	CNPacket *Packet = CNPacket::Alloc();
	mpResChatMessage(Packet, pPlayer->iAccountNo, pPlayer->szID, pPlayer->szNickname, wMessageLen, szMessage);

	SendPacket_Around(pPlayer, Packet, true);

	Packet->Free();
}

void CChatServer::ReqHeartBeat(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//
	// TODO : OnRecv�� ��Ŷ�� ���� ������ ȣ����� �����Ƿ� 
	//       UpdateThread�� �ֱ������� ����� ��ȸ�ؾ���.
	//------------------------------------------------------------	
	st_PLAYER * pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"CHAT_SERVER_LOG", LOG_WARNG, L"Player Not Find");
		return;
	}
	pPlayer->LastRecvTick = GetTickCount64();
}

void CChatServer::mpResLogin(CNPacket * pBuffer, BYTE byStatus, INT64 iAccountNo)
{
	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	*pBuffer << static_cast<WORD>(en_PACKET_CS_CHAT_RES_LOGIN);

	*pBuffer << byStatus;
	*pBuffer << iAccountNo;
}

void CChatServer::mpResSectorMove(CNPacket * pBuffer, INT64 iAccountNo, WORD wSectorX, WORD wSectorY)
{
	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	*pBuffer << static_cast<WORD>(en_PACKET_CS_CHAT_RES_SECTOR_MOVE);

	*pBuffer << iAccountNo;
	*pBuffer << wSectorX;
	*pBuffer << wSectorY;
}

void CChatServer::mpResChatMessage(CNPacket * pBuffer, INT64 iAccountNo, WCHAR * szID, WCHAR * szNickname, WORD wMessageLen, WCHAR * pMessage)
{
	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null ����
	//		WCHAR	Nickname[20]				// null ����
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	*pBuffer << static_cast<WORD>(en_PACKET_CS_CHAT_RES_MESSAGE);

	*pBuffer << iAccountNo;
	pBuffer->PutData(reinterpret_cast<char*>(szID), sizeof(WCHAR) * 20);
	pBuffer->PutData(reinterpret_cast<char*>(szNickname), sizeof(WCHAR) * 20);

	*pBuffer << wMessageLen;
	pBuffer->PutData(reinterpret_cast<char*>(pMessage), wMessageLen);
}

CLoginClient::CLoginClient() :_bConnect(false), _lUpdateTps(0), _lPlayerCnt(0)
{
	InitializeSRWLock(&_srwMapLock);
}

CLoginClient::~CLoginClient()
{
}

void CLoginClient::ClearOldLoginPlayer()
{
	AcquireSRWLockExclusive(&_srwMapLock);
	for (auto Iter = _LoginMap.begin(); Iter != _LoginMap.end();)
	{
		st_PLAYER* pPlayer = (*Iter).second;
		if (GetTickCount64() - pPlayer->lLastLoginTick > CConfig::_iIntervalHeatbeat)
		{
			Iter = _LoginMap.erase(Iter);
			_PlayerPool.Free(pPlayer);
			continue;
		}
		++Iter;
	}
	ReleaseSRWLockExclusive(&_srwMapLock);
}


bool CLoginClient::RemoveLoginPlayer(INT64 iAccountNo, char * szSessionKey)
{
	bool bRetval = true;

	AcquireSRWLockExclusive(&_srwMapLock);
	st_PLAYER * pPlayer = SearchPlayer(iAccountNo);
	if (pPlayer == nullptr)
	{
		bRetval = false;
	}
	else
	{
		if (memcmp(pPlayer->szSessionKey, szSessionKey, sizeof(pPlayer->szSessionKey)) != 0)
			bRetval = false;
	}
	ReleaseSRWLockExclusive(&_srwMapLock);

	if (pPlayer != nullptr)
	{
		_LoginMap.erase(pPlayer->iAccountNo);
		_PlayerPool.Free(pPlayer);
	}
	return bRetval;
}

CLoginClient::st_PLAYER * CLoginClient::SearchPlayer(INT64 iAccountNo)
{
	auto Iter = _LoginMap.find(iAccountNo);
	if (Iter == _LoginMap.end())
		return  nullptr;
	return Iter->second;
}

void CLoginClient::ReqNewClientLogin(CNPacket * pPacket)
{
	//------------------------------------------------------------
	// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
	//
	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		CHAR	SessionKey[64]
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	// LoginClient �� PlayerList ���� ����
	// 1. ä�� ���� ���� �Ϸ��
	// 2. �ֱ������� ����(�뷫 30��. ä�� ������ ���� ���ϰ� �α��� ���������� �����ϴ� ����)
	// 3. AccountNo�� ������ SessionKEy�� �ٸ� ���? ��å�� ���� �˾Ƽ� ó��

	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	*pPacket >> pPlayer->iAccountNo;
	pPacket->GetData(pPlayer->szSessionKey, sizeof(pPlayer->szSessionKey));
	*pPacket >> pPlayer->iParameter;
	pPlayer->lLastLoginTick = GetTickCount64();

	AcquireSRWLockExclusive(&_srwMapLock);
	st_PLAYER* pCheckPlayer = SearchPlayer(pPlayer->iAccountNo);
	if (pCheckPlayer != nullptr)
	{
		_LoginMap.erase(pPlayer->iAccountNo);
		_PlayerPool.Free(pCheckPlayer);
		LOG(L"LOGIN_CLIENT_LOG", LOG_WARNG, L"Overlapped User Login : %d", pPlayer->iAccountNo);
	}
	_LoginMap.insert(pair<UINT64, st_PLAYER*>(pPlayer->iAccountNo, pPlayer));
	ReleaseSRWLockExclusive(&_srwMapLock);


	//------------------------------------------------------------
	// ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������Ŷ ���Ű���� ������.
	// ���Ӽ�����, ä�ü����� ��Ŷ�� ������ ������, �α��μ����� Ÿ ������ ���� �� CHAT,GAME ������ �����ϹǷ� 
	// �̸� ����ؼ� �˾Ƽ� ���� �ϵ��� ��.
	//
	// �÷��̾��� ���� �α��� �Ϸ�� �� ��Ŷ�� Chat,Game ���ʿ��� �� �޾��� ������.
	//
	// ������ �� Parameter �� �̹� ����Ű ������ ���� ������ �� �ִ� Ư�� ��
	// ClientID �� ����, ���� ī������ ���� ��� ����.
	//
	// �α��μ����� ���Ӱ� �������� �ݺ��ϴ� ��� ������ ���������� ���� ������ ���� ��������
	// �����Ͽ� �ٸ� ����Ű�� ��� ���� ������ ����.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	CNPacket *pSendPacket = CNPacket::Alloc();
	*pSendPacket << static_cast<WORD>(en_PACKET_SS_RES_NEW_CLIENT_LOGIN);
	*pSendPacket << pPlayer->iAccountNo;
	*pSendPacket << pPlayer->iParameter;

	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CLoginClient::OnClientJoin()
{
	//------------------------------------------------------------
	// �ٸ� ������ �α��� ������ �α���.
	// �̴� ������ ������, �׳� �α��� ��.  
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
	//
	//		WCHAR	ServerName[32]		// �ش� ������ �̸�.  
	//	}
	//
	//------------------------------------------------------------
	CNPacket *pPacket = CNPacket::Alloc();
	*pPacket << static_cast<WORD>(en_PACKET_SS_LOGINSERVER_LOGIN);
	*pPacket << static_cast<BYTE>(dfSERVER_TYPE_CHAT);
	pPacket->PutData(reinterpret_cast<char*>(pConfig->_szServerName), sizeof(pConfig->_szServerName));

	SendPacket(pPacket);
	pPacket->Free();
}

void CLoginClient::OnClientLeave()
{
	LOG(L"LOGIN_CLIENT_LOG", LOG_SYSTM, L"Disconnect from login Server");
	_bConnect = false;
}

void CLoginClient::OnRecv(CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_SS_REQ_NEW_CLIENT_LOGIN:
		ReqNewClientLogin(pPacket);
		break;
	default:
		LOG(L"LOGIN_CLIENT_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		break;
	}
}

void CLoginClient::OnSend(int iSendSize)
{
}

void CLoginClient::OnError(int iErrCode, WCHAR * wszErr)
{
}