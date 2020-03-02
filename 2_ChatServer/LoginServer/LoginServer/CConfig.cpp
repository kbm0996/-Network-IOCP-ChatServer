#include "CConfig.h"

using namespace mylib;

// :NETWORK
//-----------------------------------
// �� ������ �̸� / ServerLink ������ �̸��� ���ƾ� ��
//-----------------------------------
WCHAR	CConfig::_szServerName[20];

//-----------------------------------
// �α��μ��� Listen IP/PORT
//
// �̴� �������� Ŭ���̾�Ʈ ���ӿ� Listen ���� Bind
//-----------------------------------
WCHAR	CConfig::_szBindIP[16];
int		CConfig::_iBindPort;
WCHAR	CConfig::_szLanBindIP[16];
int		CConfig::_iLanBindPort;

//-----------------------------------
// ä�ü��� ���� IP/PORT
//-----------------------------------
WCHAR	CConfig::_szChatServerIP[16];
int		CConfig::_iChatServerPort;

//-----------------------------------
// ����͸� ���� ���� IP/PORT
//-----------------------------------
WCHAR	CConfig::_szMonitorServerIP[16];
int		CConfig::_iMonitorServerPort;

//-----------------------------------
// IOCP ��Ŀ������ ����
//-----------------------------------
int		CConfig::_iWorkerThreadNo;


//:SYSTEM
//-----------------------------------
// �ִ�����
//-----------------------------------
int		CConfig::_iClientMax;

//-----------------------------------
// Packet Encode Key
//-----------------------------------
int		CConfig::_byPacketCode;
int		CConfig::_byPacketKey1;
int		CConfig::_byPacketKey2;

//-----------------------------------
// DB
//-----------------------------------
WCHAR	CConfig::_szDBIP[16];
WCHAR	CConfig::_szDBAccount[64];
int		CConfig::_iDBPort;
WCHAR	CConfig::_szDBPassword[64];
WCHAR	CConfig::_szDBName[64];



CConfig* pConfig = CConfig::GetInstance();