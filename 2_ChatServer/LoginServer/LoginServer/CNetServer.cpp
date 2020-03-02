﻿#include "CNetServer.h"

mylib::CNetServer::CNetServer()
{
	_bServerOn = 0;
	_iSessionID = 0;
	_ListenSocket = INVALID_SOCKET;
	_lConnectCnt = 0;
	_lAcceptCnt = 0;
	_lAcceptTps = 0;
	_lRecvTps = 0;
	_lSendTps = 0;
}

mylib::CNetServer::~CNetServer()
{
}

bool mylib::CNetServer::Start(WCHAR * szIP, int iPort, int iWorkerThreadCnt, bool bNagle, __int64 iConnectMax, BYTE byCode, BYTE byKey1, BYTE byKey2)
{
	if (_ListenSocket != INVALID_SOCKET)
		return false;

	_iWorkerThreadMax = iWorkerThreadCnt;
	_lConnectMax = iConnectMax;

	// Encryption Init
	CNPacket::SetEncodingCode(byCode, byKey1, byKey2);
	_byCode = byCode;

	// Session Init
	_SessionArr = new stSESSION[iConnectMax];
	for (int i = (int)iConnectMax - 1; i >= 0; --i)
	{
		_SessionArr[i].iIndex = i;
		_SessionStk.Push(i);
	}

	// Winsock init
	WSADATA wsa;
	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"WSAStartup() ErrorCode:%d", WSAGetLastError());
		return false;
	}

	_ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_ListenSocket == INVALID_SOCKET)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"socket() ErrorCode:%d", WSAGetLastError());
		WSACleanup();
		return false;
	}

	// Server on
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(iPort);
	InetPton(AF_INET, szIP, reinterpret_cast<PVOID>(&serveraddr.sin_addr));
	if (bind(_ListenSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"bind() ErrorCode:%d", WSAGetLastError());
		closesocket(_ListenSocket);
		_ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	if (listen(_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"listen() ErrorCode:%d", WSAGetLastError());
		closesocket(_ListenSocket);
		_ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	// SOL_SOCKET - SO_SNDBUF
	//  소켓의 송신 버퍼 크기를 0으로 만들면 소켓 버퍼를 건너뛰고 프로토콜 스택의 송신 버퍼에 다이렉트로 전달되어 성능 향상.
	// 단, 수신 버퍼의 크기는 건들지 않음. 도착지를 잃고 수신 파트는 프로그램 구조상 서버측에서 제어하는게 아니라 
	// 클라가 주는대로 받아야 하므로 패킷 처리 속도가 수신 속도를 따라오지 못할 수 있음.
	///int iBufSize = 0;
	///int iOptLen = sizeof(iBufSize);
	///setsockopt(_ListenSocket, SOL_SOCKET, SO_SNDBUF, (char *)&iBufSize, iOptLen);

	// accept() 함수가 리턴하는 소켓은 연결 대기 소켓과 동일한 속성을 갖게 된다
	setsockopt(_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&bNagle, sizeof(bNagle));

	// IOCP init
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (_hIOCP == NULL)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"CreateIoCompletionPort() ErrorCode:%d", GetLastError());
		closesocket(_ListenSocket);
		_ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	// Thread start
	_hAcceptThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL));

	_hWorkerThread = new HANDLE[_iWorkerThreadMax];
	for (int i = 0; i < iWorkerThreadCnt; ++i)
		_hWorkerThread[i] = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL));

	_bServerOn = TRUE;

	LOG(L"SYSTEM", LOG_SYSTM, L"Server Start");
	return true;
}

void mylib::CNetServer::Stop()
{
	// Server off
	_bServerOn = FALSE;
	shutdown(_ListenSocket, SD_BOTH);
	closesocket(_ListenSocket);

	// AcceptThread Exit
	WaitForSingleObject(_hAcceptThread, INFINITE);
	CloseHandle(_hAcceptThread);

	// WorkerThread Exit
	for (int i = 0; i < _iWorkerThreadMax; ++i)  // WorkerThread 종료 메세지를 IOCP Queue에 임의 삽입
		PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	WaitForMultipleObjects(_iWorkerThreadMax, _hWorkerThread, TRUE, INFINITE);
	for (int i = 0; i < _iWorkerThreadMax; ++i)
		CloseHandle(_hWorkerThread[i]);

	// Winsock close
	WSACleanup();

	// IOCP close
	CloseHandle(_hIOCP);

	// Session Release
	for (int i = 0; i < _lConnectMax; ++i)
	{
		if (_SessionArr[i].Socket != INVALID_SOCKET)
			ReleaseSession(&_SessionArr[i]);
	}
	if (_SessionArr != nullptr)
	{
		delete[] _SessionArr;
		_SessionArr = NULL;
	}
	if (_hWorkerThread != nullptr)
		delete[] _hWorkerThread;

	_iSessionID = 0;
	_ListenSocket = INVALID_SOCKET;
	_iWorkerThreadMax = 0;
	_lConnectCnt = 0;
	_lAcceptCnt = 0;
	_lAcceptTps = 0;
	_lRecvTps = 0;
	_lSendTps = 0;
	_SessionStk.Clear();

	LOG(L"SYSTEM", LOG_DEBUG, L"Server Stop");
}

void mylib::CNetServer::PrintState(int iFlag)
{
	if (iFlag & en_ALL)
	{
		wprintf(L"───────────────────────────────────────────\n");
		wprintf(L" * Net Server\n");
		if (iFlag & en_CONNECT_CNT)
		{
			//  TODO : 대량으로 접속을 끊으면 신호가 lost가 되어 ConnectSession이 남는 경우가 있음
			// netstat으로 포트 확인하고 ESTABLISH 개수 확인 : TCP 차원에서 끊어진게 맞는지, 내 코드상의 문제인지 파악
			// ESTABLISH와 ConnectSession이 다르면, 로직 상에서 Heartbeat, TCP 차원에서는 KeepAlive. 같으면, 코드상의 문제
			wprintf(L" - ConnectSession	: %lld \n", _lConnectCnt);	// 접속된 세션이 몇인가	 ++, -- // LanServer
			wprintf(L"\n");
		}
		if (iFlag & en_ACCEPT_CNT)
		{
			wprintf(L" - Accept TPS		: %lld \n", _lAcceptTps);		// 직접 카운팅
			wprintf(L" - Accept Total		: %lld \n", _lAcceptCnt);	// 누적량 카운팅 
			wprintf(L"\n");
			_lAcceptTps = 0;
		}
		if (iFlag & en_PACKET_TPS)
		{
			wprintf(L" - RecvPacket TPS	: %lld \n", _lRecvTps);	// 초당 Recv 패킷 개수 - 완료통지 왔을때 Cnt, *접근 스레드가 다수이므로 interlocked 계열 함수 사용
			wprintf(L" - SendPacket TPS	: %lld \n", _lSendTps);	/// WSASend를 호출할 때 애매함 - 보류, *접근 스레드가 다수이므로 interlocked 계열 함수 사용
			_lRecvTps = 0;
			_lSendTps = 0;
		}
		if (iFlag & en_PACKETPOOL_SIZE)
			wprintf(L" - PacketPool Use	: %d \n", CNPacket::GetUseSize());
	}
}

bool mylib::CNetServer::SendPacket(UINT64 iSessionID, CNPacket * pPacket)
{
	stSESSION *pSession = ReleaseSessionLock(iSessionID);
	if (pSession == nullptr)
		return false;

	pPacket->Encode();
	pPacket->AddRef();
	pSession->SendQ.Enqueue(pPacket);
	SendPost(pSession);

	///if (InterlockedCompareExchange(&pSession->bSendPassWorker, TRUE, FALSE) == FALSE)
	///{
	///	InterlockedIncrement64(&pSession->stIO->iCnt);
	///	// WorkerThread를 깨워서 SendPost 상태로 전환, Send 한꺼번에 처리하기
	///	PostQueuedCompletionStatus(_hIOCP, 1, (ULONG_PTR)pSession, (LPOVERLAPPED)pSession->iSessionID);
	///}

	ReleaseSessionFree(pSession);
	return true;
}

bool mylib::CNetServer::SendPacket_Disconnect(UINT64 iSessionID, CNPacket * pPacket)
{
	stSESSION *pSession = ReleaseSessionLock(iSessionID);
	if (pSession == nullptr)
		return false;

	pPacket->Encode();
	pPacket->AddRef();
	pSession->SendQ.Enqueue(pPacket);
	SendPost(pSession);

	///if (InterlockedCompareExchange(&pSession->bSendPassWorker, TRUE, FALSE) == FALSE)
	///{
	///	InterlockedIncrement64(&pSession->stIO->iCnt);
	///	// WorkerThread를 깨워서 SendPost 상태로 전환, Send 한꺼번에 처리하기
	///	PostQueuedCompletionStatus(_hIOCP, 1, (ULONG_PTR)pSession, (LPOVERLAPPED)pSession->iSessionID);
	///}

	pSession->bSendDisconnect = true;

	ReleaseSessionFree(pSession);
	return true;
}

mylib::CNetServer::stSESSION * mylib::CNetServer::ReleaseSessionLock(UINT64 iSessionID)
{
	int iIndex = GetSessionIndex(iSessionID);
	// Multi-Thread 환경에서 Release, Connect, Send, Accept 등이 ContextSwitching으로 인해 발생 시점이 사실상 무작위
	stSESSION *pSession = &_SessionArr[iIndex];

	/************************************************************/
	/* 각종 세션 상태에서 다른 스레드가 Release를 시도했을 경우 */
	/************************************************************/
	// **CASE 0 : Release 플래그가 TRUE로 확실히 바뀐 상태
	if (pSession->stIO->bRelease == TRUE)
		return nullptr;

	// **CASE 1 : ReleaseSession 내에 있을 경우
	// 이때 다른 스레드에서 SendPacket이나 Disconnect 시도 시, IOCnt가 1이 될 수 있음
	if (InterlockedIncrement64(&pSession->stIO->iCnt) == 1)
	{
		// 증가시킨 IOCnt를 다시 감소시켜 복구
		if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
			ReleaseSession(pSession);	// Release를 중첩으로 하는 문제는 Release 내에서 처리
		return nullptr;
	}

	// **CASE 2 : 이미 Disconnect 된 다음 Accept하여 새로 Session이 들어왔을 경우
	if (pSession->iSessionID != iSessionID)	// 현 SessionID와 ContextSwitching 발생 이전의 SessionID를 비교
	{
		if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
			ReleaseSession(pSession);
		return nullptr;
	}

	// **CASE 3 : Release 플래그가 FALSE임이 확실한 상태
	if (pSession->stIO->bRelease == FALSE)
		return pSession;

	// **CASE 4 : CASE 3에 진입하기 직전에 TRUE로 바뀌었을 경우
	if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
		ReleaseSession(pSession);
	return nullptr;
}

void mylib::CNetServer::ReleaseSessionFree(stSESSION * pSession)
{
	if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
		ReleaseSession(pSession);
	return;
}

bool mylib::CNetServer::ReleaseSession(stSESSION * pSession)
{
	stIOREF stComparentIO(0, FALSE);
	if (!InterlockedCompareExchange128((LONG64*)pSession->stIO, TRUE, 0, (LONG64*)&stComparentIO))
		return false;

	// TODO : shutdown()
	//  shutdown() 함수는 4HandShake(1Fin - 2Ack+3Fin - 4Fin)에서 1Fin을 보내는 함수
	// 1. 상대편에서 이에 대한 Ack를 보내지 않으면(대게 상대방이 '응답없음'으로 먹통됐을 경우) 종료가 되지 않음
	//   - 이 현상(연결 끊어야하는데 안끊어지는 경우)이 지속되면 메모리가 순간적으로 증가하고 SendBuffer가 터질 수 있음
	// 2. TimeWait이 남음
	//   - CancelIoEx 사용하여 4HandShake를 무시하고 닫아야 남지 않는다
	DisconnectSocket(pSession->Socket);

	// TODO : CancelIoEx()
	//  shutdown으로는 닫을 수 없을 경우 사용 (SendBuffer가 가득 찼을때, Heartbeat가 끊어졌을때)
	// CancelIoEx()과 WSARecv() 간에 경합이 발생한다. 경합이 발생하더라도 shutdown()이 WSARecv()를 실패하도록 유도한다.
	CancelIoEx(reinterpret_cast<HANDLE>(pSession->Socket), NULL);

	OnClientLeave(pSession->iSessionID);

	CNPacket *pPacket = nullptr;
	while (pSession->SendQ.Dequeue(pPacket))
	{
		if (pPacket != nullptr)
			pPacket->Free();
		pPacket = nullptr;
	}

	InterlockedExchange(&pSession->bSendFlag, FALSE);
	pSession->iSessionID = -1;
	pSession->Socket = INVALID_SOCKET;
	_SessionStk.Push(pSession->iIndex);
	InterlockedDecrement64(&_lConnectCnt);
	return true;
}

int mylib::CNetServer::DisconnectSocket(SOCKET Socket)
{
	return shutdown(Socket, SD_BOTH);
}

bool mylib::CNetServer::DisconnectSession(UINT64 iSessionID)
{
	stSESSION *pSession = ReleaseSessionLock(iSessionID);
	if (pSession == nullptr)
		return false;

	DisconnectSocket(pSession->Socket);

	ReleaseSessionFree(pSession);
	return true;
}

bool mylib::CNetServer::RecvPost(stSESSION * pSession)
{
	WSABUF wsabuf[2];
	int iBufCnt = 1;
	wsabuf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	wsabuf[0].len = pSession->RecvQ.GetUnbrokenEnqueueSize();
	if (pSession->RecvQ.GetUnbrokenEnqueueSize() < pSession->RecvQ.GetFreeSize())
	{
		wsabuf[1].buf = pSession->RecvQ.GetBufferPtr();
		wsabuf[1].len = pSession->RecvQ.GetFreeSize() - wsabuf[0].len;
		++iBufCnt;
	}

	DWORD dwTransferred = 0;
	DWORD dwFlag = 0;
	ZeroMemory(&pSession->RecvOverlapped, sizeof(pSession->RecvOverlapped));
	InterlockedIncrement64(&pSession->stIO->iCnt);
	if (WSARecv(pSession->Socket, wsabuf, iBufCnt, &dwTransferred, &dwFlag, &pSession->RecvOverlapped, NULL) == SOCKET_ERROR) // 성공 : 0, 실패 : SOCKET_ERROR
	{
		int err = WSAGetLastError();
		// WSA_IO_PENDING(997) : 비동기 입출력 중. Overlapped 연산은 나중에 완료될 것이다. 중첩 연산을 위한 준비가 되었으나, 즉시 완료되지 않았을 경우 발생
		if (err != WSA_IO_PENDING)
		{
			// WSAENOTSOCK(10038) : 소켓이 아닌 항목에 소켓 작업 시도
			// WSAECONNABORTED(10053) : 호스트가 연결 중지. 데이터 전송 시간 초과, 프로토콜 오류 발생
			// WSAECONNRESET(10054) : 원격 호스트에 의해 기존 연결 강제 해제. 원격 호스트가 갑자기 중지되거나 다시 시작되거나 하드 종료
			// WSAESHUTDOWN(10058) : 이미 종료된 소켓에 대한 작업
			if (err != 10038 && err != 10053 && err != 10054 && err != 10058)
				LOG(L"SYSTEM", LOG_ERROR, L"WSARecv() ErrorCode:%d / Socket:%d / RecvQ Size:%d", err, pSession->Socket, pSession->RecvQ.GetUseSize());

			if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
				ReleaseSession(pSession);

			return false;
		}
	}
	return true;
}

bool mylib::CNetServer::SendPost(stSESSION * pSession)
{
	while (1)
	{
		if (pSession->SendQ.GetUseSize() == 0)
			return false;
		if (InterlockedCompareExchange(&pSession->bSendFlag, TRUE, FALSE) == TRUE)
			return false;
		if (pSession->SendQ.GetUseSize() == 0)
		{
			InterlockedExchange(&pSession->bSendFlag, FALSE);
			continue;
		}
		break;
	}

	int iBufCnt;
	WSABUF wsabuf[100];
	CNPacket * pPacket;
	int iPacketCnt = pSession->SendQ.GetUseSize();
	for (iBufCnt = 0; iBufCnt < iPacketCnt && iBufCnt < 100; ++iBufCnt)
	{
		if (!pSession->SendQ.Peek(pPacket, iBufCnt))
			break;
		wsabuf[iBufCnt].buf = pPacket->GetHeaderPtr();
		wsabuf[iBufCnt].len = pPacket->GetPacketSize();
	}
	pSession->iSendPacketCnt = iBufCnt;

	DWORD dwTransferred;
	ZeroMemory(&pSession->SendOverlapped, sizeof(pSession->SendOverlapped));
	InterlockedIncrement64(&pSession->stIO->iCnt);
	if (WSASend(pSession->Socket, wsabuf, iBufCnt, &dwTransferred, 0, &pSession->SendOverlapped, NULL) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		// WSA_IO_PENDING(997) : 비동기 입출력 중. Overlapped 연산은 나중에 완료될 것이다. 중첩 연산을 위한 준비가 되었으나, 즉시 완료되지 않았을 경우 발생
		if (err != WSA_IO_PENDING)
		{
			// WSAENOTSOCK(10038) : 소켓이 아닌 항목에 소켓 작업 시도
			// WSAECONNABORTED(10053) : 호스트가 연결 중지. 데이터 전송 시간 초과, 프로토콜 오류 발생
			// WSAECONNRESET(10054) : 원격 호스트에 의해 기존 연결 강제 해제. 원격 호스트가 갑자기 중지되거나 다시 시작되거나 하드 종료
			// WSAESHUTDOWN(10058) : 이미 종료된 소켓에 대한 작업
			if (err != 10038 && err != 10053 && err != 10054 && err != 10058)
				LOG(L"SYSTEM", LOG_ERROR, L"WSASend() # ErrorCode:%d / Socket:%d / IOCnt:%d / SendQ Size:%d", err, pSession->Socket, pSession->stIO->iCnt, pSession->SendQ.GetUseSize());

			if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
				ReleaseSession(pSession);

			return false;
		}
	}
	return true;
}

void mylib::CNetServer::RecvComplete(stSESSION * pSession, DWORD dwTransferred)
{
	// 받은 만큼 RecvQ.MoveWritePos 이동
	pSession->RecvQ.MoveWritePos(dwTransferred);

	st_PACKET_HEADER stHeader;
	while (1)
	{
		// 1. RecvQ에 'sizeof(Header)'만큼 있는지 확인
		int iRecvSize = pSession->RecvQ.GetUseSize();
		if (iRecvSize < sizeof(stHeader))
		{
			// 더 이상 처리할 패킷이 없음
			break;
		}

		// 2. Packet 길이 확인 : Header크기('sizeof(Header)') + Payload길이('Header.wLen')
		pSession->RecvQ.Peek(reinterpret_cast<char*>(&stHeader), sizeof(stHeader));
		if (iRecvSize < sizeof(stHeader) + stHeader.wLen)
		{
			// 데이터가 덜 오거나, 위변조된 패킷
			break;
		}
		pSession->RecvQ.MoveReadPos(sizeof(stHeader));

		///////////////////////////////////////////////////////////
		//  PacketCode 확인
		if (stHeader.byCode != _byCode)
		{
			LOG(L"SYSTEM", LOG_WARNG, L"PacketCode mismatch");
			DisconnectSocket(pSession->Socket);
			return;
		}
		///////////////////////////////////////////////////////////

		// 3. Payload 길이가 버퍼 최대 크기보다 클 경우
		if (stHeader.wLen  > CNPacket::en_BUFFER_DEFAULT_SIZE)
		{
			LOG(L"SYSTEM", LOG_WARNG, L"PacketBufferSize < PayloadSize");
			DisconnectSocket(pSession->Socket);
			return;
		}

		// 4. PacketPool에 Packet 포인터 할당
		CNPacket *pPacket = CNPacket::Alloc();
		pSession->RecvQ.Dequeue(pPacket->GetPayloadPtr(), stHeader.wLen);
		pPacket->MoveWritePos(stHeader.wLen);

		InterlockedIncrement64(&_lRecvTps);

		///////////////////////////////////////////////////////////
		// Decode
		if (!pPacket->Decode(&stHeader))
		{
			LOG(L"SYSTEM", LOG_WARNG, L"Decode Fail");
			pPacket->Free();
			DisconnectSocket(pSession->Socket);
			return;
		}
		///////////////////////////////////////////////////////////

		// 5. Packet 처리
		OnRecv(pSession->iSessionID, pPacket);
		pPacket->Free();

	}

	// SessionSocket을 recv 상태로 변경
	RecvPost(pSession);
}

void mylib::CNetServer::SendComplete(stSESSION * pSession, DWORD dwTransferred)
{
	OnSend(pSession->iSessionID, dwTransferred);

	// 보낸 패킷 수 만큼 지우기
	CNPacket *pPacket;
	for (int i = 0; i < pSession->iSendPacketCnt; ++i)
	{
		pPacket = nullptr;
		if (pSession->SendQ.Dequeue(pPacket))
		{
			pPacket->Free();
			InterlockedIncrement64(&_lSendTps);
		}
	}

	InterlockedExchange(&pSession->bSendFlag, FALSE);

	if (pSession->bSendDisconnect == true && pSession->SendQ.GetUseSize() == 0)
	{
		DisconnectSocket(pSession->Socket);
		return;
	}

	if (pSession->SendQ.GetUseSize() != 0)
		SendPost(pSession);
}

unsigned int mylib::CNetServer::MonitorThread(LPVOID pCNetServer)
{
	return ((CNetServer *)pCNetServer)->MonitorThread_Process();
}

unsigned int mylib::CNetServer::AcceptThread(LPVOID pCNetServer)
{
	return ((CNetServer *)pCNetServer)->AcceptThread_Process();
}

unsigned int mylib::CNetServer::WorkerThread(LPVOID pCNetServer)
{
	return ((CNetServer *)pCNetServer)->WorkerThread_Process();
}

unsigned int mylib::CNetServer::MonitorThread_Process()
{
	LOG(L"SYSTEM", LOG_DEBUG, L"MonitorThread Exit");
	return 0;
}

unsigned int mylib::CNetServer::AcceptThread_Process()
{
	while (1)
	{
		SOCKADDR_IN SessionAddr;
		int iAddrlen = sizeof(SessionAddr);
		SOCKET SessionSocket = WSAAccept(_ListenSocket, reinterpret_cast<sockaddr*>(&SessionAddr), &iAddrlen, NULL, NULL);
		if (!_bServerOn)
			break;
		if (SessionSocket == INVALID_SOCKET)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"WSAAccept() failed%d", WSAGetLastError());
			break;
		}

		++_lAcceptTps;
		++_lAcceptCnt;

		if (_lConnectCnt >= _lConnectMax)
		{
			LOG(L"SYSTEM", LOG_SYSTM, L"Connect Full %d/%d", _lConnectCnt, _lConnectMax);
			closesocket(SessionSocket);
			continue;
		}

		WCHAR wszIP[INET_ADDRSTRLEN];
		InetNtop(AF_INET, &SessionAddr.sin_addr, wszIP, INET_ADDRSTRLEN);
		int iPort = ntohs(SessionAddr.sin_port);
		if (!OnConnectRequest(wszIP, iPort))
		{
			LOG(L"SYSTEM", LOG_SYSTM, L"Blocked IP:%s:%d", wszIP, iPort);
			closesocket(SessionSocket);
			continue;
		}

		// TODO : TCP 소켓에서 좀비세션 처리
		//  좀비 세션은 비정상적으로 종료(대게 랜선 문제, 외부에서 세션을 일부러 끊은 경우)되어 
		// 서버나 클라이언트에서 세션이 종료되었는지 모르는 상태의 세션을 의미(Close 이벤트를 못받음)
		// TCP Keepalive 사용 
		// - SO_KEEPALIVE : 시스템 레지스트리 값 변경. 시스템의 모든 SOCKET에 대해서 KEEPALIVE 설정
		// - SIO_KEEPALIVE_VALS : 특정 SOCKET만 KEEPALIVE 설정
		tcp_keepalive tcpkl;
		tcpkl.onoff = TRUE;
		tcpkl.keepalivetime = 30000; // ms
		tcpkl.keepaliveinterval = 1000;
		WSAIoctl(SessionSocket, SIO_KEEPALIVE_VALS, &tcpkl, sizeof(tcpkl), 0, 0, NULL, NULL, NULL);

		// Session Alloc
		UINT64 nIndex = -1;
		_SessionStk.Pop(nIndex);
		stSESSION *pSession = &_SessionArr[nIndex];
		pSession->Socket = SessionSocket;
		pSession->iSessionID = CreateSessionID(++_iSessionID, nIndex);
		InterlockedIncrement64(&pSession->stIO->iCnt);
		pSession->RecvQ.Clear();
		pSession->SendQ.Clear();
		pSession->bSendFlag = 0;
		pSession->iSendPacketCnt = 0;
		pSession->bSendDisconnect = FALSE;
		///pSession->bSendPassWorker = FALSE;
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(SessionSocket), _hIOCP, reinterpret_cast<ULONG_PTR>(pSession), 0) == NULL)
		{
			//  HANDLE 인자에 소켓이 아닌 값이 올 경우 잘못된 핸들(6번 에러) 발생. 
			// 소켓이 아닌 값을 넣었다는 것은 다른 스레드에서 소켓을 반환했다는 의미이므로  동기화 문제일 가능성이 높다.
			LOG(L"SYSTEM", LOG_ERROR, L"Session IOCP Enrollment ErrorCode:%d", GetLastError());
			if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
				ReleaseSession(pSession);
			continue;
		}
		pSession->stIO->bRelease = FALSE;

		OnClientJoin(pSession->iSessionID);

		// SessionSocket을 recv 상태로 변경
		RecvPost(pSession);

		// Accept하면서 Accept 패킷을 보내면서 IOCount가 1이 증가하므로 다시 1 감소
		if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
			ReleaseSession(pSession);

		InterlockedIncrement64(&_lConnectCnt);
	}
	LOG(L"SYSTEM", LOG_DEBUG, L"AcceptThread Exit");
	return 0;
}

unsigned int mylib::CNetServer::WorkerThread_Process()
{
	while (1)
	{
		// GQCS 실패시 이전 값을 그대로 반환 → 인자값 초기화 필수
		DWORD			dwTransferred = 0;
		stSESSION *		pSession = nullptr;
		LPOVERLAPPED	pOverlapped = 0;
		/*----------------------------------------------------------------------------------------------------------------
		// GQCS 리턴값 //

		(1) IOCP Queue로부터 완료 패킷 dequeue 성공일 경우.
		-> TRUE 리턴, lpCompletionKey 세팅

		(2) IOCP Queue로부터 완료 패킷 dequeue 실패
		&& lpOverlapped가 NULL(0)일 경우.
		-> FALSE 리턴

		(3) IOCP Queue로부터 완료 패킷 dequeue 성공
		&& lpOverlapped가 NOT_NULL
		&& dequeue한 요소(메세지)가 실패한 I/O일 경우.
		-> FALSE 리턴, lpCompletionKey 세팅

		(4) IOCP에 등록된 Socket이 close됐을 경우.
		-> FALSE 리턴, lpOverlapped에 NOT_NULL, lpNumberOfBytes에 0 세팅
		----------------------------------------------------------------------------------------------------------------*/
		BOOL bResult = GetQueuedCompletionStatus(_hIOCP, &dwTransferred, reinterpret_cast<PULONG_PTR>(&pSession), &pOverlapped, INFINITE);
		if (pOverlapped == 0)
		{
			// (2)
			if (bResult == FALSE)
			{
				LOG(L"SYSTEM", LOG_ERROR, L"GetQueuedCompletionStatus() ErrorCode:%d", GetLastError());
				PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);	// GQCS 에러 시 조치를 취할만한게 없으므로 종료
				break;
			}

			// (x) 정상 종료 (PQCS로 지정한 임의 메세지)
			if (dwTransferred == 0 && pSession == 0)
			{
				PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
				break;
			}
		}

		// (3)(4) 세션 종료
		if (dwTransferred == 0)
		{
			DisconnectSocket(pSession->Socket);
		}
		// (1)
		else
		{
			/// WorkerThread 이외 다른 쓰레드로부터 SendPacket을 호출
			///if (pOverlapped == reinterpret_cast<LPOVERLAPPED>(pSession->iSessionID))
			///{
			///	SendPost(pSession);
			///	InterlockedExchange(&pSession->bSendPassWorker, FALSE);
			///}

			// * GQCS Send/Recv 완료 통지 처리
			//  완료 통지 : TCP/IP suite가 관리하는 버퍼에 복사 성공
			if (pOverlapped == &pSession->RecvOverlapped)
				RecvComplete(pSession, dwTransferred);

			if (pOverlapped == &pSession->SendOverlapped)
				SendComplete(pSession, dwTransferred);
		}

		if (InterlockedDecrement64(&pSession->stIO->iCnt) == 0)
			ReleaseSession(pSession);

	}
	LOG(L"SYSTEM", LOG_DEBUG, L"WorkerThread Exit");
	return 0;
}

mylib::CNetServer::stSESSION::stSESSION()
{
	iSessionID = -1;
	Socket = INVALID_SOCKET;

	stIO = (stIOREF*)_aligned_malloc(sizeof(stIOREF), 16);

	stIO->iCnt = 0;
	stIO->bRelease = FALSE;

	RecvQ.Clear();
	SendQ.Clear();
	bSendFlag = FALSE;
	iSendPacketCnt = 0;
	///bSendPassWorker = FALSE;
}

mylib::CNetServer::stSESSION::~stSESSION()
{
	_aligned_free(stIO);
}