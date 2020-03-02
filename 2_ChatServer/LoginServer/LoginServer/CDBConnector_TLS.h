/*---------------------------------------------------------------
  TLS MySQL DB 연결 클래스

  DB접근이 필요한 클래스마다 따로따로 CDBConnector를 만들기보다 사용하기 편하게 
 TLS를 이용한 DB 연결 클래스를 작성함
 NPacket처럼 Alloc, Free하는 것이 아니라 빈번하게 사용되기 때문에 한 번 Alloc하면 쭉 사용


- 사용법

CDBConnector_TLS* pDBConnector = new CDBConnector_TLS(_szDBIP, szDBAccount, szDBPassword, szDBName, iDBPort);

Query_Insert(L"UPDATE `accountdb`.`status` SET `status`='1' WHERE `accountno` = '%lld'", iAccountNo);

Query(L"SELECT * FROM `accountdb`.`v_account` WHERE `accountno` = '%lld'", pIn->AccountNo);
MYSQL_ROW Row = FetchRow();
if (Row == nullptr)
{
	FreeResult();
}
----------------------------------------------------------------*/

#ifndef __CDB_CONNECTOR_TLS__
#define __CDB_CONNECTOR_TLS__

#include "CDBConnector.h"
#include "CLFStack.h"

namespace mylib
{
	class CDBConnector_TLS
	{
	public:
		CDBConnector_TLS(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
		virtual ~CDBConnector_TLS();

	protected:
		//////////////////////////////////////////////////////////////////////
		// MySQL DB 연결
		//////////////////////////////////////////////////////////////////////
		bool		Connect();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 끊기
		//////////////////////////////////////////////////////////////////////
		bool		Disconnect();

	public:
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
		int			GetLastError();
		WCHAR*	 	GetLastErrorMsg();

	private:
		//////////////////////////////////////////////////////////////////////
		// DBConnector 얻기
		//////////////////////////////////////////////////////////////////////
		CDBConnector*		GetDBConnector();


		//-------------------------------------------------------------
		// TLS
		//-------------------------------------------------------------
		DWORD		_dwTlsIndex;

		//-------------------------------------------------------------
		// DBConnector 회수용 스택
		//-------------------------------------------------------------
		// OS는 프로세스에 할당 된 모든 메모리를 추적한다. 때문에 이 메모리들은 프로세스 종료시 해제된다.
		//그러나, DB의 세션은 해제되지 않으므로 파괴자에서 사용했던 모든 Session을 추적하여 해제시켜줘야 한다.
		CLFStack<CDBConnector*> _stkDBC;

		WCHAR		_szDBIP[16];
		WCHAR		_szDBUser[64];
		WCHAR		_szDBPassword[64];
		WCHAR		_szDBName[64];
		int			_iDBPort;
	};
}

#endif