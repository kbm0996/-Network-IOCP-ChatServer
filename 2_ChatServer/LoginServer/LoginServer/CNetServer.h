/*----------------------------------------------------------------------------
CLanServer


- bool Start(...) ���� IP / ��Ʈ / ��Ŀ������ �� / ���ۿɼ� / �ִ������� ��
- void Stop(...)
- bool DisconnectClientID)  / UINT64
- int GetClientCount(...)
- SendPacket(ClientID, Packet *)   / UINT64

virtual void OnClientJoin(Client ���� / ClientID / ��Ÿ���) = 0;   < Accept �� ����ó�� �Ϸ� �� ȣ��.
virtual void OnClientLeave(ClientID) = 0;   	            < Disconnect �� ȣ��
virtual bool OnConnectRequest(ClientIP,Port) = 0;        < accept ����
return false; �� Ŭ���̾�Ʈ �ź�.
return true; �� ���� ���

virtual void OnRecv(ClientID, CNPacket *) = 0;              < ��Ŷ ���� �Ϸ� ��
virtual void OnSend(ClientID, int sendsize) = 0;           < ��Ŷ �۽� �Ϸ� ��

virtual void OnWorkerThreadBegin() = 0;                    < ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
virtual void OnWorkerThreadEnd() = 0;                      < ��Ŀ������ 1���� ���� ��

virtual void OnError(int errorcode, wchar *) = 0;


- ����

CServer LanServer;

void main()
{
LanServer.Start(dfNETWORK_SERVER_IP, dfNETWORK_SERVER_PORT, 4, false, 500);

while (!LanServer.IsShutdown())
{
LanServer.ServerControl();
LanServer.PrintState();
}

LanServer.Stop();
}
----------------------------------------------------------------------------*/
#ifndef __CNET_SERVER_H__
#define __CNET_SERVER_H__
#include "NetworkLib.h"

// sizeof(UINT64) == 8 Byte == 64 Bit
// [00000000 00000000 0000] [0000 00000000 00000000 00000000 00000000 00000000]
// 1. ���� 2.5 Byte = Index ����
// 2. ���� 5.5 Byte = SessionID ����
#define CreateSessionID(ID, Index)	(((UINT64)Index << 44) | ID)				
#define GetSessionIndex(SessionID)	((SessionID >> 44) & 0xfffff)
#define GetSessionID(SessionID)		(SessionID & 0x00000fffffffffff)

namespace mylib
{
	class CNetServer
	{
	public:
		CNetServer();
		virtual ~CNetServer();

		//////////////////////////////////////////////////////////////////////////
		// Server Control
		//
		//////////////////////////////////////////////////////////////////////////
		// Server ON/OFF
		bool Start(WCHAR * szIP, int iPort, int iWorkerThreadCnt, bool bNagle, __int64 iConnectMax, BYTE byCode, BYTE byPacketKey1, BYTE byPacketKey2);
		void Stop();

		// Monitoring (Bit operation)
		enum en_SERVER_MONITOR
		{
			en_CONNECT_CNT = 1,
			en_ACCEPT_CNT = 2,
			en_PACKET_TPS = 4,
			en_PACKETPOOL_SIZE = 8,
			en_ALL = 15
		};
		void PrintState(int iFlag = en_ALL);

	protected:
		// External Call
		bool SendPacket(UINT64 iSessionID, CNPacket *pPacket);
		bool SendPacket_Disconnect(UINT64 iSessionID, CNPacket *pPacket);

		//////////////////////////////////////////////////////////////////////////
		// Handler
		//
		// OnConnectRequest		: Accept ����, true/false ���� ���/�ź�
		// OnClientJoin			: Accept ����ó�� �Ϸ� ��, ���� ���� ����
		// OnClientLeave		: Disconnect ��, ���� ����
		// OnRecv				: ��Ŷ ���� ��, ��Ŷ ó��
		// OnSend				: ��Ŷ �۽� ��
		// OnError				: ���� �߻� ��
		//////////////////////////////////////////////////////////////////////////
		virtual bool OnConnectRequest(WCHAR* wszIP, int iPort) = 0;
		virtual void OnClientJoin(UINT64 SessionID) = 0;
		virtual void OnClientLeave(UINT64 SessionID) = 0;
		virtual void OnRecv(UINT64 SessionID, CNPacket * pPacket) = 0;
		virtual void OnSend(UINT64 SessionID, int iSendSize) = 0;
		virtual void OnError(int iErrCode, WCHAR * wszErr) = 0;

	private:
		//////////////////////////////////////////////////////////////////////////
		// Session
		//
		//////////////////////////////////////////////////////////////////////////
		// * Session Sync Structure
		//  Send�� 1ȸ�� �����ϴ� ����
		// 1. ������ ������ ���� X. �ӵ� ����
		// 2. �Ϸ� ���� ���� ����
		// 3. Non-Paged Memory �߻� �ּ�ȭ
		struct stIOREF
		{
			stIOREF(LONG64 iCnt, LONG64 bRelease)
			{
				this->iCnt = iCnt;
				this->bRelease = bRelease;
			}
			LONG64 iCnt;
			LONG64 bRelease;
		};

		struct stSESSION
		{
			stSESSION();
			virtual ~stSESSION();
			int			iIndex;		// ��Ʈ��ũ ó����
			UINT64		iSessionID;	// ������ ó����
			SOCKET		Socket;
			CRingBuffer	RecvQ;
			CLFQueue<CNPacket*> SendQ;
			OVERLAPPED	RecvOverlapped;
			OVERLAPPED	SendOverlapped;
			stIOREF*	stIO;

			LONG		bSendFlag;	// NP�޸� �ּ�ȭ, ����ȭ ���� ������ ���� Send�� 1ȸ�� �ϱ�� ���
			int			iSendPacketCnt;
			bool		bSendDisconnect;
			///LONG		bSendPassWorker; // WorkerThread�� Send �ѱ��
		};

		//  �ܺο��� ȣ���ϴ� �Լ�(SendPacket, DisconnectSession)���� �ʿ�
		stSESSION * ReleaseSessionLock(UINT64 iSessionID);
		void		ReleaseSessionFree(stSESSION* pSession);
		bool		ReleaseSession(stSESSION* pSession);
	protected:
		int			DisconnectSocket(SOCKET Socket);
		bool		DisconnectSession(UINT64 iSessionID);

	private:
		//////////////////////////////////////////////////////////////////////////
		// Network
		//
		//////////////////////////////////////////////////////////////////////////
		// IOCP Enrollment
		bool RecvPost(stSESSION* pSession);
		bool SendPost(stSESSION* pSession);
		// IOCP Completion Notice
		void RecvComplete(stSESSION* pSession, DWORD dwTransferred);
		void SendComplete(stSESSION* pSession, DWORD dwTransferred);
		// Network Thread
		static unsigned int CALLBACK MonitorThread(LPVOID pCLanServer); /// unused
		static unsigned int CALLBACK AcceptThread(LPVOID pCNetServer);
		static unsigned int CALLBACK WorkerThread(LPVOID pCNetServer);
		unsigned int MonitorThread_Process();
		unsigned int AcceptThread_Process();
		unsigned int WorkerThread_Process();


		//////////////////////////////////////////////////////////////////////////
		// Variable
		//
		//////////////////////////////////////////////////////////////////////////
		// Socket
		SOCKET				_ListenSocket;
		BOOL				_bServerOn;
		// IOCP
		HANDLE				_hIOCP;
		// Threads
		HANDLE				_hMonitorThread;
		HANDLE				_hAcceptThread;
		HANDLE*				_hWorkerThread;
		int					_iWorkerThreadMax;
		// Session
		stSESSION*			_SessionArr;
		UINT64				_iSessionID;
		CLFStack<UINT64>	_SessionStk;	// �� ���� ����� Freelist
	protected:
		LONG64				_lConnectMax;
		LONG64				_lConnectCnt;
		// Encrypt
		BYTE				_byCode;
		// Monitor
		LONG64				_lAcceptCnt;
		LONG64				_lAcceptTps;
		LONG64				_lRecvTps;
		LONG64				_lSendTps;
	};
}

#define MON_CONNECT_CNT		mylib::CNetServer::en_CONNECT_CNT
#define MON_ACCEPT_CNT		mylib::CNetServer::en_ACCEPT_CNT
#define MON_PACKET_TPS		mylib::CNetServer::en_PACKET_TPS
#define MON_PACKETPOOL_SIZE	mylib::CNetServer::en_PACKETPOOL_SIZE
#define MON_ALL				mylib::CNetServer::en_ALL
#endif