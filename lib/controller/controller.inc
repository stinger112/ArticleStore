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
		
		if (!is_dir('./tmp')) //Создаем папку tmp (если не было). В ней будут временно хранится тексты.
		{
			if (!mkdir('./tmp'))
				throw new RuntimeException("Can't create ./tmp directory");
			file_put_contents('./tmp/.htaccess', "Deny from all"); //Обновляем настройки апача на папку tmp
		}
	}
	
	/* private function clearTmp() //Очищаем папку tmp от текстов
	{
		$dir = './tmp';
		
		if (is_dir($dir))
		{
			if ($dh = opendir($dir))
			{
				while (($file = readdir($dh)) !== FALSE)
				{
					if (!substr_count($file, '.'))
						unlink($dir . '/' . $file);
				}
				closedir($dh);
			}
		}
	} */

	protected function filterInput(array $arInput) //Возвращает array['name']/['original_name'](опционально)/['comment'](опционально), где name - имя файла в папке tmp (уже безопасное).
	{
		//Метод не работает с папкой text, так как это противоречит концепции MVC (файлами управляет модель)
		//Куда попадает содержимое полей
		//['comment'] => HTML, SQL
		//['name'] => HTML, SQL
		if (strlen($arInput['comment']) <= MAX_COMMENT_SIZE)
		{
			if (!substr_count($arInput['comment'], "'")) //Защищаем SQL запрос
			{
				#############################################Блок добавления текста#############################################
				if ($_FILES['file']['tmp_name'] && $_FILES['file']['size'] <= MAX_TEXT_SIZE && substr_count($_FILES['file']['type'], "text"))
				{
					$arOut['name'] = hash('md5', mt_rand());
					move_uploaded_file($_FILES['file']['tmp_name'], './tmp/' . $arOut['name']);
					
					$arOut['original_name'] = $this->getDB()->real_escape_string($_FILES['file']['name']); //Добавляем оригинальное имя файла. Проверка его вывода уже непосредственно на странице.
				}	
				elseif ($arInput['text'] && strlen($arInput['text']) <= MAX_TEXT_SIZE)
				{
					$arOut['name'] = hash('md5', mt_rand());
					file_put_contents('./tmp/' . $arOut['name'], $arInput['text']);
				}
				else
				{
					$this->getView()->addNotification("Текст не был добавлен! Max " . MAX_TEXT_SIZE . " symbols.");
					return;
				}
				################################################################################################################
				
				if ($arInput['comment'])
				{
					$arOut['comment'] = htmlspecialchars($arInput['comment'], ENT_NOQUOTES|ENT_HTML5);
					$arOut['comment'] = $this->getDB()->real_escape_string($arOut['comment']);
				}
					
				return $arOut;
			}
			else
				$this->getView()->addNotification("Комментарий содержит недопустимые символы!");
		}
		else
			$this->getView()->addNotification("Слишком длинный комментарий! Max " . MAX_COMMENT_SIZE . " symbols.");
	}

	public function run()
	{
		if (!$this->getAccount()->checkAccess())
		{
			$this->getView()->clearNotification();
			return $this->getView()->displayPage('./forms/authform.inc');
		}
		$this->getAccount()->setSession();

		if ($arTextData = $this->getFilteredData()) //Если массив $this->getFilteredData() пуст, то не создаем объект TextFile
		{
			$newText = new TextFile($this->getDB(), $arTextData, $this->getAccount());
			if ($newText->getError())
				$this->getView()->addNotification($newText->getError()); //Добавляем последнюю ошибку
			else
				$this->getView()->addNotification("Текст успешно добавлен");
		}
		
		//$this->clearTmp(); //На всякий случай чистим папку tmp
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
		if ($arInput['query'])
		{
			if (strlen($arInput['query']) < 256)
			{
				$arOut['question'] = mb_convert_encoding($arInput['query'], mb_detect_encoding($arInput['query']));
				$arOut['question'] = trim($arOut['question']);
				$arOut['question'] = $this->getDB()->real_escape_string($arOut['question']);
				
				$arOut['case'] = $arInput['case'] ? '1' : '0';
				
				if ('first' === $arInput['position'] || 'nfnl' === $arInput['position'] || 'last' === $arInput['position'])
					$arOut['position'] = $arInput['position'];
				else
					$arOut['position'] = 'any';
				
				if ('phrase' === $arInput['view'] || 'paragraph' === $arInput['view'])
					$arOut['view'] = $arInput['view'];
				else
					$arOut['view'] = 'sentence';
				
				$arOut['nostat'] = isset($arInput['nostat']); //Флаг статистики
				$arOut['start'] = (int)$arInput['start']; //Стартовая позиция пагинации
				
				return $arOut;
			}
			else
				$this->getView()->addNotification("Слишком длинный запрос");
		}
		else 
			$this->getView()->addNotification("Введите поисковый запрос");
	}

	public function run()
	{
		if (!$this->getAccount()->checkAccess())
		{
			$this->getView()->clearNotification();
			return $this->getView()->displayPage('./forms/authform.inc');
		}
		$this->getAccount()->setSession();
		
		if ($arQuestionData = $this->getFilteredData())
		{
			/* Завершаем формирование $arQuestionData */
			$start = $arQuestionData['start'];
			unset($arQuestionData['start']);
			
			/* Генерируем новый запрос и проводим его */
			$question = new Question($this->getDB(), $arQuestionData);
			
			if ($question->query($this->getAccount())) //Проводим запрос
				$this->getView()->addQuestion($question, new Pagination($start));
			else
				$this->getView()->addNotification("По вашему запросу ничего не найдено");
		}
		
		$this->getView()->displayMain();
	}
}

class Detail_Controller extends Controller
{
	function __construct()
	{
		parent::__construct();
	}
	
	protected function filterInput(array $arInput)
	{
		if (preg_match('/[0-9a-z]{32}/', $arInput['name']))
		{
			$arOut['name'] = $arInput['name'];
			return $arOut;
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
		
		$arDetailRequest = $this->getFilteredData();
		$text = new TextFile($this->getDB(), $arDetailRequest['name']);
		
		if ($text->getFileInfo())
		{
			$this->getView()->displayText($text);
		}
		else //Отправляем заголовки, загружаем страницу с 404 ошибкой
		{
			header("HTTP/1.0 404 Not Found");
			header("HTTP/1.1 404 Not Found");
			header("Status: 404 Not Found");
			$this->getView()->displayPage('./errors/404.html');
		}
	}
}
?>