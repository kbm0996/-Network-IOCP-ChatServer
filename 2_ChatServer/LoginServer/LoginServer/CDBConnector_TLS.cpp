#include "CDBConnector_TLS.h"

mylib::CDBConnector_TLS::CDBConnector_TLS(WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort)
{
	_dwTlsIndex = TlsAlloc();
	if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"DBConnector_TLS() TLS_OUT_OF_INDEXES");
		// �����ڿ������� ���н� �� �� �ִ°� �����Ƿ� ���� ó��
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

	// �������ڸ� �״�� �ѱ�� ����� ã�� ����;
	// �̻������� Ǯ���ٰ� �ٽ� ����
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

	// �������ڸ� �������ڿ� �״�� �ѱ�� ����� ã�� ����;
	// �̻������� Ǯ���ٰ� �ٽ� ����
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
		// ����� �޸� ������ ����
		// OS�� ���μ����� �Ҵ� �� ��� �޸𸮸� �����Ѵ�. ������ �� �޸𸮵��� ���μ��� ����� �����ȴ�.
		//�׷���, DB�� ������ �������� �����Ƿ� �ı��ڿ��� ����ߴ� ��� Session�� �����Ͽ� ����������� �Ѵ�.
		_stkDBC.Push(pDBConnector);
		TlsSetValue(_dwTlsIndex, pDBConnector);

		//int iTryCnt = 0;
		//while (iTryCnt <= 5)
		while (1)
		{
			//++iTryCnt;

			// ���� ���д� ���� DBConnector ���� ������ ����
			if (pDBConnector->Connect())
				break;

			Sleep(1000);
		}
	}
	return pDBConnector;
}
