<?php
#####################Блок поддержки#####################
require_once('./lib/controller/controller.inc');
require_once('./settings/MyLittlePony.inc');
header('Content-Type: text/html; charset=utf-8'); //Устанавливаем кодировку
########################################################

$foo = new Registration_Controller();
$foo->run();
?>