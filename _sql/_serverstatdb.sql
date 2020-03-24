


--dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // 배틀서버 ON
--dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // 배틀서버 CPU 사용률 (커널 + 유저)
--dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
--dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // 배틀서버 패킷풀 사용량
--dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // 배틀서버 Auth 스레드 초당 루프 수
--dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // 배틀서버 Game 스레드 초당 루프 수
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // 배틀서버 접속 세션전체
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // 배틀서버 Auth 스레드 모드 인원
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // 배틀서버 Game 스레드 모드 인원
--dfMONITOR_DATA_TYPE_BATTLE_,		                        // 배틀서버 로그인을 성공한 전체 인원		<- 삭제
--dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // 배틀서버 대기방 수
--dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // 배틀서버 플레이방 수

CREATE DATABASE `status_server`;

CREATE TABLE `status_server`.`hardware` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`AvailableMBytes` BIGINT NOT NULL,
	`NonPagedBytes` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);

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

--INSERT INTO `status_server`.`battle` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `AuthFPS`, `GameFPS`, `SessionAll`, `SessionAuth`, `SessionGame`) VALUES (NOW(), '1', '1', '1', '1', '1', '1', '1', '1');

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

--INSERT INTO `status_server`.`chat` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `SessionLogin`) VALUES (NOW(), '1', '1', '1', '1', '1');

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

--INSERT INTO `status_server`.`login` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `LoginSuccessTPS`) VALUES (NOW(), '1', '1', '1', '1', '1');





