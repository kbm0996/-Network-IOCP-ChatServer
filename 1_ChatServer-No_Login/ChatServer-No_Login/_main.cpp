#include "ChatServer.h"
#include "CConfig.h"

CConfig* pConfig = CConfig::GetInstance();

void main()
{
	timeBeginPeriod(1);
	_wsetlocale(LC_ALL, L"kor");
	pConfig->LoadConfigFile("_ChatServer.cnf");

	CChatServer ChatServer;
	if (!ChatServer.Start())
		return;

	while (!ChatServer._bShutdown)
	{
		ChatServer.ServerControl();
		ChatServer.PrintState();
		//ChatServer.Heartbeat();
		Sleep(1000);
	}

	ChatServer.Stop();
	timeEndPeriod(1);
}