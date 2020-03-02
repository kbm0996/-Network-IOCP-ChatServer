#include "ChatServer.h"

void main()
{
	timeBeginPeriod(1);
	_wsetlocale(LC_ALL, L"");

	ULONGLONG lHeartBeatTick = GetTickCount64();

	CChatServer ChatServer;
	if (!ChatServer.Start())
		return;

	if (ChatServer._pLoginClient->Start(CConfig::_szLoginLanServerIP, CConfig::_iLoginLanServerPort, 4, false))
		ChatServer._pLoginClient->_bConnect = true;
	
	while (!ChatServer._bShutdown)
	{
		if (!ChatServer._pLoginClient->_bConnect)
		{
			if (ChatServer._pLoginClient->Start(CConfig::_szLoginLanServerIP, CConfig::_iLoginLanServerPort, 4, false))
				ChatServer._pLoginClient->_bConnect = true;
		}

		ChatServer.ServerControl();
		ChatServer.PrintState();
		ChatServer.Heartbeat();
		
		// Heartbeat
		ULONGLONG lCurTick = GetTickCount64();
		if (lCurTick > lHeartBeatTick + CChatServer::en_HEART_BEAT_INTERVAL)
		{
			lHeartBeatTick = lCurTick;

			// Player Heartbeat
			//for (auto Iter = _PlayerMap.begin(); Iter != _PlayerMap.end(); ++Iter)
			//{
			//	st_PLAYER* pPlayer = Iter->second;
			//	if (lCurTick > pPlayer->LastRecvTick + en_HEART_BEAT_INTERVAL)
			//	{
			//		LOG(L"CHAT_SERVER_LOG", LOG_DEBUG, L"Session %d Disconnect by Heartbeat", pPlayer->iSessionID);
			//		DisconnectSession(pPlayer->iSessionID);
			//	}
			//}

			// LoginServer Playerlist Clear
			ChatServer._pLoginClient->ClearOldLoginPlayer();
		}
		
		Sleep(1000);
	}

	ChatServer.Stop();
	timeEndPeriod(1);
}