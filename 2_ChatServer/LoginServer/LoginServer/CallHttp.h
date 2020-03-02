/*---------------------------------------------------------------
HTTP 데이터 전송 및 수신

CPU 사용률(사용한 CPU 시간)을 체크하는 클래스.
`관리도구 - 성능 모니터`와 유사함. `작업 관리자 - 성능`과는 왠진 모르지만 차이가 남

- 사용법

// 웹 서버에서 요구하는 데이터 형식 예(RapidJSON Style) : "{ \"id\":\"12d3\", \"pass\":\"1234\", \"nickname\":\"bsds\" }"
err = CallHttp(L"127.0.0.1", L"http://127.0.0.1:80/auth_login.php", GET, "id='아이디'&pass='비밀'", outData, sizeof(outData));
err = CallHttp(L"127.0.0.1", L"http://127.0.0.1:80/Register.php", POST, "{\"id\": \"gmf\",\"password\" : \"사용패스워2d드\"}", outData, sizeof(outData));
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
	// Parameters:	(WCHAR*) 도메인 주소
	//				(WCHAR*) 요청할 URL
	//				(int) 메소드(POST or GET)
	//				(char*) 보낼 데이터(Message Body)	
	//				(char*) _out_ 데이터를 받을 버퍼	
	//				(int) 데이터를 받을 버퍼 크기
	// Return:		(int) 성공시 0, 실패시 소켓 에러
	//////////////////////////////////////////////////////////////////////////
	int		CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, char * OutRecvBuffer, int OutRecvBufferSize = 1024);
	int		CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, WCHAR * OutRecvBuffer, int OutRecvBufferSize = 1024);

	//////////////////////////////////////////////////////////////////////////
	// Make `HTTP Request Message Format`
	//
	// Parameters:	(int) 메소드(POST or GET)
	//				(char*) 요청할 URL
	//				(char*) 요청할 호스트
	//				(char*) 보낼 내용 버퍼		
	//				(int) 보낼 내용 버퍼 크기
	//				(char*) _out_ 메세지 버퍼	
	//				(int) 메세지 버퍼 크기
	// Return:
	//////////////////////////////////////////////////////////////////////////
	void	makeHttpMsg(int iMethod, char * szRequestURL, char * szRequestHostIP, char * szSendContent, size_t iContentLen, char * pOutBuf, size_t iOutbufSize);


	//////////////////////////////////////////////////////////////////////////
	// Translate Domain(WString) → IP(WString)
	//
	// Parameters:	(WCHAR*) _out_ IP		
	//				(int) IP(WSTring) 길이
	//				(WCHAR const*) 도메인
	// Return:		(int) 성공시 0, 실패시 소켓 에러
	//////////////////////////////////////////////////////////////////////////
	int ConvertDomain2IP(WCHAR* _Destination, rsize_t _SizeInBytes, WCHAR const* _Source);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF16(wchar) -> UTF8(char)
	// :: 내부에서 문자열 동적 할당(new)
	//
	// Parameters:	(const WCHAR*) 변환할 문자열
	// Return:		(char*) 변환된 문자열
	//////////////////////////////////////////////////////////////////////////
	char *	ConvertWC2C(const WCHAR * inStr);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF16(wchar) -> UTF8(char)
	//
	// Parameters:	(const WCHAR*) 변환할 문자열
	//				(int) 변환할 문자열 길이
	//				(char*) 변환된 문자열을 저장할 버퍼
	//				(int) 변환된 문자열을 저장할 버퍼 크기
	// Return:		(int) 변환된 문자열의 길이
	//////////////////////////////////////////////////////////////////////////
	int	ConvertWC2C(const WCHAR * pInStr, char * pOutBuf, int iOutbufSize);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF8(char) -> UTF16(wchar)
	// :: 내부에서 문자열 동적 할당(new)
	//
	// Parameters:	(const char*) 변환할 문자열
	// Return:		(WCHAR*) 변환된 문자열
	//////////////////////////////////////////////////////////////////////////
	WCHAR *	ConvertC2WC(const char * inStr);


	//////////////////////////////////////////////////////////////////////////
	// Translate UTF8(char) -> UTF16(wchar)
	//
	// Parameters:	(const WCHAR*) 변환할 문자열
	//				(int) 변환할 문자열 길이
	//				(char*) 변환된 문자열을 저장할 버퍼
	//				(int) 변환된 문자열을 저장할 버퍼 크기
	// Return:		(int) 변환된 문자열의 길이
	//////////////////////////////////////////////////////////////////////////
	int	ConvertC2WC(const char * pInStr, WCHAR * pOutBuf, int iOutbufSize);
}
#endif