#include "CallHttp.h"

int mylib::CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, char * OutRecvBuffer, int OutRecvBufferSize)
{
	int err = 0;

	// 1. TCP 소켓 생성
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}


	// 2. RecvTimeout, SendTimeout 설정(5초 ~10초)
	//	 블럭소켓이므로 서버가 반응이 없을경우를 대비
	timeval time = { 5, 0 }; // 5.0 second
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(int));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&time, sizeof(int));

	BOOL	bOptval = TRUE;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&bOptval, sizeof(bOptval));

	u_long uOptval = TRUE;
	ioctlsocket(s, FIONBIO, &uOptval);


	// 3. HTTP 프로토콜의 데이터 생성
	WCHAR szConnectIP[INET_ADDRSTRLEN];
	err = ConvertDomain2IP(szConnectIP, sizeof(szConnectIP), szDomainAddr);
	if (err != 0)
	{
		closesocket(s);
		return err;
	}

	char szHostIP[INET_ADDRSTRLEN];
	ConvertWC2C(szConnectIP, szHostIP, sizeof(szHostIP));

	char * szPath = ConvertWC2C(URL);

	char szData[1024] = { 0, };
	makeHttpMsg(iMethodType, szPath, szHostIP, szSendData, strlen(szSendData), szData, sizeof(szData));
	///printf("%s", szData);

	delete[] szPath;

	// 4. 웹 서버 Connect
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(80); // TCP 80포트 : http 연결에 사용, TCP 443 : https 연결에 사용
	InetPton(AF_INET, szConnectIP, &serveraddr.sin_addr);
	if (connect(s, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			closesocket(s);
			return err;
		}
	}


	// 5. 3번에서 만들어진 데이터를 send
	if (send(s, szData, strlen(szData), 0) == SOCKET_ERROR)
	{
		return WSAGetLastError();
	}

	// 6. send 후 바로 recv 호출
	// 접속이 끊어질 때까지 일정 길이만 받음. 받는 길이는 컨텐츠 상황에 맞도록 설정. (ex 1024)recv
	///char szRecvBuf[OutRecvBufferSize * 2];		// RecvData
	char * ptStrPos = NULL;		// RecvData Pos
	char szCheck[10];			// Completion Code & Body Length
	int iHeaderLength = 0;
	int iBodyLength = -1;
	DWORD dwTransferred = 0;

	///ZeroMemory(szRecvBuf, sizeof(szRecvBuf));
	ZeroMemory(szCheck, sizeof(szCheck));
	ZeroMemory(OutRecvBuffer, OutRecvBufferSize);
	while (1)
	{
		FD_SET ReadSet;
		FD_ZERO(&ReadSet);
		FD_SET(s, &ReadSet);
		select(0, &ReadSet, NULL, NULL, NULL);
		if (FD_ISSET(s, &ReadSet) > 0)
		{
			int iRecvLength = recv(s, OutRecvBuffer + dwTransferred, OutRecvBufferSize / 2 - dwTransferred, 0);
			if (iRecvLength <= 0)
				break;
			dwTransferred += iRecvLength;

			///printf("%s \n\n", OutRecvBuffer_Temp);

			/* HTTP_HEADER 분석 */
			if (iBodyLength == -1)
			{
				// 7. 데이터를 받은 후 HTTP 헤더에서 완료코드 얻기.
				//	받은 데이터에서 첫번째 0x20 코드를 찾아서 그 다음 0x20 까지가 완료 코드
				//	HTTP/1.1 200 OK     // 200이 완료코드
				ptStrPos = strchr(OutRecvBuffer, 0x20); // 0x20 : space
				if (ptStrPos != NULL)
				{
					// space
					ptStrPos += 1;
					///printf("%s\n%d\n\n", ptStrPos, err);

					int iValueLen = strchr(ptStrPos, 0x20) - ptStrPos;
					strncpy_s(szCheck, sizeof(szCheck), ptStrPos, iValueLen);
					err = atoi(szCheck);
					if (err != 200)
						break;

					// 완료코드 + space
					ptStrPos += iValueLen + 1;
					iValueLen = 0;
					///printf("%s\n%d\n\n", ptStrPos, err);

					// 8. 데이터를 받은 후 HTTP 헤더에서 Content-Length: 얻기.
					//받은 데이터에서 첫번째 Content_Length : 문자열을 찾아서, 그 다음 0x0d까지가 컨텐츠(BODY) 의 길이.
					// Content - Length : 159	// 이 숫자를 변환하여 BODY 길이 저장
					ptStrPos = strstr(OutRecvBuffer, "Content-Length:");
					if (ptStrPos != NULL)
					{
						// "Content-Length: "
						ptStrPos += 16;
						///printf("%s\n%d\n\n", ptStrPos, iBodyLength);

						iValueLen = strchr(ptStrPos, 0x0d) - ptStrPos;
						strncpy_s(szCheck, sizeof(szCheck), ptStrPos, iValueLen);
						iBodyLength = atoi(szCheck); // \r\n\r\n을 포함하지만 \0은 비포함한 길이

													 // Content-Length값 + "\r\n"
						ptStrPos += iValueLen + 2;
						iValueLen = 0;
						///printf("%s\n%d\n\n", ptStrPos, iBodyLength);

						// 9. 헤더의 끝(Body 시작) 확인
						// \r\n\r\n 을 찾아서 그 아래 부분만을 BODY 로 얻어냄.
						ptStrPos = strstr(OutRecvBuffer, "\r\n\r\n");
						if (ptStrPos != NULL)
						{
							ptStrPos += 7;
							///printf("%s\n%d :: %d\n\n", ptStrPos, iBodyLength, strlen(ptStrPos));
							iHeaderLength = ptStrPos - OutRecvBuffer;
						}
					}
				}
			}

			// 10. Body만 출력
			if (dwTransferred - iHeaderLength >= iBodyLength - 3)
			{
				strcpy_s(OutRecvBuffer, iBodyLength + 1, ptStrPos);
				break;
			}
		}

		///printf("%s\n\n", OutRecvData);
	}

	closesocket(s);

	return err;
}

int mylib::CallHttp(const WCHAR * szDomainAddr, const WCHAR * URL, int iMethodType, char * szSendData, WCHAR * OutRecvBuffer, int OutRecvBufferSize)
{
	int err = 0;

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	timeval time = { 5, 0 };
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(int));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&time, sizeof(int));

	BOOL	bOptval = TRUE;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&bOptval, sizeof(bOptval));

	u_long uOptval = TRUE;
	ioctlsocket(s, FIONBIO, &uOptval);

	WCHAR szConnectIP[INET_ADDRSTRLEN];
	err = ConvertDomain2IP(szConnectIP, sizeof(szConnectIP), szDomainAddr);
	if (err != 0)
	{
		closesocket(s);
		return err;
	}

	char szHostIP[INET_ADDRSTRLEN];
	ConvertWC2C(szConnectIP, szHostIP, sizeof(szHostIP));

	char * szPath = ConvertWC2C(URL);

	char szData[1024] = { 0, };
	makeHttpMsg(iMethodType, szPath, szHostIP, szSendData, strlen(szSendData), szData, sizeof(szData));

	delete[] szPath;

	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(80);
	InetPton(AF_INET, szConnectIP, &serveraddr.sin_addr);
	if (connect(s, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			closesocket(s);
			return err;
		}
	}


	if (send(s, szData, strlen(szData), 0) == SOCKET_ERROR)
	{
		return WSAGetLastError();
	}

	char * OutRecvBuffer_Temp = new char[OutRecvBufferSize / 2];
	char * ptStrPos = NULL;		// RecvData Pos
	char szCheck[10];			// Completion Code & Body Length
	int iHeaderLength = 0;
	int iBodyLength = -1;
	DWORD dwTransferred = 0;

	ZeroMemory(szCheck, sizeof(szCheck));
	ZeroMemory(OutRecvBuffer_Temp, OutRecvBufferSize / 2);
	while (1)
	{
		FD_SET ReadSet;
		FD_ZERO(&ReadSet);
		FD_SET(s, &ReadSet);
		select(0, &ReadSet, NULL, NULL, NULL);
		if (FD_ISSET(s, &ReadSet) > 0)
		{
			int iRecvLength = recv(s, OutRecvBuffer_Temp + dwTransferred, OutRecvBufferSize / 2 - dwTransferred, 0);
			if (iRecvLength <= 0)
				break;
			dwTransferred += iRecvLength;

			if (iBodyLength == -1)
			{

				ptStrPos = strchr(OutRecvBuffer_Temp, 0x20);
				if (ptStrPos != NULL)
				{
					ptStrPos += 1;

					int iValueLen = strchr(ptStrPos, 0x20) - ptStrPos;
					strncpy_s(szCheck, sizeof(szCheck), ptStrPos, iValueLen);
					err = atoi(szCheck);
					if (err != 200)
						break;

					ptStrPos += iValueLen + 1;
					iValueLen = 0;

					ptStrPos = strstr(OutRecvBuffer_Temp, "Content-Length:");
					if (ptStrPos != NULL)
					{
						ptStrPos += 16;


						iValueLen = strchr(ptStrPos, 0x0d) - ptStrPos;
						strncpy_s(szCheck, sizeof(szCheck), ptStrPos, iValueLen);
						iBodyLength = atoi(szCheck);

						ptStrPos += iValueLen + 2;
						iValueLen = 0;

						ptStrPos = strstr(OutRecvBuffer_Temp, "\r\n\r\n");
						if (ptStrPos != NULL)
						{
							ptStrPos += 7;
							iHeaderLength = ptStrPos - OutRecvBuffer_Temp;
						}
					}
				}
			}

			if (dwTransferred - iHeaderLength >= iBodyLength - 3)
			{
				strcpy_s(OutRecvBuffer_Temp, iBodyLength + 1, ptStrPos);
				ConvertC2WC(OutRecvBuffer_Temp, OutRecvBuffer, OutRecvBufferSize);
				break;
			}
		}
	}

	delete[] OutRecvBuffer_Temp;
	closesocket(s);

	return err;
}

void mylib::makeHttpMsg(int iMethodType, char * szRequestURL, char * szRequestHostIP, char * szSendContent, size_t iContentLen, char * pOutBuf, size_t iOutbufSize)
{
	if (iMethodType == POST)
	{
		////////////////////////////////////////////////////////////////
		//	POST %s HTTP/1.1\r\n		// RequestMethod, RequestURL, HTTPVersion
		//	User-Agent: Fiddler\r\n		// ClientSoftwareName&Version
		//	Content-Length: %d\r\n
		//	Host: %s\r\n				// Host to request
		//	Content-Type: application/x-www-form-urlencoded\r\n	// MessageBodyType(application/x-www-form-urlencoded, application/json, ...)
		//	Content-Length: %d\r\n
		//	\r\n
		//	(data)
		////////////////////////////////////////////////////////////////
		sprintf_s(pOutBuf, iOutbufSize, "%sPOST %s HTTP/1.1\r\n", pOutBuf, szRequestURL);
		sprintf_s(pOutBuf, iOutbufSize, "%sUser-Agent: Fiddler\r\n", pOutBuf);
		sprintf_s(pOutBuf, iOutbufSize, "%sContent-Length: %d\r\n", pOutBuf, iContentLen);
		sprintf_s(pOutBuf, iOutbufSize, "%sHost: %s\r\n", pOutBuf, szRequestHostIP);
		sprintf_s(pOutBuf, iOutbufSize, "%sContent-Type: application/x-www-form-urlencoded\r\n", pOutBuf);
		sprintf_s(pOutBuf, iOutbufSize, "%sConnection: close\r\n\r\n", pOutBuf);
		sprintf_s(pOutBuf, iOutbufSize, "%s%s", pOutBuf, szSendContent);
	}
	else
		sprintf_s(pOutBuf, iOutbufSize, "%sGET %s?%s HTTP/1.1\r\nHost: %s\r\n\r\n", pOutBuf, szRequestURL, szSendContent, szRequestHostIP);
}

int mylib::ConvertDomain2IP(WCHAR * _Destination, rsize_t _SizeInBytes, WCHAR const * _Source)
{
	int err = 0;

	// Translate Domain(WString) → IP(ADDRINFOW)
	// Return:		(int) 성공시 0 
	ADDRINFOW * pResult;
	if (GetAddrInfo(_Source, NULL, NULL, &pResult) != 0)
	{
		return WSAGetLastError();
	}

	// Translate IP(ADDRINFOW) → IP(WString)
	// Return:		(const WCHAR*) 성공시 IP 문자열 버퍼, 실패시 NULL
	if (InetNtop(AF_INET, &((SOCKADDR_IN *)pResult->ai_addr)->sin_addr, _Destination, min(_SizeInBytes, sizeof(WCHAR)*INET6_ADDRSTRLEN)) == NULL)
	{
		return WSAGetLastError();
	}

	FreeAddrInfo(pResult);

	return err;
}

char * mylib::ConvertWC2C(const WCHAR* inStr)
{
	int iStrLen = wcslen(inStr);
	char * pOutStr = new char[iStrLen + 1];
	ZeroMemory(pOutStr, sizeof(char) * iStrLen + 1);

	if (WideCharToMultiByte(CP_UTF8, 0, inStr, iStrLen, pOutStr, sizeof(char) * iStrLen + 1, NULL, NULL) == 0)
	{
		delete[] pOutStr;
		return nullptr;
	}
	return pOutStr;
}

int mylib::ConvertWC2C(const WCHAR * pInStr, char * pOutBuf, int iOutBufSize)
{
	ZeroMemory(pOutBuf, iOutBufSize);
	return WideCharToMultiByte(CP_UTF8, 0, pInStr, wcslen(pInStr), pOutBuf, iOutBufSize, NULL, NULL);
}

WCHAR * mylib::ConvertC2WC(const char * inStr)
{
	int iStrLen = strlen(inStr);
	WCHAR * pOutStr = new WCHAR[iStrLen + 1];
	ZeroMemory(pOutStr, sizeof(WCHAR) * iStrLen + 1);

	if (MultiByteToWideChar(CP_UTF8, 0, inStr, iStrLen, pOutStr, sizeof(WCHAR) * iStrLen + 1) == 0)
	{
		delete[] pOutStr;
		return nullptr;
	}
	return pOutStr;
}

int mylib::ConvertC2WC(const char * pInStr, WCHAR * pOutBuf, int iOutbufSize)
{
	return MultiByteToWideChar(CP_UTF8, 0, pInStr, strlen(pInStr), pOutBuf, iOutbufSize);
}

