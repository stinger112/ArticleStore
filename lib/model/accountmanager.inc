<?php
require_once './lib/model/model.inc';

class AccountManager extends Model_Common//Инкапсулирует логику регистрации и авторизации. Не проверяет входные данные на предмет содержания команд MySQL, HTML-тегов и прочего
{
	private $arUser; //$arUser['login']/['pass']/['another_data'] = ЛОГИН/ПАРОЛЬ/любые_другие_данные
	private $access;

	function __construct(mysqli $DBLink, array $arInput = array())
	{
		parent::__construct($DBLink);
			
		$this->setAccount($arInput); //Заполнение из входного массива/кук
	}

	###############################################Методы доступа к членам класса###############################################
	public function setAccount(array $array) //Устанавливает логин и пароль. Инициирует переменную доступа.
	{
		//Аккаут устанавливается в любом случае, а переменная $access заполняется в зависимости от результата запроса
		//Метод включает в себя автоматический запрос к БД для сверки, воизбежание многократных запросов в течении выполнения одного скрипта
		$this->arUser = array(); //Удаляем данные старого аккаунта
		$this->access = FALSE;
			
		if ($array['login'] && $array['pass']) //Установка аккаунта из входного массива
		{
			if (is_string($array['login']) && is_string($array['pass'])) //Чертовски не хватает строгой типизации
			{
				$sql = 'SELECT * '
					. ' FROM `users` '
					. ' WHERE `login` = \'' . $array['login'] . '\' AND `pass` = \'' . $array['pass'] . '\'';
					
				$this->arUser = array("login" => $array['login'], "pass" => $array['pass']); //Заполняем аккаунт из входного массива (если sql-запрос будет успешен, то аккаунт перезапишется и добавится $access = TRUE)
			}
		}
		elseif($_SESSION['ID'] or $_COOKIE['ID']) //Попытка установки аккаунта из куков/сессии
		{
			$hash = $_SESSION['ID'] or $hash = $_COOKIE['ID'];
			$hash = md5($hash); //Защита от команд злоумышленника в куках/сессиях
			$sql = 'SELECT * '
				. ' FROM `users` '
				. '	WHERE `hash` = \'' . $hash . '\'';
		}
		else
			return FALSE;
			
		if ($qResult = $this->getDB()->query($sql))
			if ($arResult = $qResult->fetch_assoc())
			{
				$this->arUser = $arResult; //Заполняем аккаунт из БД, так мы получим в arUser ВСЮ информацию
				$this->access = TRUE; //Устанавливаем переменную доступа
			}

			return TRUE;
	}

	public function getLogin()
	{
		return $this->arUser['login'];
	}

	public function getPassword()
	{
		return $this->arUser['pass'];
	}

	public function getUserID()
	{
		return $this->arUser['id'];
	}

	public function getFullInformation()
	{
		$arTemp = $this->arUser;
		$arTemp['access'] = $this->access;
		return $arTemp;
	}

	public function checkAccess()
	{
		return $this->access;
	}
	############################################################################################################################

	public function createAccount() //Cоздает аккаунт из внутренних данных
	{
		if (is_string($this->getLogin()) && is_string($this->getPassword()))
		{
			$login = $this->getLogin();
			$pass = $this->getPassword();
			$sql = 'INSERT INTO users(login, pass, hash)'
					. "	VALUES ('$login', '$pass', '" . session_id() . "')"; //Заполняем уникальным значением SID (нужно для БД, но в дальнейшем перезаписывается)
				
			if ($this->getDB()->query($sql))
			{
				addInLog("Registration successful", $login);
				return $this;
			}
			else
			{
				return FALSE;
			}
		}
	}

	public function clearAccount() //Удаляет текущую сессию, куки и очищает данные аккаунта
	{
		if ($this->access)
		{
			if ($_SESSION['ID']) //Удаляем текущую сессию и куки
			{
				unset($_SESSION['ID']);
				if ($_COOKIE['ID'])
					setcookie('ID', "", time()-3600);
			}

			addInLog("Exit", $this->arUser['login']); //Запись в лог добавляется только у пользователей с доступом
		}
			
		unset($this->arUser);
		$this->access = FALSE;
	}

	public function setSession($cookie = FALSE) //Устанавливает сессию и куки для текущего аккаунта (только в случае, если не установлены)
	{
		if (!$_SESSION['ID'])
		{
			if ($_COOKIE['ID'])
			{
				$_SESSION['ID'] = $_COOKIE['ID'];
				return TRUE;
			}
			elseif ($this->access)
			{
				$hash = md5(session_id()); //Формируем случайный хеш
				$_SESSION['ID'] = $hash;

				$sql = 'UPDATE `users` '
						. " SET `hash` = '" . md5($hash) . "', `lastlogin` = NOW()"
						. " WHERE `id` = '" . $this->getUserID() . "'";
					
				if ($cookie)
					if (!setcookie('ID', $hash, time()+3600))
						return FALSE;

				if (!$this->getDB()->query($sql))
					return FALSE;
					
				addInLog("Authorization", $this->arUser['login']);
				return TRUE;
			}
		}
			
		return FALSE;
	}
}
?>