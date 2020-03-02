#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "CNetServer.h"
#include "CommonProtocol.h"
#include "CConfig.h"
#include <list>
#include <map>

using namespace std;
using namespace mylib;

class CChatServer : public CNetServer
{
public:

	CChatServer();
	virtual ~CChatServer();

	enum en_SERVER_CONFIG
	{
		// String Length
		en_LEN_ID = 20,
		en_LEN_NICK = 20,
		en_SESSIONKEY_LEN = 64,

		// Message Type
		en_MSG_JOIN = 0,
		en_MSG_LEAVE,
		en_MSG_PACKET,
		en_MSG_HEARTBEAT,

		// Sector Cnt
		en_SECTOR_MAX_Y = 100,
		en_SECTOR_MAX_X = 100,

		// Heartbeat timeout
		en_HEART_BEAT_INTERVAL = 30000
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
		char	szSessionKey[en_SESSIONKEY_LEN];

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
	// Update Thread
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

	void mpResLogin(CNPacket *pBuffer, BYTE byStatus, INT64 iAccountNo);
	void mpResSectorMove(CNPacket *pBuffer, INT64 iAccountNo, WORD wSectorX, WORD wSectorY);
	void mpResChatMessage(CNPacket *pBuffer, INT64 iAccountNo, WCHAR *szID, WCHAR *szNickname, WORD wMessageLen, WCHAR *szMessage);


	//////////////////////////////////////////////////////////////////////////
	// Handler
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
	void OnRecv(UINT64 SessionID, CNPacket * pPacket);
	void OnSend(UINT64 SessionID, int iSendSize);
	void OnError(int iErrCode, WCHAR * wszErr);
	void OnHeartBeat();

private:
	// Thread
	HANDLE	_hUpdateThread;
	HANDLE	_hUpdateEvent;

public:
	bool	_bShutdown;
	bool	_bControlMode;

private:
	// Player
	CLFMemoryPool<st_PLAYER>	_PlayerPool;
	map<UINT64, st_PLAYER*>		_PlayerMap;	// SessionID, st_PLAYER*

	// Sector
	list<st_PLAYER*>			_Sector[en_SECTOR_MAX_Y][en_SECTOR_MAX_X];

	// Message
	CLFMemoryPool<st_MESSAGE>	_MessagePool;
	CLFQueue<st_MESSAGE*>		_MessageQ;
	
	// Monitoring
	LONG64	_lUpdateTps;
	LONG64	_lPlayerCnt;
	LONG64	_lSessionMissCnt;
	char	_szSessionKey[64];
};
#endif