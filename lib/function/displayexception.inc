<?php
#########################Функция вывода сообщения на основе исключения#########################
function displayException(Exception $e, $details = NULL)
{
	if($details)
	{
		echo "<p><b>{$e->getMessage()}: </b>";
		echo "<pre>";
		print_r($e->getTrace());
		echo "</pre>";
		echo "</p>";
	}
	else
	{
		$trace = $e->getTrace();
		echo "<p><b>{$trace[0]['function']} ({$trace[0]['class']}): </b>";
		echo $e->getMessage() . " on line " . $e->getLine() . " in " . $e->getFile();
		echo "</p>";
	}
}
###############################################################################################
?>