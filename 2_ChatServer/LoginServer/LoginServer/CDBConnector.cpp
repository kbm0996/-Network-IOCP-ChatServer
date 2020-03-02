#include "CDBConnector.h"
#include "CallHttp.h"

mylib::CDBConnector::CDBConnector(WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort)
{
	ConvertWC2C(szDBIP, _szDBIP, sizeof(_szDBIP));
	ConvertWC2C(szUser, _szDBUser, sizeof(_szDBUser));
	ConvertWC2C(szPassword, _szDBPassword, sizeof(_szDBPassword));
	ConvertWC2C(szDBName, _szDBName, sizeof(_szDBName));
	_iDBPort = iDBPort;

	mysql_init(&_MySQL);
}

mylib::CDBConnector::~CDBConnector()
{
	Disconnect();
}

bool mylib::CDBConnector::Connect()
{
	_pMySQL = mysql_real_connect(&_MySQL, _szDBIP, _szDBUser, _szDBPassword, _szDBName, _iDBPort, (char*)NULL, 0);
	if (_pMySQL == nullptr)
	{	
		SaveLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"mysql_real_connect() : %d %s", _iLastError, _szLastErrorMsg);
		return false;
	}

	mysql_set_character_set(_pMySQL, "utf8");
	mysql_query(_pMySQL, "set session character_set_connection=euckr;");
	mysql_query(_pMySQL, "set session character_set_results=euckr;");
	mysql_query(_pMySQL, "set session character_set_client=euckr;");

	return true;
}

bool mylib::CDBConnector::Disconnect()
{
	mysql_close(_pMySQL);
	return true;
}

bool mylib::CDBConnector::Query(WCHAR * szStringFormat, ...)
{
	ZeroMemory(_szQuery, sizeof(WCHAR) * eQUERY_MAX_LEN);
	ZeroMemory(_szQueryUTF8, sizeof(char) * eQUERY_MAX_LEN);

	// 멤버에 쿼리문 저장
	// _szQuery (UTF16)
	va_list vaList;
	va_start(vaList, szStringFormat);
	HRESULT hResult = StringCchVPrintf(_szQuery, sizeof(WCHAR) * eQUERY_MAX_LEN, szStringFormat, vaList);
	va_end(vaList);
	if (FAILED(hResult))
		_szQuery[2047] = '\0';

	// _szQueryUTF8 (UTF8)
	if (ConvertWC2C(_szQuery, _szQueryUTF8, eQUERY_MAX_LEN) == 0)
		LOG(L"SYSTEM", LOG_ERROR, L"UTF16toUTF8()");

	// 재시도 횟수
	int iQueryCount = 5;
	while (iQueryCount > 0)
	{
		--iQueryCount;
		if (_pMySQL == nullptr)
		{
			Connect();
			continue;
		}
		int iResult = mysql_query(_pMySQL, _szQueryUTF8);
		if (iResult != 0)
		{
			// 연결과 관련된 에러
			//	CR_SOCKET_CREATE_ERROR
			//	CR_CONNECTION_ERROR
			//	CR_CONN_HOST_ERROR
			//	CR_SERVER_GONE_ERROR
			//	CR_SERVER_HANDSHAKE_ERR
			//	CR_SERVER_LOST
			//	CR_INVALID_CONN_HANDLE
			//
			//	위 에러들은 소켓, 연결 관련 에러로서 연결중 / 연결끊김 등의
			//	상황에 발생한다.몇몇 에러는 connect 시에 발생하는 에러도 있으나
			//	혹시 모르므로 연결관련 에러는 모두 체크 해보겠음
			//
			//	- 쿼리를 날림 : 에러발생
			//	- 연결 에러라면 재연결 시도
			//	- 연결 성공시 쿼리 날림
			//	- 연결 실패시 재연결 시도
			//	- 일정횟수 실패시 서버 종료

			SaveLastError();
			LOG(L"SYSTEM", LOG_ERROR, L"mysql_query() : %d %s (%s)", _iLastError, _szLastErrorMsg, _szQuery);

			if (_iLastError == CR_SOCKET_CREATE_ERROR || _iLastError == CR_CONNECTION_ERROR ||
				_iLastError == CR_CONN_HOST_ERROR || _iLastError == CR_SERVER_GONE_ERROR ||
				_iLastError == CR_SERVER_HANDSHAKE_ERR || _iLastError == CR_SERVER_LOST ||
				_iLastError == CR_INVALID_CONN_HANDLE)
			{
				Connect();
				continue;
			}

			return false;
		}

		_pSqlResult = mysql_store_result(_pMySQL);
		return true;
	}
	return false;
}

bool mylib::CDBConnector::Query_Insert(WCHAR * szStringFormat, ...)
{
	ZeroMemory(_szQuery, sizeof(WCHAR) * eQUERY_MAX_LEN);
	ZeroMemory(_szQueryUTF8, sizeof(char) * eQUERY_MAX_LEN);

	// 멤버에 쿼리문 저장
	// _szQuery (UTF16)
	va_list vaList;
	va_start(vaList, szStringFormat);
	HRESULT hResult = StringCchVPrintf(_szQuery, sizeof(WCHAR) * eQUERY_MAX_LEN, szStringFormat, vaList);
	va_end(vaList);
	if (FAILED(hResult))
		_szQuery[2047] = '\0';

	// _szQueryUTF8 (UTF8)
	if (ConvertWC2C(_szQuery, _szQueryUTF8, eQUERY_MAX_LEN) == 0)
		LOG(L"SYSTEM", LOG_ERROR, L"UTF16toUTF8()");

	// 재시도 횟수
	int iQueryCount = 5;
	while (iQueryCount > 0)
	{
		--iQueryCount;
		if (_pMySQL == nullptr)
		{
			Connect();
			continue;
		}
		int iResult = mysql_query(_pMySQL, _szQueryUTF8);
		if (iResult != 0)
		{
			// 연결과 관련된 에러
			//	CR_SOCKET_CREATE_ERROR
			//	CR_CONNECTION_ERROR
			//	CR_CONN_HOST_ERROR
			//	CR_SERVER_GONE_ERROR
			//	CR_SERVER_HANDSHAKE_ERR
			//	CR_SERVER_LOST
			//	CR_INVALID_CONN_HANDLE
			//
			//	위 에러들은 소켓, 연결 관련 에러로서 연결중 / 연결끊김 등의
			//	상황에 발생한다.몇몇 에러는 connect 시에 발생하는 에러도 있으나
			//	혹시 모르므로 연결관련 에러는 모두 체크 해보겠음
			//
			//	- 쿼리를 날림 : 에러발생
			//	- 연결 에러라면 재연결 시도
			//	- 연결 성공시 쿼리 날림
			//	- 연결 실패시 재연결 시도
			//	- 일정횟수 실패시 서버 종료

			SaveLastError();
			LOG(L"SYSTEM", LOG_ERROR, L"mysql_query() : %d %s", _iLastError, _szLastErrorMsg);

			if (_iLastError == CR_SOCKET_CREATE_ERROR || _iLastError == CR_CONNECTION_ERROR ||
				_iLastError == CR_CONN_HOST_ERROR || _iLastError == CR_SERVER_GONE_ERROR ||
				_iLastError == CR_SERVER_HANDSHAKE_ERR || _iLastError == CR_SERVER_LOST ||
				_iLastError == CR_INVALID_CONN_HANDLE)
			{
				Connect();
				continue;
			}

			return false;
		}

		_pSqlResult = mysql_store_result(_pMySQL);
		FreeResult();
		return true;
	}
	return false;
}

MYSQL_ROW mylib::CDBConnector::FetchRow()
{
	return mysql_fetch_row(_pSqlResult);
}

void mylib::CDBConnector::FreeResult()
{
	mysql_free_result(_pSqlResult);
}

void mylib::CDBConnector::SaveLastError()
{
	_iLastError = mysql_errno(&_MySQL);
	ConvertC2WC(mysql_error(&_MySQL), _szLastErrorMsg, sizeof(_szLastErrorMsg));
}
