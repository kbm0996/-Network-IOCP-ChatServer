/*---------------------------------------------------------------
  MySQL DB 연결 클래스

  단순하게 MySQL Connector 를 통한 DB 연결만 관리한다.
 스레드에 안전하지 않으므로 주의 해야 함.
 여러 스레드에서 동시에 이를 사용한다면 개판이 됨.


- 사용법

CDBConnector* pDBConnector = new CDBConnector(_szDBIP, szDBAccount, szDBPassword, szDBName, iDBPort);

Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='1' WHERE `accountno` = '%lld'", iAccountNo);

Query(L"SELECT * FROM `accountdb`.`v_account` WHERE `accountno` = '%lld'", pIn->AccountNo);
MYSQL_ROW Row = FetchRow();
if (Row == nullptr)
{
	FreeResult();
}
----------------------------------------------------------------*/

#ifndef __CDB_CONNECTOR__
#define __CDB_CONNECTOR__

#include "NetworkLib.h"
#pragma comment(lib,"mysql/lib/vs14/mysqlclient.lib")
#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"


namespace mylib
{
	class CDBConnector
	{
	protected:
		// CDBConnector_TLS 클래스로부터 protected 소멸자에 접근하기 위한 friend 선언
		friend class CDBConnector_TLS;

		enum en_DB_CONNECTOR
		{
			eQUERY_MAX_LEN = 2048
		};

		CDBConnector(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
		virtual ~CDBConnector();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 연결
		//////////////////////////////////////////////////////////////////////
		bool		Connect();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 끊기
		//////////////////////////////////////////////////////////////////////
		bool		Disconnect();

		//////////////////////////////////////////////////////////////////////
		// 쿼리 날리고 결과셋 임시 보관
		//
		//////////////////////////////////////////////////////////////////////
		bool		Query(WCHAR *szStringFormat, ...);
		// DBWriter 스레드의 Insert/Update 쿼리 전용 결과셋을 저장하지 않음.
		bool		Query_Insert(WCHAR *szStringFormat, ...);


		//////////////////////////////////////////////////////////////////////
		// 쿼리를 날린 뒤에 결과 뽑아오기.
		//
		// 결과가 없다면 NULL 리턴.
		//////////////////////////////////////////////////////////////////////
		MYSQL_ROW	FetchRow();

		//////////////////////////////////////////////////////////////////////
		// 한 쿼리에 대한 결과 모두 사용 후 정리.
		//////////////////////////////////////////////////////////////////////
		void		FreeResult();

		//////////////////////////////////////////////////////////////////////
		// Error 얻기.한 쿼리에 대한 결과 모두 사용 후 정리.
		//////////////////////////////////////////////////////////////////////
		int			GetLastError() { return _iLastError; };
		WCHAR*		GetLastErrorMsg() { return _szLastErrorMsg; }

	private:
		//////////////////////////////////////////////////////////////////////
		// mysql 의 LastError 를 맴버변수로 저장한다.
		//////////////////////////////////////////////////////////////////////
		void		SaveLastError();


		//-------------------------------------------------------------
		// MySQL 연결객체 본체
		//-------------------------------------------------------------
		MYSQL		_MySQL;

		//-------------------------------------------------------------
		// MySQL 연결객체 포인터. 위 변수의 포인터임. 
		// 이 포인터의 null 여부로 연결상태 확인.
		//-------------------------------------------------------------
		MYSQL*		_pMySQL;

		//-------------------------------------------------------------
		// 쿼리를 날린 뒤 Result 저장소.
		//-------------------------------------------------------------
		MYSQL_RES*	_pSqlResult;

		char		_szDBIP[16];
		char		_szDBUser[64];
		char		_szDBPassword[64];
		char		_szDBName[64];
		int			_iDBPort;


		WCHAR		_szQuery[eQUERY_MAX_LEN];
		char		_szQueryUTF8[eQUERY_MAX_LEN];

		int			_iLastError;
		WCHAR		_szLastErrorMsg[128];
	};
}
#endif
