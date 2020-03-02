#include "CLoginServer.h"


void main()
{
	timeBeginPeriod(1);
	_wsetlocale(LC_ALL, L"kor");
	
	CLoginServer LoginServer;
	if (!LoginServer.Start())
		return;

	while (!LoginServer._bShutdown)
	{
		LoginServer.ServerControl();
		LoginServer.PrintState();
		Sleep(1000);
	}

	LoginServer.Stop();
	timeEndPeriod(1);
}