#ifndef __DB_ACCOUNT__
#define __DB_ACCOUNT__

#include "NetworkLib.h"
#include "CDBConnector_TLS.h"
#include "CallHttp.h"

using namespace mylib;

class CDBAccount : public CDBConnector_TLS
{
public:
	enum en_AccountType
	{
		dfACCOUNT_STATUS_INIT = 0,
		dfACCOUNT_SESSION_CHACK,
		dfACCOUNT_WHITEIP_CHACK,
	};
	struct st_SESSIONCHACK_IN
	{
		INT64	AccountNo;
		char	*SessionKey;
	};

	struct st_SESSIONCHACK_OUT
	{
		WCHAR	*ID;
		WCHAR	*Nickname;
		BYTE	byStatus;
	};

	CDBAccount(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
	virtual ~CDBAccount();

	bool ReadDB(BYTE byType, void *pIn = nullptr, void *pOut = nullptr);	// 무조건 읽기가아닌 동기로 쓰기 원한다면 쓰기도 가능하다.
	bool WriteDB(INT64 iAccountNo, bool bSuccessFlag);	// 읽기 전혀없이 무조건 저장용

private:
	void DBProc_STATUS_INIT();
	void DBProc_SESSION_CHACK(st_SESSIONCHACK_IN *pIn, st_SESSIONCHACK_OUT *pOut);
};

#endif