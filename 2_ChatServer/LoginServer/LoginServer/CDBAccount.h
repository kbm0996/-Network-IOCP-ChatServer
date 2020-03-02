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

	bool ReadDB(BYTE byType, void *pIn = nullptr, void *pOut = nullptr);	// ������ �бⰡ�ƴ� ����� ���� ���Ѵٸ� ���⵵ �����ϴ�.
	bool WriteDB(INT64 iAccountNo, bool bSuccessFlag);	// �б� �������� ������ �����

private:
	void DBProc_STATUS_INIT();
	void DBProc_SESSION_CHACK(st_SESSIONCHACK_IN *pIn, st_SESSIONCHACK_OUT *pOut);
};

#endif