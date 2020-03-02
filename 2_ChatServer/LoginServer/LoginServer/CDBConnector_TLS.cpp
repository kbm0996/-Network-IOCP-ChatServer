#include "CDBConnector_TLS.h"

mylib::CDBConnector_TLS::CDBConnector_TLS(WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort)
{
	_dwTlsIndex = TlsAlloc();
	if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"DBConnector_TLS() TLS_OUT_OF_INDEXES");
		// 생성자에서부터 실패시 할 수 있는게 없으므로 종료 처리
		CRASH();
		exit(0);
	}
	memcpy(_szDBIP, szDBIP, sizeof(_szDBIP));
	memcpy(_szDBUser, szUser, sizeof(_szDBUser));
	memcpy(_szDBPassword, szPassword, sizeof(_szDBPassword));
	memcpy(_szDBName, szDBName, sizeof(_szDBName));
	_iDBPort = iDBPort;
}

mylib::CDBConnector_TLS::~CDBConnector_TLS()
{
	CDBConnector* pDBConnector;
	while (_stkDBC.Pop(pDBConnector))
		delete pDBConnector;
	TlsFree(_dwTlsIndex);
}

bool mylib::CDBConnector_TLS::Connect()
{
	CDBConnector* pDBConnector = GetDBConnector();
	return pDBConnector->Connect();
}

bool mylib::CDBConnector_TLS::Disconnect()
{
	CDBConnector* pDBConnector = GetDBConnector();
	return pDBConnector->Disconnect();
}

bool mylib::CDBConnector_TLS::Query(WCHAR * szStringFormat, ...)
{
	CDBConnector* pDBConnector = GetDBConnector();

	// 가변인자를 그대로 넘기는 방법을 찾지 못함;
	// 이상하지만 풀었다가 다시 삽입
	WCHAR szQuery[CDBConnector::eQUERY_MAX_LEN] = { 0, };
	va_list vaList;
	va_start(vaList, szStringFormat);
	HRESULT hResult = StringCchVPrintf(szQuery, sizeof(WCHAR) * CDBConnector::eQUERY_MAX_LEN, szStringFormat, vaList);
	va_end(vaList);
	if (FAILED(hResult))
	{
		szQuery[CDBConnector::eQUERY_MAX_LEN - 1] = '\0';
		LOG(L"SYSTEM", LOG_ERROR, L"DBConnector_TLS::Query_Insert() Too Long Query");
	}

	return pDBConnector->Query(szQuery);
}

bool mylib::CDBConnector_TLS::Query_Insert(WCHAR * szStringFormat, ...)
{
	CDBConnector* pDBConnector = GetDBConnector();

	// 가변인자를 가변인자에 그대로 넘기는 방법을 찾지 못함;
	// 이상하지만 풀었다가 다시 삽입
	WCHAR szQuery[CDBConnector::eQUERY_MAX_LEN] = { 0, };
	va_list vaList;
	va_start(vaList, szStringFormat);
	HRESULT hResult = StringCchVPrintf(szQuery, sizeof(WCHAR) * CDBConnector::eQUERY_MAX_LEN, szStringFormat, vaList);
	va_end(vaList);
	if (FAILED(hResult))
	{
		szQuery[CDBConnector::eQUERY_MAX_LEN - 1] = '\0';
		LOG(L"SYSTEM", LOG_ERROR, L"DBConnector_TLS::Query_Insert() Too Long Query");
	}

	return pDBConnector->Query_Insert(szQuery);
}

MYSQL_ROW mylib::CDBConnector_TLS::FetchRow()
{
	CDBConnector* pDBConnector = GetDBConnector();
	return pDBConnector->FetchRow();
}

void mylib::CDBConnector_TLS::FreeResult()
{
	CDBConnector* pDBConnector = GetDBConnector();
	pDBConnector->FreeResult();
}

int mylib::CDBConnector_TLS::GetLastError()
{
	CDBConnector* pDBConnector = GetDBConnector();
	return pDBConnector->GetLastError();;
}

WCHAR * mylib::CDBConnector_TLS::GetLastErrorMsg()
{
	CDBConnector* pDBConnector = GetDBConnector();
	return pDBConnector->GetLastErrorMsg();
}

mylib::CDBConnector * mylib::CDBConnector_TLS::GetDBConnector()
{
	CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		pDBConnector = new CDBConnector(_szDBIP, _szDBUser, _szDBPassword, _szDBName, _iDBPort);
		// 종료시 메모리 해제용 스택
		// OS는 프로세스에 할당 된 모든 메모리를 추적한다. 때문에 이 메모리들은 프로세스 종료시 해제된다.
		//그러나, DB의 세션은 해제되지 않으므로 파괴자에서 사용했던 모든 Session을 추적하여 해제시켜줘야 한다.
		_stkDBC.Push(pDBConnector);
		TlsSetValue(_dwTlsIndex, pDBConnector);

		//int iTryCnt = 0;
		//while (iTryCnt <= 5)
		while (1)
		{
			//++iTryCnt;

			// 연결 실패는 이후 DBConnector 사용시 에러로 뱉음
			if (pDBConnector->Connect())
				break;

			Sleep(1000);
		}
	}
	return pDBConnector;
}
