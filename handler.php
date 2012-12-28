<?php
#####################Блок поддержки#####################
require_once './lib/controller/controller.inc';
require_once './settings/MyLittlePony.inc';
header('Content-Type: text/html; charset=utf-8'); //Устанавливаем кодировку
########################################################

if (isset($_GET['auth']))
{
	$currentController = new Authorization_Controller();
}
elseif (isset($_GET['upload']))
{
	$currentController = new Upload_Controller();
}
elseif (isset($_GET['query']))
{
	$currentController = new Search_Controller();
}
else
{
	$currentController = new Controller;
}

if (isset($_GET['exit']))
	$currentController->exitAccount();

$currentController->run();
?>