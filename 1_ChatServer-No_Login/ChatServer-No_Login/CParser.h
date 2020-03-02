/*------------------------------------------------------------------------------------------
PARSER 클래스

파싱이란 어떠한 형태의 파일을 읽어서 다른 프로그램이 사용할 수 있는 데이터로 변환하는 것이다.
이 클래스는 아래 예시의 원본 데이터와 같은 형식의 파일을 읽어 특정 항목(Key)의 값(Value)을 얻어낼 수 있다.


- 사용법
1. 원본 데이터
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

2. 코드 사용예
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
			// 토큰 버퍼 크기
			en_TOKEN_MAX_LEN = 256,

			// 스킵 토큰
			en_TOKEN_SPACE = 0x20,
			en_TOKEN_BACKSPACE = 0x08,
			en_TOKEN_TAB = 0x09,
			en_TOKEN_LF = 0x0a,	// 개행 문자 '\n' Unix에선 LF만 개행
			en_TOKEN_CR = 0x0d	// 개행 문자 '\r' 
		};

		//////////////////////////////////////////////////////////////////////////
		// 생성자, 파괴자.
		//
		// Parameters:
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CParser();
		CParser(const char* szFileName);	// 내부에서 LoadFile 호출
		virtual ~CParser();

		//////////////////////////////////////////////////////////////////////////
		// 파일 불러오기
		//  파싱할 대상 파일을 읽어서 저장한다.
		//
		// Parameters:	(char*) 파일 이름
		// Return:		(bool) 성공 여부
		//////////////////////////////////////////////////////////////////////////
		bool LoadFile(const char* szFileName);

		//////////////////////////////////////////////////////////////////////////
		// 필드 찾기
		//  구분자(`::`)를 기준으로 필드 시작 지점(`{`}과 종료 지점(`}`)을 정한다.
		//
		// Parameters:	(char*) 필드 이름
		// Return:		(bool) 성공 여부
		//////////////////////////////////////////////////////////////////////////
		bool SearchField(const char* szFieldName = nullptr);

		//////////////////////////////////////////////////////////////////////////
		// 값 찾기
		//  먼저 불러온 파일을 파싱하여 입력한 키와 대응하는 값을 찾아낸다. 
		// * SearchField()에서 정한 필드 내에서 파싱을 시도한다.
		// * SearchField()를 호출하지 않았거나 실패했을 경우, 파일의 처음부터 끝까지 검색하여 파싱한다.
		//
		// Parameters:	(char*) 키
		//				_out1_ (int*) 값을 저장할 버퍼
		//				_out2_ (char*)	값을 저장할 문자열 버퍼, (int) 버퍼 크기
		//				_out3_ (WCHAR*) 값을 저장할 문자열 버퍼, (int) 버퍼 크기
		// Return:		(bool) 성공 여부
		//////////////////////////////////////////////////////////////////////////
		bool GetValue(const char* szKey, int* pOutBuf);
		bool GetValue(const char* szKey, char* pOutBuf, int iOutBufSize);
		bool GetValue(const char* szKey, WCHAR* pOutBuf, int iOutBufSize);

	protected:
		//////////////////////////////////////////////////////////////////////////
		// 비(非)명령어 생략
		//  주석, 개행, 스페이스, "" 등 토큰을 생략하고, _iCurPos를 진행시킨다. 
		// * GetNextToken() 함수 내부에서 호출
		//
		// Parameters:	
		// Return:		(bool) 더 읽을 내용이 있으면 true, 없으면 false
		//////////////////////////////////////////////////////////////////////////
		bool SkipNoneCommand();

		//////////////////////////////////////////////////////////////////////////
		// 토큰 얻기
		//  다음 토큰을 출력하고, 읽은 토큰 길이만큼 _iCurPos를 진행시킨다.
		// * SearchField() 함수 내부에서 호출
		// * GetValue() 함수 내부에서 호출
		//
		// Parameters:	(char**) 출력한 토큰을 저장할 문자열 버퍼
		//				(int*) 출력할 토큰 길이
		// Return:		(bool) 성공 여부
		//////////////////////////////////////////////////////////////////////////
		bool GetNextToken(char** chppBuffer, int* pOutSize);

	private:
		int		_iFileSize;			// 파일 크기 (EOF)
		char*	_pRawData;			// 원본 데이터

		int		_iFieldBeginPos;	// 필드 시작 위치
		int		_iFieldEndPos;		// 필드 끝 위치
		int		_iCurPos;			// 현재 읽기 위치
	};
}
#endif