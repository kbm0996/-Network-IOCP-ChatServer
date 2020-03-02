/*---------------------------------------------------------------
  TLS MySQL DB ���� Ŭ����

  DB������ �ʿ��� Ŭ�������� ���ε��� CDBConnector�� ����⺸�� ����ϱ� ���ϰ� 
 TLS�� �̿��� DB ���� Ŭ������ �ۼ���
 NPacketó�� Alloc, Free�ϴ� ���� �ƴ϶� ����ϰ� ���Ǳ� ������ �� �� Alloc�ϸ� �� ���


- ����

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
		// MySQL DB ����
		//////////////////////////////////////////////////////////////////////
		bool		Connect();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB ����
		//////////////////////////////////////////////////////////////////////
		bool		Disconnect();

	public:
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
		int			GetLastError();
		WCHAR*	 	GetLastErrorMsg();

	private:
		//////////////////////////////////////////////////////////////////////
		// DBConnector ���
		//////////////////////////////////////////////////////////////////////
		CDBConnector*		GetDBConnector();


		//-------------------------------------------------------------
		// TLS
		//-------------------------------------------------------------
		DWORD		_dwTlsIndex;

		//-------------------------------------------------------------
		// DBConnector ȸ���� ����
		//-------------------------------------------------------------
		// OS�� ���μ����� �Ҵ� �� ��� �޸𸮸� �����Ѵ�. ������ �� �޸𸮵��� ���μ��� ����� �����ȴ�.
		//�׷���, DB�� ������ �������� �����Ƿ� �ı��ڿ��� ����ߴ� ��� Session�� �����Ͽ� ����������� �Ѵ�.
		CLFStack<CDBConnector*> _stkDBC;

		WCHAR		_szDBIP[16];
		WCHAR		_szDBUser[64];
		WCHAR		_szDBPassword[64];
		WCHAR		_szDBName[64];
		int			_iDBPort;
	};
}

#endif