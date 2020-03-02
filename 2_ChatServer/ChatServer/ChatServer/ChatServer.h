#ifndef __CHAT_SERVER_H__
#define __CHAT_SERVER_H__

#include "NetworkLib.h"
#include "CConfig.h"

using namespace std;
using namespace mylib;

//////////////////////////////////////////////////////////////////////////
// TODO: ChatNetServer Class
//
//////////////////////////////////////////////////////////////////////////
class CChatServer : public CNetServer
{
public:
	friend class CLoginClient;

	CChatServer();
	virtual ~CChatServer();

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

		// Sector Cnt
		en_MAX_SECTOR_Y = 100,
		en_MAX_SECTOR_X = 100,

		// Heartbeat timeout
		en_HEART_BEAT_INTERVAL = 300000
	};

	struct st_MESSAGE
	{
		WORD		wType;
		UINT64		iSessionID;
		CNPacket*	pPacket;
	};

	struct st_PLAYER
	{
		UINT64	iSessionID;
		INT64	iAccountNo;
		WCHAR	szID[en_LEN_ID];
		WCHAR	szNickname[en_LEN_NICK];
		char	szSessionKey[en_LEN_SESSIONKEY];

		short	shSectorX, shSectorY;
		ULONGLONG LastRecvTick;
	};

	struct st_SECTOR
	{
		short shSectorX, shSectorY;
	};

	struct st_SECTOR_AROUND
	{
		int iCount;
		st_SECTOR Around[9];
	};

	bool Start();
	void Stop();
	void ServerControl();

	// 15000명 수용 목표
	void PrintState();

	// 외부에서 주기적으로 호출
	void Heartbeat();

protected:
	//////////////////////////////////////////////////////////////////////////
	// Thread
	//
	//////////////////////////////////////////////////////////////////////////
	static unsigned int CALLBACK UpdateThread(LPVOID pCChatServer);
	unsigned int CALLBACK UpdateThread_Process();

	st_PLAYER* SearchPlayer(UINT64 iSessionID);
	bool GetSectorAround(short shSectorX, short shSectorY, st_SECTOR_AROUND *pSectorAround);
	bool SetSector(st_PLAYER* pPlayer, short shSectorX, short shSectorY);

	void Proc_ClientJoin(st_MESSAGE * pMessage);	// stPlayer 생성
	void Proc_ClientLeave(st_MESSAGE * pMessage);	// stPlayer 해제
	void Proc_Packet(st_MESSAGE * pMessage);
	void Proc_Heartbeat();

	// Send
	void SendPacket_Around(st_PLAYER* pPlayer, CNPacket *pPacket, bool bSendMe = true);

	// Request, Response
	void ReqLogin(UINT64 iSessionID, CNPacket *pPacket);
	void ReqSectorMove(UINT64 iSessionID, CNPacket *pPacket);
	void ReqChatMessage(UINT64 iSessionID, CNPacket *pPacket);
	void ReqHeartBeat(UINT64 iSessionID, CNPacket *pPacket);

	// Making Packet
	void mpResLogin(CNPacket *pBuffer, BYTE byStatus, INT64 iAccountNo);
	void mpResSectorMove(CNPacket *pBuffer, INT64 iAccountNo, WORD wSectorX, WORD wSectorY);
	void mpResChatMessage(CNPacket *pBuffer, INT64 iAccountNo, WCHAR *szID, WCHAR *szNickname, WORD wMessageLen, WCHAR *szMessage);


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
	bool OnConnectRequest(WCHAR* wszIP, int iPort);
	void OnClientJoin(UINT64 SessionID);
	void OnClientLeave(UINT64 SessionID);
	void OnRecv(UINT64 SessionID, CNPacket * pPacket);	// 패킷 수신 완료 후
	void OnSend(UINT64 SessionID, int iSendSize);	// 패킷 송신 완료 후
	void OnError(int iErrCode, WCHAR * wszErr);
	void OnHeartBeat();

public:
	// Client
	CLoginClient* _pLoginClient;

	bool	_bShutdown;
	bool	_bControlMode;

private:
	// Thread
	HANDLE	_hMonitorThread;
	HANDLE	_hUpdateThread;
	HANDLE	_hUpdateEvent;

	// Player
	CLFMemoryPool<st_PLAYER>	_PlayerPool;
	map<UINT64, st_PLAYER*>		_PlayerMap;	// SessionID, st_PLAYER*

	// Sector
	list<st_PLAYER*>			_Sector[en_MAX_SECTOR_Y][en_MAX_SECTOR_X];

	// Message
	CLFMemoryPool<st_MESSAGE>	_MessagePool;
	CLFQueue<st_MESSAGE*>		_MessageQ;
	
	// Monitoring
	LONG64	_lUpdateTps;
	LONG64	_lPlayerCnt;
	LONG64	_lSessionMissCnt;
	char	_szSessionKey[en_LEN_SESSIONKEY];
};


//////////////////////////////////////////////////////////////////////////
// TODO: LoginLanClient Class
//
//////////////////////////////////////////////////////////////////////////
class CLoginClient : public CLanClient
{
public:
	friend class CChatServer;

	CLoginClient();
	virtual ~CLoginClient();

	struct st_PLAYER
	{
		INT64	iAccountNo;
		char	szSessionKey[CChatServer::en_LEN_SESSIONKEY];
		INT64	iParameter;
		ULONGLONG lLastLoginTick;
	};
	//////////////////////////////////////////////////////////////////////////
	// 외부 호출
	//
	//////////////////////////////////////////////////////////////////////////
	// 로그인 요청된 플레이어 주기적으로 삭제
	void ClearOldLoginPlayer();
	// 채팅 서버 접속한 플레이어 최종 삭제
	bool RemoveLoginPlayer(INT64 iAccountNo, char *szSessionKey);

private:
	st_PLAYER* SearchPlayer(INT64 iAccountNo);

	void ReqNewClientLogin(CNPacket *pPacket);

	//////////////////////////////////////////////////////////////////////////
	// Notice
	//
	// OnClientJoin			: 서버 연결 직후
	// OnClientLeave		: 서버 연결 끊긴 이후
	// OnRecv				: 패킷 수신 후, 패킷 처리
	// OnSend				: 패킷 송신 후
	// OnError				: 에러 발생 후
	//////////////////////////////////////////////////////////////////////////
	void OnClientJoin();
	void OnClientLeave();
	void OnRecv(CNPacket * pPacket);
	void OnSend(int iSendSize);
	void OnError(int iErrCode, WCHAR * wszErr);


public:
	bool _bConnect;
private:
	LONG64 _lUpdateTps;
	LONG64 _lPlayerCnt;

	SRWLOCK _srwMapLock;
	map<INT64, st_PLAYER*>	_LoginMap;
	CLFMemoryPool<st_PLAYER> _PlayerPool;
};

#define MON_CONNECT_CNT		mylib::CNetServer::en_CONNECT_CNT
#define MON_ACCEPT_CNT		mylib::CNetServer::en_ACCEPT_CNT
#define MON_PACKET_TPS		mylib::CNetServer::en_PACKET_TPS
#define MON_PACKETPOOL_SIZE	mylib::CNetServer::en_PACKETPOOL_SIZE
#define MON_ALL				mylib::CNetServer::en_ALL
#endif