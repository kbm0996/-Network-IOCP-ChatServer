CREATE DATABASE `accountdb`;



CREATE TABLE `accountdb`.`account` (
	`accountno` BIGINT NOT NULL AUTO_INCREMENT,
	`userid` CHAR(64) NOT NULL,
	`userpass` CHAR(64) NOT NULL,
	`usernick` CHAR(64) NOT NULL,	
	PRIMARY KEY (`accountno`),
	INDEX `userid_INDEX` (`userid` ASC)
);


 
CREATE TABLE `accountdb`.`sessionkey` (
	`accountno` BIGINT NOT NULL,
	`sessionkey` CHAR(64) NULL,
    PRIMARY KEY (`accountno`)
);




CREATE TABLE `accountdb`.`status` (
	`accountno` BIGINT NOT NULL,
	`status` INT NOT NULL DEFAULT 0,
	PRIMARY KEY (`accountno`)
);



CREATE TABLE `accountdb`.`whiteip` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`ip` CHAR(32) NOT NULL,
    PRIMARY KEY (`no`)
);





	
	
CREATE
    VIEW `accountdb`.`v_account` 
    AS
(
SELECT
  `a`.`accountno`  AS `accountno`,
  `a`.`userid`     AS `userid`,
  `a`.`usernick`   AS `usernick`,
  `b`.`sessionkey` AS `sessionkey`,
  `c`.`status`     AS `status`
FROM `accountdb`.`account` `a`
 LEFT JOIN `accountdb`.`sessionkey` `b` ON (`a`.`accountno` = `b`.`accountno`)
 LEFT JOIN `accountdb`.`status` `c` ON (`a`.`accountno` = `c`.`accountno`)
);










