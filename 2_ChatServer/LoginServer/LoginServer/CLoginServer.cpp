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
	// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
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

	// ä�� ������ ���� ���� Ȯ��
	if (!_bConnectChatServer)
	{
		pPlayer->byStatus = dfLOGIN_STATUS_NOSERVER;

		CNPacket *Packet = CNPacket::Alloc();
		mpResLogin(Packet, pPlayer->iAccountNo, pPlayer->byStatus, pPlayer->szID, pPlayer->szNickname);
		SendPacket(pPlayer->iSessionID, Packet);
		Packet->Free();

		return;
	}


	// �α��� ��û �ð�
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
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,
	_pLoginLanServer->ResNewClientLogin(pPlayer->iAccountNo, pPlayer->szSessionKey, pPlayer->iSessionID);
}

void CLoginServer::ResCompleteNewClientLogin(BYTE byServerType, INT64 iAccountNo, INT64 iParameter)
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
	//en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
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

		// Ŭ���̾�Ʈ�� �α��� ��� ����
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
	// �α��� �������� Ŭ���̾�Ʈ�� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (���ǿ���) / 1 (����) ...  �ϴ� defines ���
	//
	//		WCHAR	ID[20]				// ����� ID		. null ����
	//		WCHAR	Nickname[20]		// ����� �г���	. null ����
	//
	//		WCHAR	GameServerIP[16]	// ���Ӵ�� ����,ä�� ���� ����
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
	// GameServer. �ӽ÷� ChatServer
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

	// �ش� ServerType���κ��� �� �Ϸ� ������ NetServer�� ����
	_pLoginNetServer->ResCompleteNewClientLogin(dfSERVER_TYPE_CHAT, iAccountNo, iParameter);
}

void CLoginLanServer::ResNewClientLogin(INT64 iAccountNo, CHAR * szSessionKey, INT64 iParameter)
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
