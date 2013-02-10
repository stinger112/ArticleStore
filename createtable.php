<?php
function createDB(mysqli $db)
{
	$sql = 'CREATE TABLE `user` (											'
		. ' `id`			INT UNSIGNED	NOT NULL 	AUTO_INCREMENT,		'
		. ' `login`			VARCHAR(32)		NOT NULL	UNIQUE,				'
		. ' `pass`			VARCHAR(32)		NOT NULL,						'
		. ' `hash`			VARCHAR(32)		NOT NULL	UNIQUE,				'
		. ' `regdate`		TIMESTAMP		DEFAULT CURRENT_TIMESTAMP, 		'
		. ' `lastlogin`		TIMESTAMP		DEFAULT 0,						'
		. ' PRIMARY KEY (`id`)												'
		. ' )																'
		. ' ENGINE = InnoDB CHARACTER SET utf8;								';
	if ($db->query($sql))
		echo "<p>Table 'user' created</p>";
	else
		echo "<p>" . $db->error . "</p>";

	$sql = 'CREATE TABLE `files` (														'
			. ' `id`			INT UNSIGNED	NOT NULL 	AUTO_INCREMENT,				'
			. '	`path`			VARCHAR(255)	NOT NULL	UNIQUE,						'
			. '	`user_id`		INT	UNSIGNED	NOT NULL,								'
			. ' `original_name`	VARCHAR(255),											'
			. '	`comment`		TEXT,													'
			. '	`date`			TIMESTAMP		NOT NULL	DEFAULT CURRENT_TIMESTAMP,	'
			. '	PRIMARY KEY (`id`),														'
			. ' FOREIGN KEY (`user_id`) REFERENCES user(`id`) ON DELETE CASCADE			'
			. '	)																		'
			. ' ENGINE = InnoDB CHARACTER SET utf8;										';
	if ($db->query($sql))
		echo "<p>Table 'files' created</p>";
	else
		echo "<p>" . $db->error . "</p>";

	//Добавить автоматическую группировку записей по добавлению (избавляет от нагрузок, связанных с группированием при запросах)
	//Данная таблица не содержит первичного ключа, т.к. может содержать абсолютно идентичные запросы
	$sql = 'CREATE TABLE `questions` (												'
			. ' `id`		INT UNSIGNED	NOT NULL 	AUTO_INCREMENT,				'
			. '	`question`	TINYTEXT		NOT NULL,								'
			. '	`position`	VARCHAR(32)		NOT NULL,								'
			. '	`case`		BOOLEAN			NOT NULL,								'
			. '	`user_id`	INT	UNSIGNED	NOT NULL,								'
			. '	`date`		TIMESTAMP		NOT NULL	DEFAULT CURRENT_TIMESTAMP,	'
			. ' PRIMARY KEY (`id`),													'
			. ' FOREIGN KEY (`user_id`) REFERENCES user(`id`) ON DELETE CASCADE		'
			. '	)																	'
			. ' ENGINE = InnoDB CHARACTER SET utf8;									';
	if($db->query($sql))
		echo "<p>Table 'questions' created</p>";
	else
		echo "<p>" . $db->error . "</p>";
	
	return TRUE;
}

require_once './settings/MyLittlePony.inc';
$db = new mysqli(MYSQL_SERVER, MYSQL_LOGIN, MYSQL_PASSWORD, MYSQL_NAME);

createDB($db);
?>