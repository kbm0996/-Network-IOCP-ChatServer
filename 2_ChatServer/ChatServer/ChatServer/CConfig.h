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

			if (!Parser.GetValue("LOGIN_SERVER_IP", _szLoginServerIP, sizeof(_szLoginServerIP))) 
				return false;
			if (!Parser.GetValue("LOGIN_SERVER_PORT", &_iLoginServerPort)) 
				return false;

			if (!Parser.GetValue("LOGIN_SERVER_IP", _szLoginLanServerIP, sizeof(_szLoginLanServerIP))) 
				return false;
			if (!Parser.GetValue("LOGIN_LAN_SERVER_PORT", &_iLoginLanServerPort)) 
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

			if (!Parser.GetValue("INTERVAL_HEARTBEAT", &_iIntervalHeatbeat)) 
				return false;

			return true;
		}

		// :NETWORK
		//-----------------------------------
		// �� ������ �̸� / ServerLink ������ �̸��� ���ƾ� ��
		//-----------------------------------
		static WCHAR	_szServerName[20];

		//-----------------------------------
		// ä�ü��� Listen IP/PORT
		//
		// �̴� �������� Ŭ���̾�Ʈ ���ӿ� Listen ���� Bind
		//-----------------------------------
		static WCHAR	_szBindIP[16];
		static int		_iBindPort;

		//-----------------------------------
		// �α��μ��� ���� IP/PORT
		//-----------------------------------
		static WCHAR	_szLoginServerIP[16];
		static int		_iLoginServerPort;

		static WCHAR	_szLoginLanServerIP[16];
		static int		_iLoginLanServerPort;

		//-----------------------------------
		// ����͸� ���� ���� IP/PORT
		//-----------------------------------
		static WCHAR	_szMonitorServerIP[16];
		static int		_iMonitorServerPort;

		//-----------------------------------
		// IOCP ��Ŀ������ ����
		//-----------------------------------
		static int		_iWorkerThreadNo;


		//:SYSTEM
		//-----------------------------------
		// �ִ�����
		//-----------------------------------
		static int		_iClientMax;

		//-----------------------------------
		// Packet Encode Key
		//-----------------------------------
		static int		_byPacketCode;
		static int		_byPacketKey1;
		static int		_byPacketKey2;

		//-----------------------------------
		// Heartbeat timeout
		//-----------------------------------
		static int		_iIntervalHeatbeat;
	};
}

extern mylib::CConfig* pConfig;
#endif