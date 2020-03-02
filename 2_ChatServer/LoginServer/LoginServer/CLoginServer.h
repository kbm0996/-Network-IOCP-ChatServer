#ifndef __LOGIN_SERVER__
#define __LOGIN_SERVER__

#include "NetworkLib.h"
#include "CommonProtocol.h"
#include "CDBAccount.h"
#include "CConfig.h"
#include <list>

using namespace std;
using namespace mylib;

//////////////////////////////////////////////////////////////////////////
// TODO: LoginNetServer Class
//
//////////////////////////////////////////////////////////////////////////
class CLoginServer : public CNetServer
{
public:
	CLoginServer();
	virtual ~CLoginServer();

	enum en_SERVER_CONFIG
	{
		// Message Type
		en_MSG_JOIN = 0,
		en_MSG_LEAVE,
		en_MSG_PACKET,
		en_MSG_HEARTBEAT,

		// String Length
		en_LEN_ID = 20,
		en_LEN_NICK = 20,
		en_LEN_SESSIONKEY = 64,

		// Heartbeat timeout
		en_TIME_OUT = 60000
	};
	struct st_PLAYER
	{
		UINT64	iSessionID;
		INT64	iAccountNo;
		WCHAR	szID[en_LEN_ID];
		WCHAR	szNickname[en_LEN_NICK];
		char	szSessionKey[en_LEN_SESSIONKEY];

		BYTE	byStatus;

		ULONGLONG lLoginReqTick;
	};

	bool Start();
	void Stop();
	void ServerControl();

	void PrintState();

private:
	st_PLAYER * SearchPlayer(UINT64 iSessionID);

	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	// Client to LoginServer
	/* NetServer ::*/void ReqNewClientLogin(UINT64 iSessionID, CNPacket *pPacket);
	// LanServer ::  void ResNewClientLogin(INT64 iAccountNo, CHAR* szSessionKey, INT64 iParameter);

	// Server to Server
	// LanServer ::  void ReqCompleteNewClientLogin(UINT64 iSessionID, CNPacket *pPacket);
	/* NetServer ::*/void ResCompleteNewClientLogin(BYTE byServerType, INT64 iAccountNo, INT64 iParameter);
	void mpResLogin(CNPacket * pBuffer, INT64 iAccountNo, BYTE byStatus, WCHAR* szID, WCHAR* szNick);


protected:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnConnectRequest		: Accept 직후, true/false 접속 허용/거부
	// OnClientJoin			: Accept 접속처리 완료 후, 유저 접속 관련
	// OnClientLeave		: Disconnect 후, 유저 정리
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnError				: 에러 발생 후
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnConnectRequest(WCHAR* wszIP, int iPort);
	virtual void OnClientJoin(UINT64 SessionID);
	virtual void OnClientLeave(UINT64 SessionID);
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);
	virtual void OnHeartBeat();


private:
	// Thread
	HANDLE	_hUpdateThread;
	HANDLE	_hUpdateEvent;
public:
	bool	_bShutdown;
private:
	bool	_bControlMode;

	// Player
	CLFMemoryPool<st_PLAYER>	_PlayerPool;
	list<st_PLAYER*>		_PlayerList;
	SRWLOCK _srwPlayerLock;

	// Monitoring
	LONG64		_lLoginSuccessTps;
	ULONGLONG	_lLoginSuccessTime_Min;
	ULONGLONG	_lLoginSuccessTime_Max;
	ULONGLONG	_lLoginSuccessTime_Cnt;

	// Server to Server
	friend class CLoginLanServer;
	CLoginLanServer* _pLoginLanServer;
	bool		_bConnectChatServer;

	// DB
	CDBAccount* _pDBAccount;
};


//////////////////////////////////////////////////////////////////////////
// TODO: LoginLanServer Class
//
//////////////////////////////////////////////////////////////////////////
class CLoginLanServer : public CLanServer
{
public:
	friend class CLoginServer;

	CLoginLanServer(CLoginServer* pLoginServer);
	virtual ~CLoginLanServer();

private:
	struct st_SERVER
	{
		UINT64	iSessionID;
		BYTE	byServerType;
		WCHAR	szServerName[32];
	};

	st_SERVER* SearchServer(UINT64 iSessionID);

	//////////////////////////////////////////////////////////////////////////
	// Request, Response
	//
	//////////////////////////////////////////////////////////////////////////
	// Server to Server
	void ReqLoginServerLogin(UINT64 iSessionID, CNPacket *pPacket);

	// 다른 서버로부터 Client 최종 접속 통지
	/* LanServer ::*/void ReqCompleteNewClientLogin(UINT64 iSessionID, CNPacket *pPacket);
	// NetServer ::  void ResCompleteNewClientLogin(BYTE byServerType, INT64 iAccountNo, INT64 iParameter);

	// NetServer to LanServer
	// NetServer ::  void ReqNewClientLogin(UINT64 iSessionID, CNPacket *pPacket);
	/* LanServer ::*/void ResNewClientLogin(INT64 iAccountNo, CHAR* szSessionKey, INT64 iParameter);

protected:
	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnConnectRequest		: Accept 직후, true/false 접속 허용/거부
	// OnClientJoin			: Accept 접속처리 완료 후, 유저 접속 관련
	// OnClientLeave		: Disconnect 후, 유저 정리
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnError				: 에러 발생 후
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnConnectRequest(WCHAR* wszIP, int iPort);
	virtual void OnClientJoin(UINT64 SessionID);
	virtual void OnClientLeave(UINT64 SessionID);;
	virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	virtual void OnSend(UINT64 SessionID, int iSendSize);
	virtual void OnError(int iErrCode, WCHAR * wszErr);

private:
	// Player
	list<st_SERVER*>	_ServerList;
	SRWLOCK				_srwServerLock;

	CLoginServer* _pLoginNetServer;
};

#endif