<?php
require_once './lib/view/view.inc';
require_once './lib/model/accountmanager.inc';
require_once './lib/model/textmanager.inc';

class Controller
{
	private $db;
	private $View;
	private $account; //Содержит объект модели текущего аккаунта
	private $arFilteredData; //Массив предобработанных входных данных

	function __construct()
	{
		$this->db = new mysqli(MYSQL_SERVER, MYSQL_LOGIN, MYSQL_PASSWORD, MYSQL_NAME);
		$this->getDB()->query("set names 'utf8'"); //Устанавливаем рабочую кодировку
			
			
		$this->View = new View;
			
		$this->arFilteredData = $this->filterInput($_REQUEST) or $this->arFilteredData = array(); //Нужно для того, что бы даже если забыли в filterInput() вернуть массив, переменная была массивом!

		$this->account = new AccountManager($this->db, $this->arFilteredData);
	}

	###############################################Методы доступа к членам класса###############################################
	public function getView()
	{
		return $this->View;
	}

	public function getAccount()
	{
		return $this->account;
	}

	public function getDB()
	{
		return $this->db;
	}

	public function getFilteredData()
	{
		return $this->arFilteredData;
	}
	############################################################################################################################

	public function exitAccount()
	{
		$this->getAccount()->clearAccount();
	}

	###############################################Виртуальные методы (обязательны для замещения в дочерних классах)###############################################
	protected function filterInput(array $arInput) //Фильтрует входной массив данных в соответствии с задачей (здесь возвращает array['login']/['pass'])
	{
		//Есть аналогичная функция filter_input, но решил не приспосабливать (хотя можно попытаться разобраться с callable-типом)
		//Куда попадает содержимое полей:
	}

	public function run() //Выполнение контроллера. (здесь выполняет проверку на авторизованность, отрисовывает main page)
	{
		if (!$this->getAccount()->checkAccess())
		{
			return $this->getView()->displayPage('./forms/authform.inc');
		}
		$this->getAccount()->setSession();

		$this->getView()->displayMain();
	}
	###############################################################################################################################################################
}

//Можно и без нижележащего контроллера, но другие реализации мне показались архитектурно неправильными.
//Восстанавливает сессию любой контроллер, но ставит куки только авторизационный!
class Authorization_Controller extends Controller //Контроллер авторизации (пока выполняет все функции базового контроллера, но отрисовывает ошибку)
{
	function __construct()
	{
		parent::__construct();
	}

	protected function filterInput(array $arInput)
	{
		//['login'] => SQL
		//['password'] => SQL
		if (!substr_count($arInput['login'], "'") <= 32 && !substr_count($arInput['pass'], "'"))
		{
			$login = $this->getDB()->real_escape_string($arInput['login']);
			$pass = $this->getDB()->real_escape_string($arInput['pass']);
			return array("login" => $login, "pass" => $pass, "mem" => $arInput['mem']);
		}
	}

	public function run()
	{
		if (!$this->getAccount()->checkAccess())
		{
			$this->getView()->addNotification("Неверный логин или пароль!");
			return $this->getView()->displayPage('./forms/authform.inc');
		}

		$this->getAccount()->setSession($_POST['mem']); //Сразу из POST, потому что является простым флагом, и не попадает никуда.
			
		$this->getView()->displayMain();
	}
}

class Registration_Controller extends Controller
{
	function __construct()
	{
		parent::__construct();
	}

	protected function filterInput(array $arInput) //Проверка массива $_POST (ограничение на 32 символа установлено в БД)
	{
		//Куда попадает содержимое полей
		//['login'] => HTML, SQL, mkdir()
		//['password'] => SQL
			
		$patternLog = '/^[-_.\w]*[a-zA-Z]+[-_.\w]*$/'; //Возможна замена $ на \z
		$patternPass = '/^[-_.,?;:"}{`~!@#$%^&*)(+=\w]*$/'; //Без верхней кавычки ('), Она сломает SQL запрос.
			
		if ($arInput['login'] && $arInput['pass'] && $arInput['pass2'])
		{
			if (preg_match($patternLog, $arInput['login']) && preg_match($patternPass, $arInput['pass']))
			{
				if (strlen($arInput['login']) <= 32 && strlen($arInput['pass']) <= 32)
				{
					if (substr_compare($arInput['pass'], $arInput['pass2'], 0) === 0)
					{
						return array("login" => $arInput['login'], "pass" => $arInput['pass']);
					}
					else
						$this->getView()->addNotification("Пароль и подтверждение не совпадают!");
				}
				else
					$this->getView()->addNotification("Слишком длинное значение! Максимальная длина - 32 символа.");
			}
			else
				$this->getView()->addNotification("Логин или пароль содержат недопустимые символы!");
		}
		else
			$this->getView()->addNotification("* отмечены обязательные поля");
	}

	public function run()
	{
		$createResult = $this->getAccount()->createAccount();
		if ($createResult === FALSE)
		{
			$this->getView()->addNotification("Такой пользователь уже существует!");
			$this->getView()->displayPage('./forms/regform.inc');
		}
		elseif ($createResult === NULL)
		$this->getView()->displayPage('./forms/regform.inc');
		else
		{
			$this->getView()->displayPage('./forms/regsuccess.html');
		}
	}
}

define("MAX_TEXT_SIZE", 20000);
define("MAX_COMMENT_SIZE", 1000);

class Upload_Controller extends Controller
{
	function __construct()
	{
		parent::__construct();
	}

	protected function filterInput(array $arInput) //Возвращает array['author']/['comment']/['text'] (author берется из текущего аккаунта)
	{
		//Куда попадает содержимое полей
		//['text'] => HTML
		//['comment'] => HTML, SQL
		if (strlen($arInput['comment']) <= MAX_COMMENT_SIZE)
		{
			if (!substr_count($arInput['comment'], "'")) //Защищаем SQL запрос
			{
				#############################################Блок добавления текста#############################################
				if ($_FILES['file']['tmp_name'] != "none" && $_FILES['file']['size'] <= MAX_TEXT_SIZE && substr_count($_FILES['file']['type'], "text"))
				{
					$tmp_text = file_get_contents($_FILES['file']['tmp_name']);
					$arOut['text'] = strip_tags($tmp_text);
				}
				elseif ($arInput['text'] && strlen($arInput['text']) <= MAX_TEXT_SIZE)
				{
					$arOut['text'] = strip_tags($arInput['text']);
				}
				else
				{
					$this->getView()->addNotification("Текст не был добавлен! Max " . MAX_TEXT_SIZE . " symbols.");
					return;
				}
				################################################################################################################
					
				$arOut['comment'] = strip_tags($arInput['comment']);
				$arOut['comment'] = $this->getDB()->real_escape_string($arOut['comment']);
					
				return $arOut;
			}
			else
				$this->getView()->addNotification("Комментарий содержит недопустимые символы!");
		}
		else
		{
			$this->getView()->addNotification("Слишком длинный комментарий! Max " . MAX_COMMENT_SIZE . " symbols.");
		}
	}

	public function run()
	{
		if (!$this->getAccount()->checkAccess())
		{
			$this->getView()->clearNotification();
			return $this->getView()->displayPage('./forms/authform.inc');
		}
		$this->getAccount()->setSession();
			
		if ($arTextData = $this->getFilteredData())
		{
			$arTextData['id'] = $this->getAccount()->getUserID(); //Добавляем id пользователя
			$arTextData['author'] = $this->getAccount()->getLogin(); //Дополняем автором (пояснения внутри соотв. метода)
			$newText = new TextFile($this->getDB(), $arTextData);
			if ($newText->getError())
			{
				$this->getView()->addNotification($newText->getError()); //Добавляем последнюю ошибку
			}
			else
				$this->getView()->addNotification("Текст успешно добавлен");

		}
			
		$this->getView()->displayMain();
	}
}

class Search_Controller extends Controller
{
	function __construct()
	{
		parent::__construct();
	}

	protected function filterInput(array $arInput)
	{
		return $arInput;
	}

	public function run()
	{
		if (!$this->getAccount()->checkAccess())
		{
			$this->getView()->clearNotification();
			return $this->getView()->displayPage('./forms/authform.inc');
		}
		$this->getAccount()->setSession();
			
			
		$this->getView()->displayMain();
	}
}
?>