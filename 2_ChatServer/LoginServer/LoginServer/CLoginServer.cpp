#include "CLoginServer.h"

CLoginServer::CLoginServer() : _bShutdown(false), _bControlMode(false), 
_lLoginSuccessTps(0), _lLoginSuccessTime_Min(0xffffffff), 
_lLoginSuccessTime_Max(0), _lLoginSuccessTime_Cnt(0), _bConnectChatServer(false)
{
	_pLoginLanServer = new CLoginLanServer(this);
	InitializeSRWLock(&_srwPlayerLock);
	CNPacket::CNPacket(200);

	if (!pConfig->LoadConfigFile("_LoginServer.cnf"))
		CRASH();

	_pDBAccount = new CDBAccount(CConfig::_szDBIP, CConfig::_szDBAccount, CConfig::_szDBPassword, CConfig::_szDBName, CConfig::_iDBPort);
}

CLoginServer::~CLoginServer()
{
	delete _pLoginLanServer;
	delete _pDBAccount;
}

bool CLoginServer::Start()
{
	if (!_pDBAccount->ReadDB(CDBAccount::dfACCOUNT_STATUS_INIT))
		return false;

	if (!_pLoginLanServer->Start(CConfig::_szLanBindIP, CConfig::_iLanBindPort, 4, false, 3))
		return false;

	if (!CNetServer::Start(CConfig::_szBindIP, CConfig::_iBindPort, CConfig::_iWorkerThreadNo, false, CConfig::_iClientMax, CConfig::_byPacketCode, CConfig::_byPacketKey1, CConfig::_byPacketKey2))
		return false;

	return true;
}

void CLoginServer::Stop()
{
	CNetServer::Stop();
	_pLoginLanServer->Stop();
}

void CLoginServer::ServerControl()
{
	//------------------------------------------
	// L : Control Lock / U : Unlock / Q : Quit
	//------------------------------------------
	//  _kbhit() 함수 자체가 느리기 때문에 사용자 혹은 더미가 많아지면 느려져서 실제 테스트시 주석처리 권장
	// 그런데도 GetAsyncKeyState를 안 쓴 이유는 창이 활성화되지 않아도 키를 인식함 Windowapi의 경우 
	// 제어가 가능하나 콘솔에선 어려움

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

void CLoginServer::PrintState()
{
	ULONGLONG lLoginSuccessTime_Avr = (_lLoginSuccessTime_Min + _lLoginSuccessTime_Max) / 2;
	ULONGLONG lLoginSuccessTime_Min = _lLoginSuccessTime_Min;
	if (_lLoginSuccessTime_Min == 0xffffffff)
	{
		lLoginSuccessTime_Avr = _lLoginSuccessTime_Max;
		lLoginSuccessTime_Min = 0;
	}

	wprintf(L"===========================================\n");
	wprintf(L" Login Server\n");
	wprintf(L"===========================================\n");
	wprintf(L" - LoginSuccess TPS	: %lld \n", _lLoginSuccessTps);		
	//wprintf(L" - LoginWait		: %d \n");
	wprintf(L"\n");
	wprintf(L" - CompleteTime Avr	: %lld \n", lLoginSuccessTime_Avr);
	wprintf(L"	Min / Max	: %lld / %lld ms\n", lLoginSuccessTime_Min, _lLoginSuccessTime_Max);
	_lLoginSuccessTps = 0;

	CNetServer::PrintState(1||2);
	_pLoginLanServer->PrintState();
}

bool CLoginServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CLoginServer::OnClientJoin(UINT64 SessionID)
{
	st_PLAYER* pPlayer = _PlayerPool.Alloc();
	pPlayer->byStatus = dfLOGIN_STATUS_NONE;
	//pPlayer->iAccountNo = 0;
	pPlayer->iSessionID = SessionID;
	pPlayer->lLoginReqTick = -1;
	ZeroMemory(pPlayer->szID, sizeof(pPlayer->szID));
	ZeroMemory(pPlayer->szNickname, sizeof(pPlayer->szNickname));
	ZeroMemory(pPlayer->szSessionKey, sizeof(pPlayer->szSessionKey));

	AcquireSRWLockExclusive(&_srwPlayerLock);
	_PlayerList.push_back(pPlayer);
	ReleaseSRWLockExclusive(&_srwPlayerLock);
}

void CLoginServer::OnClientLeave(UINT64 SessionID)
{
	AcquireSRWLockExclusive(&_srwPlayerLock);
	st_PLAYER* pPlayer = nullptr;
	for (auto Iter = _PlayerList.begin(); Iter != _PlayerList.end(); Iter++)
	{
		pPlayer = *Iter;
		if (pPlayer->iSessionID == SessionID)
		{
			_PlayerList.erase(Iter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&_srwPlayerLock);

	pPlayer->lLoginReqTick = -1;
	_PlayerPool.Free(pPlayer);
}

void CLoginServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		ReqNewClientLogin(SessionID, pPacket);
		break;
	default:
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		DisconnectSession(SessionID);
		break;
	}
}

void CLoginServer::OnSend(UINT64 SessionID, int iSendSize)
{
	st_PLAYER* pPlayer = SearchPlayer(SessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID:%d", SessionID);
		return;
	}
	if (pPlayer->iSessionID == SessionID)
		DisconnectSession(SessionID);
}

void CLoginServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

void CLoginServer::OnHeartBeat()
{
}

CLoginServer::st_PLAYER * CLoginServer::SearchPlayer(UINT64 iSessionID)
{
	AcquireSRWLockExclusive(&_srwPlayerLock);
	st_PLAYER* pPlayer = nullptr;
	auto Iter = _PlayerList.begin();
	for (; Iter != _PlayerList.end(); ++Iter)
	{
		pPlayer = (*Iter);
		if (pPlayer->iSessionID == iSessionID)
			break;
	}
	if (Iter == _PlayerList.end())
		pPlayer = nullptr;
	ReleaseSRWLockExclusive(&_srwPlayerLock);
	return  pPlayer;
}

void CLoginServer::ReqNewClientLogin(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 로그인 서버로 클라이언트 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	BYTE byStatus = 1;
	st_PLAYER *pPlayer = SearchPlayer(iSessionID);
	if (pPlayer == nullptr)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID:%d", iSessionID);
		return;
	}

	*pPacket >> pPlayer->iAccountNo;
	pPacket->GetData(reinterpret_cast<char*>(pPlayer->szSessionKey), sizeof(pPlayer->szSessionKey));

	// 채팅 서버와 연결 여부 확인
	if (!_bConnectChatServer)
	{
		pPlayer->byStatus = dfLOGIN_STATUS_NOSERVER;

		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		return;
	}


	// 로그인 요청 시간
	pPlayer->lLoginReqTick = GetTickCount64();

	// TODO : DB
	CDBAccount::st_SESSIONCHACK_IN pIn;
	pIn.AccountNo = pPlayer->iAccountNo;
	pIn.SessionKey = pPlayer->szSessionKey;

	CDBAccount::st_SESSIONCHACK_OUT pOut;
	pOut.ID = pPlayer->szID;
	pOut.Nickname = pPlayer->szNickname;

	_pDBAccount->ReadDB(CDBAccount::dfACCOUNT_SESSION_CHACK, reinterpret_cast<void*>(&pIn), reinterpret_cast<void*>(&pOut));
	
	pPlayer->byStatus = pOut.byStatus;
	if (pPlayer->byStatus != dfLOGIN_STATUS_OK)
	{
		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		return;
	}
	
	//------------------------------------------------------------
	// 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
	//
	// 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
	// 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
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
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,
	_pLoginLanServer->ResNewClientLogin(pPlayer->iAccountNo, pPlayer->szSessionKey, pPlayer->iSessionID);
}

void CLoginServer::ResCompleteNewClientLogin(BYTE byServerType, INT64 iAccountNo, INT64 iParameter)
{
	//------------------------------------------------------------
	// 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
	//
	// 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
	// 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
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
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

	// 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
	// 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
	st_PLAYER *pPlayer = SearchPlayer(iParameter);
	if (pPlayer == nullptr)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player Not Find - SessionID(Parameter):%d", iParameter);
		return;
	}
	if (pPlayer->iAccountNo != iAccountNo)
	{
		LOG(L"LOGIN_NET_SERVER_LOG", LOG_WARNG, L"Player SessionID Error - %d != %d", pPlayer->iAccountNo, iAccountNo);
		return;
	}

	if (byServerType == dfSERVER_TYPE_CHAT)
	{
		InterlockedIncrement64(&_lLoginSuccessTps);
		pPlayer->byStatus = dfLOGIN_STATUS_OK;

		ULONGLONG lLoginLapseTick = GetTickCount64() - pPlayer->lLoginReqTick;
		_lLoginSuccessTime_Cnt += lLoginLapseTick;
		_lLoginSuccessTime_Max = max(lLoginLapseTick, _lLoginSuccessTime_Max);
		_lLoginSuccessTime_Min = min(lLoginLapseTick, _lLoginSuccessTime_Min);

		// 클라이언트에 로그인 결과 전송
		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		pPlayer->lLoginReqTick = -1;
	}
}

void CLoginServer::mpResLogin(CNPacket * pBuffer, INT64 iAccountNo, BYTE byStatus, WCHAR* szID, WCHAR* szNick)
{
	//------------------------------------------------------------
	// 로그인 서버에서 클라이언트로 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (세션오류) / 1 (성공) ...  하단 defines 사용
	//
	//		WCHAR	ID[20]				// 사용자 ID		. null 포함
	//		WCHAR	Nickname[20]		// 사용자 닉네임	. null 포함
	//
	//		WCHAR	GameServerIP[16]	// 접속대상 게임,채팅 서버 정보
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//	}
	//
	//------------------------------------------------------------
	*pBuffer << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
	*pBuffer << iAccountNo;
	*pBuffer << byStatus;
	pBuffer->PutData(reinterpret_cast<char*>(szID), sizeof(WCHAR) * en_LEN_ID);
	pBuffer->PutData(reinterpret_cast<char*>(szNick), sizeof(WCHAR) * en_LEN_NICK);
	// GameServer. 임시로 ChatServer
	pBuffer->PutData(reinterpret_cast<char*>(CConfig::_szChatServerIP), sizeof(WCHAR) * 16);
	*pBuffer << static_cast<USHORT>(CConfig::_iChatServerPort);
	// ChatServer
	pBuffer->PutData(reinterpret_cast<char*>(pConfig->_szChatServerIP), sizeof(WCHAR) * 16);
	*pBuffer << static_cast<USHORT>(CConfig::_iChatServerPort);
}


CLoginLanServer::CLoginLanServer(CLoginServer *pLoginServer)
{
	InitializeSRWLock(&_srwServerLock);

	_pLoginNetServer = pLoginServer;
}

CLoginLanServer::~CLoginLanServer()
{
}

bool CLoginLanServer::OnConnectRequest(WCHAR * wszIP, int iPort)
{
	return true;
}

void CLoginLanServer::OnClientJoin(UINT64 SessionID)
{
	st_SERVER* pServer = new st_SERVER;
	pServer->iSessionID = SessionID;

	AcquireSRWLockExclusive(&_srwServerLock);
	_ServerList.push_back(pServer);
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CLoginLanServer::OnClientLeave(UINT64 SessionID)
{
	AcquireSRWLockExclusive(&_srwServerLock);
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end(); ++Iter)
	{
		st_SERVER * pServer = *Iter;
		if (pServer->iSessionID == SessionID)
		{
			if (pServer->byServerType == dfSERVER_TYPE_CHAT)
				_pLoginNetServer->_bConnectChatServer = false;

			_ServerList.erase(Iter);
			delete pServer;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
}

void CLoginLanServer::OnRecv(UINT64 SessionID, CNPacket * pPacket)
{
	WORD wPacketType;
	*pPacket >> wPacketType;

	switch (wPacketType)
	{
	case en_PACKET_SS_LOGINSERVER_LOGIN:
		ReqLoginServerLogin(SessionID, pPacket);
		break;
	case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:
		ReqCompleteNewClientLogin(SessionID, pPacket);
		break;
	default:
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_ERROR, L"PacketType Error : %d", wPacketType);
		DisconnectSession(SessionID);
		break;
	}
}

void CLoginLanServer::OnSend(UINT64 SessionID, int iSendSize)
{
}

void CLoginLanServer::OnError(int iErrCode, WCHAR * wszErr)
{
}

CLoginLanServer::st_SERVER * CLoginLanServer::SearchServer(UINT64 iSessionID)
{
	AcquireSRWLockExclusive(&_srwServerLock);
	st_SERVER * pServer = nullptr;
	auto Iter = _ServerList.begin();
	for (; Iter != _ServerList.end(); ++Iter)
	{
		pServer = (*Iter);
		if (pServer->iSessionID == iSessionID)
			break;
	}
	if (Iter == _ServerList.end())
		pServer = nullptr;
	ReleaseSRWLockExclusive(&_srwServerLock);
	return pServer;
}

void CLoginLanServer::ReqLoginServerLogin(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 다른 서버가 로그인 서버로 로그인.
	// 이는 응답이 없으며, 그냥 로그인 됨.  
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
	//
	//		WCHAR	ServerName[32]		// 해당 서버의 이름.  
	//	}
	//
	//------------------------------------------------------------
	// en_PACKET_SS_LOGINSERVER_LOGIN,
	st_SERVER * pServer = SearchServer(iSessionID);
	if (pServer == nullptr)
	{
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}
	*pPacket >> pServer->byServerType;
	pPacket->GetData(reinterpret_cast<char*>(pServer->szServerName), sizeof(pServer->szServerName));

	if (pServer->byServerType == dfSERVER_TYPE_CHAT)
		_pLoginNetServer->_bConnectChatServer = true;
}

void CLoginLanServer::ReqCompleteNewClientLogin(UINT64 iSessionID, CNPacket * pPacket)
{
	//------------------------------------------------------------
	// 게임.채팅 서버가 새로운 클라이언트 접속패킷 수신결과를 돌려줌.
	// 게임서버용, 채팅서버용 패킷의 구분은 없으며, 로그인서버에 타 서버가 접속 시 CHAT,GAME 서버를 구분하므로 
	// 이를 사용해서 알아서 구분 하도록 함.
	//
	// 플레이어의 실제 로그인 완료는 이 패킷을 Chat,Game 양쪽에서 다 받았을 시점임.
	//
	// 마지막 값 Parameter 는 이번 세션키 공유에 대해 구분할 수 있는 특정 값
	// ClientID 를 쓰던, 고유 카운팅을 쓰던 상관 없음.
	//
	// 로그인서버에 접속과 재접속을 반복하는 경우 이전에 공유응답이 새로 접속한 뒤의 응답으로
	// 오해하여 다른 세션키를 들고 가는 문제가 생김.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	//en_PACKET_SS_RES_NEW_CLIENT_LOGIN,
	st_SERVER * pServer = SearchServer(iSessionID);
	if (pServer == nullptr)
	{
		LOG(L"LOGIN_LAN_SERVER_LOG", LOG_WARNG, L"Server Not Find");
		return;
	}

	INT64 iAccountNo;
	INT64 iParameter;
	*pPacket >> iAccountNo;
	*pPacket >> iParameter;

	// 해당 ServerType으로부터 온 완료 통지를 NetServer에 전달
	_pLoginNetServer->ResCompleteNewClientLogin(dfSERVER_TYPE_CHAT, iAccountNo, iParameter);
}

void CLoginLanServer::ResNewClientLogin(INT64 iAccountNo, CHAR * szSessionKey, INT64 iParameter)
{
	//------------------------------------------------------------
	// 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
	//
	// 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
	// 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
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
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,
	CNPacket *Packet = CNPacket::Alloc();
	*Packet << static_cast<WORD>(en_PACKET_SS_REQ_NEW_CLIENT_LOGIN);
	*Packet << iAccountNo;
	Packet->PutData(szSessionKey, 64);
	*Packet << iParameter;

	AcquireSRWLockExclusive(&_srwServerLock);
	for (auto Iter = _ServerList.begin(); Iter != _ServerList.end(); ++Iter)
	{
		SendPacket((*Iter)->iSessionID, Packet);
	}
	ReleaseSRWLockExclusive(&_srwServerLock);
	Packet->Free();
}
