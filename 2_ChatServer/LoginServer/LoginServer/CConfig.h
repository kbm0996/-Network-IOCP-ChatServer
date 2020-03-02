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


			//////////////////////////////////////
			if (!Parser.SearchField("NETWORK"))
				return false;

			if (!Parser.GetValue("SERVER_NAME", _szServerName, sizeof(_szServerName)))
				return false;

			if (!Parser.GetValue("BIND_IP", _szBindIP, sizeof(_szBindIP)))
				return false;

			if (!Parser.GetValue("BIND_PORT", &_iBindPort))
				return false;

			if (!Parser.GetValue("LAN_BIND_IP", _szLanBindIP, sizeof(_szLanBindIP)))
				return false;

			if (!Parser.GetValue("LAN_BIND_PORT", &_iLanBindPort))
				return false;

			if (!Parser.GetValue("CHAT_SERVER_IP", _szChatServerIP, sizeof(_szChatServerIP)))
				return false;

			if (!Parser.GetValue("CHAT_SERVER_PORT", &_iChatServerPort))
				return false;

			if (!Parser.GetValue("MONITORING_SERVER_IP", _szMonitorServerIP, sizeof(_szMonitorServerIP)))
				return false;
			
			if (!Parser.GetValue("MONITORING_SERVER_PORT", &_iMonitorServerPort))
				return false;

			if (!Parser.GetValue("WORKER_THREAD", &_iWorkerThreadNo))
				return false;


			//////////////////////////////////////
			if (!Parser.SearchField("SYSTEM"))
				return false;

			if (!Parser.GetValue("CLIENT_MAX", &_iClientMax))
				return false;

			if (!Parser.GetValue("PACKET_CODE", &_byPacketCode))
				return false;
			
			if (!Parser.GetValue("PACKET_KEY1", &_byPacketKey1))
				return false;

			if (!Parser.GetValue("PACKET_KEY2", &_byPacketKey2))
				return false;

			if (!Parser.GetValue("DB_IP", _szDBIP, sizeof(_szDBIP)))
				return false;

			if (!Parser.GetValue("DB_ACCOUNT", _szDBAccount, sizeof(_szDBAccount)))
				return false;

			if (!Parser.GetValue("DB_PORT", &_iDBPort))
				return false;
			
			if (!Parser.GetValue("DB_PASS", _szDBPassword, sizeof(_szDBPassword)))
				return false;

			if (!Parser.GetValue("DB_NAME", _szDBName, sizeof(_szDBName)))
				return false;

			return true;
		}

		// :NETWORK
		//-----------------------------------
		// 본 서버군 이름 / ServerLink 정보의 이름과 같아야 함
		//-----------------------------------
		static WCHAR	_szServerName[20];

		//-----------------------------------
		// 로그인서버 Listen IP/PORT
		//
		// 이는 서버에서 클라이언트 접속용 Listen 소켓 Bind
		//-----------------------------------
		static WCHAR	_szBindIP[16];
		static int		_iBindPort;
		static WCHAR	_szLanBindIP[16];
		static int		_iLanBindPort;

		//-----------------------------------
		// 채팅서버 연결 IP/PORT
		//-----------------------------------
		static WCHAR	_szChatServerIP[16];
		static int		_iChatServerPort;

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

		//-----------------------------------
		// DB
		//-----------------------------------
		static WCHAR	_szDBIP[16];
		static WCHAR	_szDBAccount[64];
		static int		_iDBPort;
		static WCHAR	_szDBPassword[64];
		static WCHAR	_szDBName[64];
	};
}

extern mylib::CConfig* pConfig;
#endif