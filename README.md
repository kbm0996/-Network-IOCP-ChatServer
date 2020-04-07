# 서버 연동 예제
## 📢 개요
 IOCP(입출력 완료 포트; I/O completion port) 에코 서버-인증 서버 연동, 더미 클라이언트

## 📌 구성
### 서버 통신
  ![capture](https://user-images.githubusercontent.com/18212066/77258063-84262d00-6cbb-11ea-840f-dfbfbc23be70.png)
  
  **figure 1. Non-systemlink*
  
  ![capture](https://user-images.githubusercontent.com/18212066/77206138-a1cb8900-6b39-11ea-857a-052d534c01a5.jpg)
  
  **figure 2. Systemlink*
  
### DB 정보
+ Account 테이블

각 계정의 고유번호, id, password, 닉네임

| accountno | userid | userpass | usernick |
|:---:|:---:|:---:|:---:|
|BIGINT|CHAR(64)|CHAR(64)|CHAR(64)|
```sql
CREATE TABLE `accountdb`.`account` (
	`accountno` BIGINT NOT NULL AUTO_INCREMENT,
	`userid` CHAR(64) NOT NULL,
	`userpass` CHAR(64) NOT NULL,
	`usernick` CHAR(64) NOT NULL,	
	PRIMARY KEY (`accountno`),
	INDEX `userid_INDEX` (`userid` ASC)
);
```

+ sessionkey 테이블

서버에서 클라이언트로부터의 로그인 요청을 받을때 정상적인 요청인지 검증하기 위해 보내는 세션키

| accountno | sessionkey |
|:---:|:---:|
|BIGINT|CHAR(64)|
```sql
CREATE TABLE `accountdb`.`sessionkey` (
	`accountno` BIGINT NOT NULL,
	`sessionkey` CHAR(64) NULL,
    PRIMARY KEY (`accountno`)
);
```

### 서버 화면

  ![capture](https://user-images.githubusercontent.com/18212066/77473272-8bcf0880-6e58-11ea-9391-2d0b9603e333.png)
  
  **figure 3. LoginServer(AuthServer)*
  
  ![capture](https://user-images.githubusercontent.com/18212066/77473276-8c679f00-6e58-11ea-93cd-cd927d06c6c2.png)
  
  **figure 4. GameServer(MainServer)*


## 📌 동작 원리

### 생성과 파괴

 IOCP는 비동기 입출력 결과와 이 결과를 처리할 스레드에 관한 정보를 담고 있는 구조
 
  ![capture](https://github.com/kbm0996/-Network-IOCP-EchoServerClient/blob/master/figure/3.png)
  
  **figure 1. CPP+DB(MySQL)*
