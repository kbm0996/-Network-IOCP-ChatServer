/*---------------------------------------------------------------
  MySQL DB ���� Ŭ����

  �ܼ��ϰ� MySQL Connector �� ���� DB ���Ḹ �����Ѵ�.
 �����忡 �������� �����Ƿ� ���� �ؾ� ��.
 ���� �����忡�� ���ÿ� �̸� ����Ѵٸ� ������ ��.


- ����

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
		// CDBConnector_TLS Ŭ�����κ��� protected �Ҹ��ڿ� �����ϱ� ���� friend ����
		friend class CDBConnector_TLS;

		enum en_DB_CONNECTOR
		{
			eQUERY_MAX_LEN = 2048
		};

		CDBConnector(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
		virtual ~CDBConnector();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB ����
		//////////////////////////////////////////////////////////////////////
		bool		Connect();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB ����
		//////////////////////////////////////////////////////////////////////
		bool		Disconnect();

		//////////////////////////////////////////////////////////////////////
		// ���� ������ ����� �ӽ� ����
		//
		//////////////////////////////////////////////////////////////////////
		bool		Query(WCHAR *szStringFormat, ...);
		// DBWriter �������� Insert/Update ���� ���� ������� �������� ����.
		bool		Query_Insert(WCHAR *szStringFormat, ...);


		//////////////////////////////////////////////////////////////////////
		// ������ ���� �ڿ� ��� �̾ƿ���.
		//
		// ����� ���ٸ� NULL ����.
		//////////////////////////////////////////////////////////////////////
		MYSQL_ROW	FetchRow();

		//////////////////////////////////////////////////////////////////////
		// �� ������ ���� ��� ��� ��� �� ����.
		//////////////////////////////////////////////////////////////////////
		void		FreeResult();

		//////////////////////////////////////////////////////////////////////
		// Error ���.�� ������ ���� ��� ��� ��� �� ����.
		//////////////////////////////////////////////////////////////////////
		int			GetLastError() { return _iLastError; };
		WCHAR*		GetLastErrorMsg() { return _szLastErrorMsg; }

	private:
		//////////////////////////////////////////////////////////////////////
		// mysql �� LastError �� �ɹ������� �����Ѵ�.
		//////////////////////////////////////////////////////////////////////
		void		SaveLastError();


		//-------------------------------------------------------------
		// MySQL ���ᰴü ��ü
		//-------------------------------------------------------------
		MYSQL		_MySQL;

		//-------------------------------------------------------------
		// MySQL ���ᰴü ������. �� ������ ��������. 
		// �� �������� null ���η� ������� Ȯ��.
		//-------------------------------------------------------------
		MYSQL*		_pMySQL;

		//-------------------------------------------------------------
		// ������ ���� �� Result �����.
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
