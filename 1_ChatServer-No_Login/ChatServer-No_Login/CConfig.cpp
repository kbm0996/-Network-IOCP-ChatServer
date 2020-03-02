#include "CConfig.h"

using namespace mylib;

// :NETWORK
//-----------------------------------
// 본 서버군 이름 / ServerLink 정보의 이름과 같아야 함
//-----------------------------------
WCHAR	CConfig::_szServerName[20];

//-----------------------------------
// 채팅서버 Listen IP/PORT
//
// 이는 서버에서 클라이언트 접속용 Listen 소켓 Bind
//-----------------------------------
WCHAR	CConfig::_szBindIP[16];
int		CConfig::_iBindPort;

//-----------------------------------
// 로그인서버 연결 IP/PORT
//-----------------------------------
WCHAR	CConfig::_szLoginServerIP[16];
int		CConfig::_iLoginServerPort;
WCHAR	CConfig::_szLoginLanServerIP[16];
int		CConfig::_iLoginLanServerPort;

//-----------------------------------
// 모니터링 서버 연결 IP/PORT
//-----------------------------------
WCHAR	CConfig::_szMonitorServerIP[16];
int		CConfig::_iMonitorServerPort;

//-----------------------------------
// IOCP 워커스레드 개수
//-----------------------------------
int		CConfig::_iWorkerThreadNo;


//:SYSTEM
//-----------------------------------
// 최대사용자
//-----------------------------------
int		CConfig::_iClientMax;

//-----------------------------------
// Packet Encode Key
//-----------------------------------
int		CConfig::_byPacketCode;
int		CConfig::_byPacketKey1;
int		CConfig::_byPacketKey2;