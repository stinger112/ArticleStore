<?php
#########################Функция добавления сообщения в лог#########################
function addInLog($message, $filename)
{
	if (is_string($message) && is_string($filename)) //Все беды из-за отсутствия строгой типизации
	{
		$path = "./logs/" . $filename . ".log";
		if (file_put_contents($path, date("o-m-d H:i:s") . " - " . $message . "\r\n", FILE_APPEND/*|FILE_USE_INCLUDE_PATH*/))
			return TRUE; //Error in file
	}
}
####################################################################################
?>