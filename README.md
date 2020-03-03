# 네트워크 프로그래밍 IOCP
## 📢 개요
 IOCP(입출력 완료 포트; I/O completion port) 에코 서버-인증 서버 연동, 더미 클라이언트

## 📌 동작 원리

### 생성과 파괴

 IOCP는 비동기 입출력 결과와 이 결과를 처리할 스레드에 관한 정보를 담고 있는 구조
 
  ![capture](https://github.com/kbm0996/-Network-IOCP-EchoServerClient/blob/master/figure/3.png)
  

## 📌 입출력 완료 포트 생성 및 연결

 CreateCompletionPort() 함수는 두 가지 역할을 한다. 하나는 입출력 완료 포트를 새로 생성하는 일이고, 또 하나는 소켓과 입출력 완료 포트를 연결하는 일이다. 소켓과 입출력 완료 포트를 연결해두면 이 소켓에 대한 비동기 입출력 결과가 입출력 완료 포트에 저장된다.

```cpp
HANDLE CreateIoCompletionPort(
     HANDLE FileHandle,        // IOCP와 연결할 파일 핸들이나 소켓. INVALID_HANDLE_VALUE값 전달시 신규 생성
     HANDLE ExistingCompletionPort, // 연결할 IOCP 핸들, NULL이면 새 IOCP 생성
     ULONG_PTR CompletionKey,  // 입출력 완료 패킷(비동기 입출력 작업 완료시 생성되어 IOCP에 저장됨) 부가 정보
     DWORD NumberOfConcurrentThreads // 동시에 실행할 수 있는 스레드 수, 0 입력시 CPU 수만큼 스레드 수를 맞춤
); // 성공 : 입출력 완료 포트 핸들, 실패 : NULL
```
※ 참고로 _PTR로 끝나는 변수는 포인터 변수가 아니다. 포인터를 담을 수 있는 변수라는 의미

### 입출력 완료 포트를 새로 생성하는 코드 예제

```cpp
// Global 
HANDLE g_hIOCP; 

// Main
int main(int argc, char* argv[])
{
     g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); 
     if(g_hIOCP == NULL) return 1;
   
     ...
   
     return 0;
}
```
※ 마지막 인자는 스레드를 몇개 생성하는지 설정하는 것인데 1을 넣는다고 스레드를 1개만 사용하는 것이 아니다. 이미 스레드가 Sleep() 등의 이유로 실행 중이면 1개 더 깨우기 때문이다. 따라서, 사용자가 원하는대로 항상 1개의 스레드만이 작동하는 것이 아니므로 0을 입력하여 CPU 개수만큼의 스레드를 사용하는 것이 보편적이다.

### 기존 소켓과 입출력 완료 포트를 연결하는 코드 예제

```cpp
// Global 
HANDLE g_hIOCP; 
SOCKET g_Sock;
// Main
int main(int argc, char* argv[])
{
     HANDLE hResult;
     g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); 
     if(g_hIOCP == NULL) return 1;
   
     ...

     hResult = CreateIoCompletionPort((HANDLE)g_Sock, g_hIOCP, (DWORD)g_Sock, 0);
     if(hResult == NULL) return 1;

     ...
   
     return 0;
}
```
