/*------------------------------------------------------------------------------------------
PARSER Ŭ����

�Ľ��̶� ��� ������ ������ �о �ٸ� ���α׷��� ����� �� �ִ� �����ͷ� ��ȯ�ϴ� ���̴�.
�� Ŭ������ �Ʒ� ������ ���� �����Ϳ� ���� ������ ������ �о� Ư�� �׸�(Key)�� ��(Value)�� �� �� �ִ�.


- ����
1. ���� ������
:: TEST_ZONE
{
Version = 014
ServerID = 2
ServerBindIP = "192.168.11.29"
ServerBindPort = 50003
//ServerBindPort = 50001
WorkerThread = 33
MaxUser = 3003
}

2. �ڵ� ��뿹
int iVersion, iServerID, iServerBindPort, iWorkerThread, iMaxUser;
char szServerBindIP[256];

CParser g_Parser("test.ini");
if (LoadFile("Config.ini"))
{
if (g_Parser.SearchField("TEST_ZONE"))
{
g_Parser.GetValue("Version", &iVersion);
g_Parser.GetValue("ServerID", &iServerID);
g_Parser.GetValue("ServerBindIP", szServerBindIP, sizeof(szServerBindIP));
g_Parser.GetValue("ServerBindPort", &iServerBindPort);
g_Parser.GetValue("WorkerThread", &iWorkerThread);
g_Parser.GetValue("MaxUser", &iMaxUser);

printf("\nTEST_ZONE\n\n");
printf("*Version	= %d\n", iVersion);
printf("*ServerID	= %d\n", iServerID);
printf("*ServerBindIP	= \"%s\" \n", szServerBindIP);
printf("*ServerBindPort	= %d\n", iServerBindPort);
printf("*WorkerThread	= %d\n", iWorkerThread);
printf("*MaxUser	= %d\n", iMaxUser);
}
}
-------------------------------------------------------------------------------------------*/

#ifndef __PARSER_H__
#define __PARSER_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>

namespace mylib
{
	class CParser
	{
	public:
		enum en_PARSER
		{
			// ��ū ���� ũ��
			en_TOKEN_MAX_LEN = 256,

			// ��ŵ ��ū
			en_TOKEN_SPACE = 0x20,
			en_TOKEN_BACKSPACE = 0x08,
			en_TOKEN_TAB = 0x09,
			en_TOKEN_LF = 0x0a,	// ���� ���� '\n' Unix���� LF�� ����
			en_TOKEN_CR = 0x0d	// ���� ���� '\r' 
		};

		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CParser();
		CParser(const char* szFileName);	// ���ο��� LoadFile ȣ��
		virtual ~CParser();

		//////////////////////////////////////////////////////////////////////////
		// ���� �ҷ�����
		//  �Ľ��� ��� ������ �о �����Ѵ�.
		//
		// Parameters:	(char*) ���� �̸�
		// Return:		(bool) ���� ����
		//////////////////////////////////////////////////////////////////////////
		bool LoadFile(const char* szFileName);

		//////////////////////////////////////////////////////////////////////////
		// �ʵ� ã��
		//  ������(`::`)�� �������� �ʵ� ���� ����(`{`}�� ���� ����(`}`)�� ���Ѵ�.
		//
		// Parameters:	(char*) �ʵ� �̸�
		// Return:		(bool) ���� ����
		//////////////////////////////////////////////////////////////////////////
		bool SearchField(const char* szFieldName = nullptr);

		//////////////////////////////////////////////////////////////////////////
		// �� ã��
		//  ���� �ҷ��� ������ �Ľ��Ͽ� �Է��� Ű�� �����ϴ� ���� ã�Ƴ���. 
		// * SearchField()���� ���� �ʵ� ������ �Ľ��� �õ��Ѵ�.
		// * SearchField()�� ȣ������ �ʾҰų� �������� ���, ������ ó������ ������ �˻��Ͽ� �Ľ��Ѵ�.
		//
		// Parameters:	(char*) Ű
		//				_out1_ (int*) ���� ������ ����
		//				_out2_ (char*)	���� ������ ���ڿ� ����, (int) ���� ũ��
		//				_out3_ (WCHAR*) ���� ������ ���ڿ� ����, (int) ���� ũ��
		// Return:		(bool) ���� ����
		//////////////////////////////////////////////////////////////////////////
		bool GetValue(const char* szKey, int* pOutBuf);
		bool GetValue(const char* szKey, char* pOutBuf, int iOutBufSize);
		bool GetValue(const char* szKey, WCHAR* pOutBuf, int iOutBufSize);

	protected:
		//////////////////////////////////////////////////////////////////////////
		// ��(ު)��ɾ� ����
		//  �ּ�, ����, �����̽�, "" �� ��ū�� �����ϰ�, _iCurPos�� �����Ų��. 
		// * GetNextToken() �Լ� ���ο��� ȣ��
		//
		// Parameters:	
		// Return:		(bool) �� ���� ������ ������ true, ������ false
		//////////////////////////////////////////////////////////////////////////
		bool SkipNoneCommand();

		//////////////////////////////////////////////////////////////////////////
		// ��ū ���
		//  ���� ��ū�� ����ϰ�, ���� ��ū ���̸�ŭ _iCurPos�� �����Ų��.
		// * SearchField() �Լ� ���ο��� ȣ��
		// * GetValue() �Լ� ���ο��� ȣ��
		//
		// Parameters:	(char**) ����� ��ū�� ������ ���ڿ� ����
		//				(int*) ����� ��ū ����
		// Return:		(bool) ���� ����
		//////////////////////////////////////////////////////////////////////////
		bool GetNextToken(char** chppBuffer, int* pOutSize);

	private:
		int		_iFileSize;			// ���� ũ�� (EOF)
		char*	_pRawData;			// ���� ������

		int		_iFieldBeginPos;	// �ʵ� ���� ��ġ
		int		_iFieldEndPos;		// �ʵ� �� ��ġ
		int		_iCurPos;			// ���� �б� ��ġ
	};
}
#endif