<?php
require_once './lib/function/addinlog.inc';
require_once './lib/model/model.inc';

class TextFile extends Model_Common //Инкапсулирует структуру файла и методы работы с ним
{
	/* $arFileInfo cодержит следующие поля (согласно структуре БД):
	* id
	* path
	* user_id
	* original_name
	* comment
	* date
	*/
	private $arFileInfo;
	private $arResult; //Содержит результаты последнего поиска
	
	function __construct(mysqli $DBLink, $data, AccountManager $account = NULL)
	{
		parent::__construct($DBLink);
			
		if (!is_dir('./text'))
			if (!mkdir('./text'))
				throw new RuntimeException("Can't create ./text directory");
			
		if (is_array($data))
			$this->addText($data, $account);
		elseif (is_string($data))
			$this->associateWithText($data);
		else
			$this->error = "Внутренняя ошибка."; //Получен неверный тип аргумента
	}
	
	function __toString()
	{
		return $this->getText();
	}
	
	###############################################Методы доступа к членам класса###############################################
	public function getFileInfo($field)
	{
		return isset($field) ? $this->arFileInfo[$field] : $this->arFileInfo;
	}
	
	public function getText() //Получить текст в виде строки
	{
		$path = $this->arFileInfo['path'];
		return file_get_contents($path);
	}
	
	public function getFileName()
	{
		return substr($this->arFileInfo['path'], strlen($this->arFileInfo['path']) - 32);
	}
	
	public function getAuthor()
	{
		if (preg_match('./text/(.+)/[\w]{32}', $this->arFileInfo['path'], $mathes))
			return $mathes[1][0];
	}

	public function getResult()
	{
		return $this->arResult;
	}
	
	static public function getFilePathList (mysqli $db) //Вернет массив путей текстовых файлов или пустой массив
	{
		$sql = 'SELECT path '
			. '	FROM `files` ';
		if ($arFilePath = $db->query($sql)->fetch_all(MYSQLI_ASSOC)) //Получили массив путей
		{
			foreach ($arFilePath as &$row) //Необходимо для приведения результатов fetch_all() к одномерному массиву
			{
				$row = $row['path'];
			}
		}
		else
			$arFilePath = array();
	
		return $arFilePath;
	}
	############################################################################################################################
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
	
	private function addText(array $arTextData, AccountManager $account) //Добавляет новый текст в систему
	{
		extract($arTextData); // Создаем и заполняем переменные
			
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
					. "	VALUES ('$destination', '$original_name', '" . $account->getUserID() . "', '$comment')";
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
	
	public function search($question, $position = 'any', $case = FALSE, $mode = 'sentence') //Поиск по текущему текстовому файлу. Заполняет массив $arResult
	{
		if (is_array($this->arFileInfo) && is_string($question))
		{
			unset($this->arResult); //Чистим массивы от предыдущих результатов, что бы они не попали в новый поиск

			/* Дополнительное формирование поискового запроса */
			$case = $case ? '' : 'i';
			$question = preg_quote($question, '/');
			
			$SentencePattern = '/(?<![A-ZА-Я])[\.?!…‼]+\s*(?=[A-ZА-Я][^\.]|$)/u'; //Взял с sanych.net (чтоб не велосипедить)
			
			$IncludedSymbols = '-")(\w\s'; //Игнорируемые знаки препинания, принимающиеся за часть фразы
			switch ($position)
			{
				case 'nfnl':
					$PhrasePattern = "/((?!\A)|[$IncludedSymbols]+)" . $question . "([$IncludedSymbols]+|(?!\Z))/u" . $case;
					break;
				case 'first':
					$PhrasePattern = '/^' . $question . "[$IncludedSymbols]*/u" . $case;
					break;
				case 'last':
					$PhrasePattern = '/[-"\(\)\w\s]*' . $question . '$/u' . $case;
					break;
				default:
					$PhrasePattern = "/[$IncludedSymbols]*" . $question . "[$IncludedSymbols]*/u" . $case;
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
								$this->arResult = array_merge((array)$this->arResult, $arTmpPhrase[0]);
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

}

class Question extends Model_Common
{
	/* $arQuestionInfo содержит следующие поля:
	* question
	* position
	* case
	* view
	* nostat - флаг статистики (присутствует опционально)
	*/
	private $arQuestionInfo;
	private $arTextList = array(); //Здесь хранится массив текстов, полученный в результате последнего запроса
	
	function __construct(mysqli $DBLink, $arQuestionData)
	{
		parent::__construct($DBLink);
		$this->arQuestionInfo = $arQuestionData;
	}
	
	public function getQuestionInfo($field)
	{
		return isset($field) ? $this->arQuestionInfo[$field] : $this->arQuestionInfo;
	}
	
	public function getTextList()
	{
		return $this->arTextList;
	}
	
	public function query(AccountManager $account) //Проводит поисковый запрос, возвращает массив TextFile либо пустой массив, фиксирует статистику
	{
		extract($this->arQuestionInfo); // Создаем и заполняем переменные
		
		foreach (TextFile::getFilePathList($this->getDB()) as $path) //Делаем массив объектов TextFile
		{
			$tmpFile = new TextFile($this->getDB(), $path);
		
			if ($tmpFile->search($question, $position, $case, $view)) //Выполняем поиск в текущем файле
				$this->arTextList[] = $tmpFile;
		}
			
		if (!$nostat) //Возможно не вести статистику
		{
			$sql = 'INSERT INTO questions(question, position, `case`, user_id)'
				. "	VALUES ('$question', '$position', '$case', '" . $account->getUserID() . "')";
			$this->getDB()->query($sql);
						
			addInLog("Search question: '$question' with position '$position'", $account->getLogin()); //Добавляем запрос в лог
		}
			
		return $this->arTextList;
	}
	
	/* ---!!!Пока не работает!!!--- */
	static public function getTopQuestionsList($QuestNum = 5) //Возвращает массив объектов Question
	{
		if (filter_var($QuestNum, FILTER_VALIDATE_INT, array( 'options' => array( 'min_range' => 1))))
		{
			//Запрос должен возвращать строки сгруппированные по числу повторений поля question
			$sql = 'SELECT * ' 
				. '	FROM `question` ';
			if ($qResult = $db->query($sql))
			{
				while (($arResult = $qResult->fetch_array(MYSQLI_ASSOC)) || $i < 5)
				{
					
					$i++;
				}
			}
			else
				$arFilePath = array();
		}
	}
}
?>