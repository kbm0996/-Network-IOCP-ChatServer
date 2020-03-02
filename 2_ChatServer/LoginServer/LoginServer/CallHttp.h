/*---------------------------------------------------------------
HTTP ������ ���� �� ����

CPU ����(����� CPU �ð�)�� üũ�ϴ� Ŭ����.
`�������� - ���� �����`�� ������. `�۾� ������ - ����`���� ���� ������ ���̰� ��

- ����

// �� �������� �䱸�ϴ� ������ ���� ��(RapidJSON Style) : "{ \"id\":\"12d3\", \"pass\":\"1234\", \"nickname\":\"bsds\" }"
err = CallHttp(L"127.0.0.1", L"http://127.0.0.1:80/auth_login.php", GET, "id='���̵�'&pass='���'", outData, sizeof(outData));
err = CallHttp(L"127.0.0.1", L"http://127.0.0.1:80/Register.php", POST, "{\"id\": \"gmf\",\"password\" : \"����н���2d��\"}", outData, sizeof(outData));
----------------------------------------------------------------*/
#ifndef __CALL_HTTP_H__
#define __CALL_HTTP_H__

#include "NetworkLib.h"

// CallHttp Function - MethodType
#define POST	0
#define GET		1

namespace mylib
{
	//////////////////////////////////////////////////////////////////////////
	// Send Data to WebServer + Recv Data to WebServer
	//
	// Parameters:	(WCHAR*) ������ �ּ�
	//				(WCHAR*) ��û�� URL
	//				(int) �޼ҵ�(POST or GET)
	//				(char*) ���� ������(Message Body)	
	//				(char*) _out_ �����͸� ���� ����	
	//				(int) �����͸� ���� ���� ũ��
	// Return:		(int) ������ 0, ���н� ���� ����
	//////////////////////////////////////////////////////////////////////////
	int		CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, char * OutRecvBuffer, int OutRecvBufferSize = 1024);
	int		CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, WCHAR * OutRecvBuffer, int OutRecvBufferSize = 1024);

	//////////////////////////////////////////////////////////////////////////
	// Make `HTTP Request Message Format`
	//
	// Parameters:	(int) �޼ҵ�(POST or GET)
	//				(char*) ��û�� URL
	//				(char*) ��û�� ȣ��Ʈ
	//				(char*) ���� ���� ����		
	//				(int) ���� ���� ���� ũ��
	//				(char*) _out_ �޼��� ����	
	//				(int) �޼��� ���� ũ��
	// Return:
	//////////////////////////////////////////////////////////////////////////
	void	makeHttpMsg(int iMethod, char * szRequestURL, char * szRequestHostIP, char * szSendContent, size_t iContentLen, char * pOutBuf, size_t iOutbufSize);


	//////////////////////////////////////////////////////////////////////////
	// Translate Domain(WString) �� IP(WString)
	//
	// Parameters:	(WCHAR*) _out_ IP		
	//				(int) IP(WSTring) ����
	//				(WCHAR const*) ������
	// Return:		(int) ������ 0, ���н� ���� ����
	//////////////////////////////////////////////////////////////////////////
	int ConvertDomain2IP(WCHAR* _Destination, rsize_t _SizeInBytes, WCHAR const* _Source);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF16(wchar) -> UTF8(char)
	// :: ���ο��� ���ڿ� ���� �Ҵ�(new)
	//
	// Parameters:	(const WCHAR*) ��ȯ�� ���ڿ�
	// Return:		(char*) ��ȯ�� ���ڿ�
	//////////////////////////////////////////////////////////////////////////
	char *	ConvertWC2C(const WCHAR * inStr);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF16(wchar) -> UTF8(char)
	//
	// Parameters:	(const WCHAR*) ��ȯ�� ���ڿ�
	//				(int) ��ȯ�� ���ڿ� ����
	//				(char*) ��ȯ�� ���ڿ��� ������ ����
	//				(int) ��ȯ�� ���ڿ��� ������ ���� ũ��
	// Return:		(int) ��ȯ�� ���ڿ��� ����
	//////////////////////////////////////////////////////////////////////////
	int	ConvertWC2C(const WCHAR * pInStr, char * pOutBuf, int iOutbufSize);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF8(char) -> UTF16(wchar)
	// :: ���ο��� ���ڿ� ���� �Ҵ�(new)
	//
	// Parameters:	(const char*) ��ȯ�� ���ڿ�
	// Return:		(WCHAR*) ��ȯ�� ���ڿ�
	//////////////////////////////////////////////////////////////////////////
	WCHAR *	ConvertC2WC(const char * inStr);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF8(char) -> UTF16(wchar)
	//
	// Parameters:	(const WCHAR*) ��ȯ�� ���ڿ�
	//				(int) ��ȯ�� ���ڿ� ����
	//				(char*) ��ȯ�� ���ڿ��� ������ ����
	//				(int) ��ȯ�� ���ڿ��� ������ ���� ũ��
	// Return:		(int) ��ȯ�� ���ڿ��� ����
	//////////////////////////////////////////////////////////////////////////
	int	ConvertC2WC(const char * pInStr, WCHAR * pOutBuf, int iOutbufSize);
}
#endif