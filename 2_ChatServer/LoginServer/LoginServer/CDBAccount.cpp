#include "CDBAccount.h"

CDBAccount::CDBAccount(WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort) : CDBConnector_TLS(szDBIP, szUser, szPassword, szDBName, iDBPort)
{
}

CDBAccount::~CDBAccount()
{
}

bool CDBAccount::ReadDB(BYTE byType, void * pIn, void * pOut)
{
	switch (byType)
	{
	case dfACCOUNT_STATUS_INIT:
		DBProc_STATUS_INIT();
		break;
	case dfACCOUNT_SESSION_CHACK:
		DBProc_SESSION_CHACK((st_SESSIONCHACK_IN*)pIn, (st_SESSIONCHACK_OUT*)pOut);
		break;
	case dfACCOUNT_WHITEIP_CHACK:
		break;
	default:
		return false;
	}
	return true;
}

bool CDBAccount::WriteDB(INT64 iAccountNo, bool bSuccessFlag)
{
	if (bSuccessFlag)
	{
		if (!Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='1' WHERE `accountno` = '%lld'", iAccountNo))
			return false;
		return true;
	}
	else
	{
		if (!Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='0' WHERE `accountno` = '%lld'", iAccountNo))
			return false;
		return true;
	}
}

void CDBAccount::DBProc_STATUS_INIT()
{
	Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='0'");
}

void CDBAccount::DBProc_SESSION_CHACK(st_SESSIONCHACK_IN * pIn, st_SESSIONCHACK_OUT * pOut)
{
	// AccountNo로 ID, Nickname 얻기
	MYSQL_ROW Row;

	// 뷰테이블 데이터얻기
	Query(L"SELECT * FROM `accountdb`.`v_account` WHERE `accountno` = '%lld'", pIn->AccountNo);
	Row = FetchRow();
	if (Row == nullptr)
	{
		FreeResult();
		LOG(L"DBAccount", LOG_ERROR, L"account table nonexistent [AccountNo : %lld]", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_ACCOUNT_MISS;
		return;
	}

	// 뷰에서 읽어온 것중 1개라도 null이라면 오류
	// 세션키는 아래에서 확인
	if (Row[1] == nullptr || Row[2] == nullptr || Row[4] == nullptr)
	{
		FreeResult();
		LOG(L"DBAccount", LOG_ERROR, L"Player table nonexistent [AccountNo : %lld]", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_STATUS_MISS;
		return;
	}

	ConvertC2WC(Row[1], pOut->ID, 20);
	ConvertC2WC(Row[2], pOut->Nickname, 20);

	// 세션키가 없거나 일치하지 않는다면
	if (pIn->AccountNo >= 1000000)
	{
		if (memcmp(Row[3], pIn->SessionKey, 64) != 0)
		{
			FreeResult();
			LOG(L"DBAccount", LOG_DEBUG, L"Player SessionKey Error [AccountNo : %lld]", pIn->AccountNo);
			pOut->byStatus = dfLOGIN_STATUS_FAIL;
			return;
		}
	}

	// 현재 Status 얻기
	int iStatus = atoi(Row[4]);
	FreeResult();
	// 플레이어가 게임중이라면
	if (iStatus != 0)
	{
		//LOG(L"DBAccount", LOG_DEBUG, L"Player In Game Playing [AccountNo : %lld]", pIn->AccountNo);
		pOut->byStatus = dfLOGIN_STATUS_GAME;
		return;
	}

	pOut->byStatus = dfLOGIN_STATUS_OK;
}
