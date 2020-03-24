# 서버 연동 예제
## 📢 개요
 IOCP(입출력 완료 포트; I/O completion port) 에코 서버-인증 서버 연동, 더미 클라이언트

## 📌 구성
### 서버 구조
  ![capture](https://user-images.githubusercontent.com/18212066/77258063-84262d00-6cbb-11ea-840f-dfbfbc23be70.png)
  
  **figure 1. Non-systemlink*
  
  ![capture](https://user-images.githubusercontent.com/18212066/77206138-a1cb8900-6b39-11ea-857a-052d534c01a5.jpg)
  
  **figure 2. Systemlink*
  
### DB 테이블

#### Account Table
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

```sql
CREATE TABLE `accountdb`.`sessionkey` (
	`accountno` BIGINT NOT NULL,
	`sessionkey` CHAR(64) NULL,
    PRIMARY KEY (`accountno`)
);
```

```sql
CREATE TABLE `accountdb`.`status` (
	`accountno` BIGINT NOT NULL,
	`status` INT NOT NULL DEFAULT 0,
	PRIMARY KEY (`accountno`)
);
```

```sql
CREATE TABLE `accountdb`.`whiteip` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`ip` CHAR(32) NOT NULL,
    PRIMARY KEY (`no`)
);
```

#### Server Monitoring Table

```sql
CREATE TABLE `status_server`.`hardware` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`AvailableMBytes` BIGINT NOT NULL,
	`NonPagedBytes` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);
```

```sql
CREATE TABLE `status_server`.`battle` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`AuthFPS` BIGINT NOT NULL,
	`GameFPS` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`SessionAuth` BIGINT NOT NULL,
	`SessionGame` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);
```

```sql
CREATE TABLE `status_server`.`chat` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`SessionLogin` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);
```

```sql
CREATE TABLE `status_server`.`login` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`LoginSuccessTPS` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
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
