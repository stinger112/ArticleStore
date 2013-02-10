<?php

//require_once './lib/model/textmanager.inc';
$foo = array('a' => array(1,2), 'b' => array(3,4), 'c' => array(5,6));

$foo = array_values($foo);
var_dump($foo);

/* echo "\$_SESSION:\n";
var_dump($_SESSION);

echo "ID hash:\n";
var_dump(md5($_SESSION['ID']));

echo "\$_COOKIE:\n";
var_dump($_COOKIE);

//var_dump(__FILE__);

//	phpinfo();
//	require_once './settings/MyLittlePony0.inc';
//	$db = new mysqli(MYSQL_SERVER, MYSQL_LOGIN, MYSQL_PASSWORD, MYSQL_NAME);
//	var_dump($db->character_set_name());
/*$sql = 'SELECT * '
 . ' FROM register.user '
. '	WHERE `hash` = \'' . md5('80bdf8feb8a9b0280278a69a1b256d29') . '\'';
$qResult = $db->query($sql);
$arRow = $qResult->fetch_assoc();
print_r($arRow);*/
?>