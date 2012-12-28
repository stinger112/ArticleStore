<?php
require_once('./lib/function/addinlog.inc');

abstract class Model_Common
{
	private $db;
	protected $error; //Содержит строку с последней ошибкой (безопасной для прямого вывода пользователю). Решил не усложнять и не вводить систему ошибок через errno и таблицы обработки

	function __construct(mysqli $DBLink/*, $dbname */)
	{
		if ($DBLink->connect_errno)
			throw new mysqli_sql_exception($DBLink->connect_error);
			
		$this->db = $DBLink;
			
		$this->getDB()->query("SET sql_mode = 'STRICT_ALL_TABLES'"); //Запрещаем добавление неполных записей
			
		/* if (is_string($dbname))
			{
		$this->db->select_db($dbname) or $this->createDB();
		} */
	}

	###############################################Методы доступа к членам класса###############################################
	public function getDB()
	{
		return $this->db;
	}

	public function getError()
	{
		return $this->error;
	}
	############################################################################################################################

	###############################################Чисто виртуальные методы###############################################
	//abstract protected function createDB();
}
?>