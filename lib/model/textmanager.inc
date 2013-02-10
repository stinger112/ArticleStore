<?php
require_once './lib/function/addinlog.inc';
require_once './lib/model/model.inc';

class TextFile extends Model_Common //Инкапсулирует структуру файла и методы работы с ним
{
	private $arFileInfo; //Содержит информацию о файле согласно структуре БД
	private $arResult; //Содержит результаты последнего поиска
	
	function __construct(mysqli $DBLink, $data, AccountManager $account = NULL)
	{
		parent::__construct($DBLink);
			
		if (!is_dir('./text'))
			if (!mkdir('./text'))
				throw new RuntimeException("Can't create ./text directory");
			
		if (is_array($data))
		{
			$this->addText($data, $account);
		}
		elseif (is_string($data))
		{
			$this->associateWithText($data);
		}
		else
			$this->error = "Внутренняя ошибка."; //Получен неверный тип аргумента
	}
	
	###############################################Методы доступа к членам класса###############################################
	private function associateWithText($path) //Ассоциирует объект TextFile с определенным файлом (согласно БД)
	{
		if (is_string($path) && is_file($path))
		{
			$sql = 'SELECT * '
				. '	FROM `files` '
				. '	WHERE `path` = \'' . $path . '\'';
			if ($qRes = $this->getDB()->query($sql))
			{
				if ($arRes = $qRes->fetch_assoc())
				{
					$this->arFileInfo = $arRes;
					return TRUE;
				}
				else
				{
					$this->error = "Файл не найден";
					return FALSE;
				}
			}
		}
	}

	public function getText() //Получить текст в виде строки
	{
		$path = $this->arFileInfo['path'];
		if ($text = file_get_contents($path)) //Игнорируется разбиение по абзацам?
			return $text;
	}

	public function getFileInfo()
	{
		return $this->arFileInfo;
	}

	public function getResult() //Возвращение параграфа пока не работает
	{
		return $this->arResult;
	}
	############################################################################################################################
	static public function getFilePathsList ()
	{
		$arFiles = array(); //В случае отсутствия результатов вернется пустой массив
		
		$sql = 'SELECT path '
			. '	FROM `files` ';
		$arFilePath = $db->query($sql)->fetch_all(MYSQLI_ASSOC); //Получили массив путей
		
		foreach ($arFilePath as $row) //Делаем массив объектов (ключ массива - путь к файлу)
		{
			$tmpFile = new TextFile($db, $row['path']);
		
			if ($tmpFile->search($SearchStr, $Position, $Case, $View)) //Выполняем поиск в текущем файле
				$arFiles[] = $tmpFile;
		}
		
		return $arFiles;
	}
	
	public function search($SearchStr, $Position, $Case, $mode) //Поиск по текущему текстовому файлу. Заполняет собой массив $arResult
	{
		if (is_string($SearchStr))
		{
			//Чистим массивы от предыдущих результатов, что бы они не попали в новый поиск
			unset($this->arResult);
			
			if ($Case)
				$Case = '';
			else
				$Case = 'i';
			
			//Дополнительное формирование поискового запроса
			$SearchStr = trim($SearchStr);
			$SearchStr = preg_quote($SearchStr, '/');
			
			$SentencePattern = '/(?<![A-ZА-Я])[\.?!…‼]+\s*(?=[A-ZА-Я][^\.]|$)/u'; //Взял с sanych.net (чтоб не велосипедить)
			
			//$IncludedSymbols = '-\"\(\)\w\s'; //Возможно добавление игнорируемых знаков препинания. Текущие символы: -")(
			switch ($Position)
			{
				case 'nfnl':
					$PhrasePattern = '/((?!\A)|[-\"\(\)\w\s]+)' . $SearchStr . '([-\"\(\)\w\s]+|(?!\Z))/u' . $Case;
					break;
				case 'first':
					$PhrasePattern = '/^' . $SearchStr . '[-"\(\)\w\s]*/u' . $Case;
					break;
				case 'last':
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '$/u' . $Case;
					break;
				default:
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '[-"\(\)\w\s]*/u' . $Case;
					break;
			}
						
			$arText = preg_split("/\n\s*/", $this->getText(), -1, PREG_SPLIT_NO_EMPTY); //Разбиваем текст на абзацы
			foreach ($arText as $ParagraphIndex => $arParagraph) //Формируем параграфы
			{
				$arParagraph = preg_split($SentencePattern, $arParagraph, -1, PREG_SPLIT_NO_EMPTY); //Разбиваем текущий абзац на предложения
				//В целях оптимизации не создаем внутренней локальной копии и работаем по ссылке со значениями массива $arParagraph
				switch ($mode) //Находим все совпадения в текущем предложении и заполняем $arResult
				{
					case 'phrase':
						foreach ($arParagraph as &$Sentence)
						{
							if(preg_match_all($PhrasePattern, $Sentence, $arTmpPhrase))
							{
								if (is_array($this->arResult)) //Условие необходимо для корректной работы array_merge()
									$this->arResult = array_merge($this->arResult, $arTmpPhrase[0]);
								else 
									$this->arResult = $arTmpPhrase[0];
							}
						}
						break;
					case 'sentence':
						foreach ($arParagraph as &$Sentence) 
						{
							if(preg_match($PhrasePattern, $Sentence)) 
								$this->arResult[] = $Sentence;
						}
						break;
					case 'paragraph':
						foreach ($arParagraph as &$Sentence)
						{
							if(preg_match($PhrasePattern, $Sentence))
								$flag = TRUE;
						}			
						if ($flag)
							$this->arResult[] = $arText[$ParagraphIndex];
						break;
					default: //Возвращает число совпадений
						foreach ($arParagraph as &$Sentence)
						{
							$this->arResult += preg_match_all($PhrasePattern, $Sentence);
						}
						break;
				}
			}
			
			return $this->arResult;
		}
	}

	static public function searchGlobal(mysqli $db, $arQuestionData, AccountManager $account)
	{
		/* Создаем и заполняем соответствующие переменные: */
		$SearchStr;
		$Position;
		$Case;
		$View;
		extract($arQuestionData);
		
		$sql = 'INSERT INTO questions(question, user_id)'
			. "	VALUES ('$SearchStr', '" . $account->getUserID() . "')";
		if ($db->query($sql)) //Добавляем запрос в БД. Начинаем процедуру поиска если добавился
		{
			$arFiles = array(); //В случае отсутствия результатов вернется пустой массив
			
			$sql = 'SELECT path '
				. '	FROM `files` ';
			$arFilePath = $db->query($sql)->fetch_all(MYSQLI_ASSOC); //Получили массив путей
				
			foreach ($arFilePath as $row) //Делаем массив объектов (ключ массива - путь к файлу)
			{
				$tmpFile = new TextFile($db, $row['path']);
	
				if ($tmpFile->search($SearchStr, $Position, $Case, $View)) //Выполняем поиск в текущем файле
					$arFiles[] = $tmpFile;
			}
			
			addInLog("Search question: '$SearchStr' with position '$Position'", $account->getLogin()); //Добавляем запрос в лог
			
			return $arFiles; 
		}
	}
	
	private function addText(array $arTextData, AccountManager $account)
	{
		unset($this->error); //Чистим от предыдущей ошибки
		
		/* Создаем и заполняем соответствующие переменные: */
		$name;
		$orig_name;
		$comment; //Переменная заполняется опционально, в случае присутсвия комментария
		extract($arTextData);
			
		//Решил не организовывать никаких дополнительных запросов имени автора, во ибежание лишних обращений к БД
		//Все равно автор хранится внутри аккаунта в оперативной памяти
		//Значительно проще (и целесообразнее) передавать его в массиве $arTextData
		if (is_string($name))
		{
			if (file_exists("./tmp/$name"))
			{
				$destination = "./text/". $account->getLogin();
				
				if (!is_dir($destination))
					if (!mkdir($destination))
						throw new RuntimeException("Can't create $destination directory for user " . $account->getLogin());
				
				$destination .= "/" . $name; //Этот относительный путь потом добавляется в БД
				rename("./tmp/$name", $destination);
				$sql = 'INSERT INTO files(path, original_name, user_id, comment)'
					. "	VALUES ('$destination', '$orig_name', '" . $account->getUserID() . "', '$comment')";
				if ($this->getDB()->query($sql))
				{
					$this->associateWithText($destination);	//Извлекаются все данные соответственно структуре БД
					addInLog("File '$name' uploaded", $account->getLogin());
				}
				else 
					$this->error = "File didn't created. Probably file with that name is created";
			}
			else
				throw new RuntimeException("File ./tmp/$name didn't find");
		}
		
		return $this;
	}
}

class Question extends Model_Common
{
	private $arQuestionInfo; //Содержит информацию о последнем поисковом запросе
	
	function __construct(mysqli $DBLink, $arQuestionData, AccountManager $account)
	{
		parent::__construct($DBLink);
		
		/* Создаем и заполняем соответствующие переменные: */
		$SearchStr;
		$Position;
		$Case;
		$View;
		extract($arQuestionData);
		
		$this->arQuestionInfo;
		
		$sql = 'INSERT INTO questions(question, position, case, user_id)'
			. "	VALUES ('$SearchStr', '$Position', '$Case','" . $account->getUserID() . "')";
		if ($this->getDB()->query($sql))
			addInLog("Search question: '$SearchStr' with position '$Position'", $account->getLogin()); //Добавляем запрос в лог
		else
			$this->error = "Не выполнено фиксирование статистики в БД";
	}
	
	static public function getTopQuestionsList($NumbersOfQuestions = 5) //СДЕЛАТЬ
	{
		if (is_int($NumbersOfQuestions) and ($NumbersOfQuestions =! 0)) //Заменить на filter_var
		{
			echo "my $NumbersOfQuestions\n";
	
			$result = "заглушка";
			return $result;
		}
		else
		{
			return FALSE;
		}
	}
}
?>