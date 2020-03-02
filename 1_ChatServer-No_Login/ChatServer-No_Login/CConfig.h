#ifndef __SERVER_CONFIG__
#define __SERVER_CONFIG__

#include "CParser.h"

namespace mylib
{
	class CConfig
	{
	private:

		CConfig() {}
		virtual ~CConfig() {}
	public:
		static CConfig* GetInstance()
		{
			static CConfig Instance;
			return &Instance;
		}

		bool LoadConfigFile(char* szConfigFileName)
		{
			CParser Parser;
			if (!Parser.LoadFile(szConfigFileName))
				return false;

			// :NETWORK
			Parser.GetValue("SERVER_NAME", _szServerName, sizeof(_szServerName));

			Parser.GetValue("BIND_IP", _szBindIP, sizeof(_szBindIP));
			Parser.GetValue("BIND_PORT", &_iBindPort);

			Parser.GetValue("LOGIN_SERVER_IP", _szLoginServerIP, sizeof(_szLoginServerIP));
			Parser.GetValue("LOGIN_SERVER_PORT", &_iLoginServerPort);

			Parser.GetValue("LOGIN_SERVER_IP", _szLoginLanServerIP, sizeof(_szLoginLanServerIP));
			Parser.GetValue("LOGIN_LAN_SERVER_PORT", &_iLoginLanServerPort);

			Parser.GetValue("MONITORING_SERVER_IP", _szMonitorServerIP, sizeof(_szMonitorServerIP));
			Parser.GetValue("MONITORING_SERVER_PORT", &_iMonitorServerPort);

			Parser.GetValue("WORKER_THREAD", &_iWorkerThreadNo);

			// :SYSTEM
			Parser.GetValue("CLIENT_MAX", &_iClientMax);

			Parser.GetValue("PACKET_CODE", &_byPacketCode);
			Parser.GetValue("PACKET_KEY1", &_byPacketKey1);
			Parser.GetValue("PACKET_KEY2", &_byPacketKey2);


			return true;
		}

		// :NETWORK
		//-----------------------------------
		// 본 서버군 이름 / ServerLink 정보의 이름과 같아야 함
		//-----------------------------------
		static WCHAR	_szServerName[20];

		//-----------------------------------
		// 채팅서버 Listen IP/PORT
		//
		// 이는 서버에서 클라이언트 접속용 Listen 소켓 Bind
		//-----------------------------------
		static WCHAR	_szBindIP[16];
		static int		_iBindPort;

		//-----------------------------------
		// 로그인서버 연결 IP/PORT
		//-----------------------------------
		static WCHAR	_szLoginServerIP[16];
		static int		_iLoginServerPort;

		static WCHAR	_szLoginLanServerIP[16];
		static int		_iLoginLanServerPort;

		//-----------------------------------
		// 모니터링 서버 연결 IP/PORT
		//-----------------------------------
		static WCHAR	_szMonitorServerIP[16];
		static int		_iMonitorServerPort;

		//-----------------------------------
		// IOCP 워커스레드 개수
		//-----------------------------------
		static int		_iWorkerThreadNo;


		//:SYSTEM
		//-----------------------------------
		// 최대사용자
		//-----------------------------------
		static int		_iClientMax;

		//-----------------------------------
		// Packet Encode Key
		//-----------------------------------
		static int		_byPacketCode;
		static int		_byPacketKey1;
		static int		_byPacketKey2;
	};
}
#endif