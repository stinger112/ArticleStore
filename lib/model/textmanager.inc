<?php
require_once './lib/function/addinlog.inc';
require_once './lib/model/model.inc';

class TextFile extends Model_Common //Инкапсулирует структуру файла и методы работы с ним
{
	/* $arFileInfo cодержит следующие поля (согласно структуре БД):
	* id
	* name
	* login
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
	public function getFileInfo($field = NULL)
	{
		return isset($field) ? $this->arFileInfo[$field] : $this->arFileInfo;
	}
	
	public function getText() //Получить текст в виде строки
	{
		return file_get_contents('./text/' . $this->arFileInfo['login'] . '/' . $this->arFileInfo['name']);
	}
	
	public function getFileName()
	{
		return $this->arFileInfo['name'];
	}
	
	public function getAuthor()
	{
		return $this->arFileInfo['login'];
	}

	public function getResult()
	{
		return $this->arResult;
	}
	
	static public function getFileList (mysqli $db) //Вернет массив имен текстовых файлов или пустой массив
	{
		$arFilePath = array();
		
		$sql = 'SELECT name '
			. '	FROM `files` ';
		if ($qResult = $db->query($sql))
		{
			while ($row = $qResult->fetch_array(MYSQLI_ASSOC)) //Достаем каждую следующую строку как массив
			{
				$arFilePath[] = $row['name'];
			}
		}

		return $arFilePath;
	}
	############################################################################################################################
	private function associateWithText($filename) //Ассоциирует объект TextFile с определенным файлом (согласно БД)
	{
		if (32 == strlen($filename))
		{
			$sql = <<<SQL
SELECT files.id, name, users.login, original_name, comment, date
FROM files
	JOIN users
		ON files.user_id = users.id
		AND files.name = '$filename'
SQL;
			if ($arRes = $this->getDB()->query($sql)->fetch_assoc())
			{
				if (is_file('./text/' . $arRes['login'] . '/' . $arRes['name']))
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
	
				$destination .= "/" . $name;
				rename("./tmp/$name", $destination);
				$sql = 'INSERT INTO files(name, original_name, user_id, comment)'
					. "	VALUES ('$name', '$original_name', '" . $account->getUserID() . "', '$comment')";
				if ($this->getDB()->query($sql))
				{
					$this->associateWithText($name);	//Извлекаются все данные соответственно структуре БД
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
	
	function __toString()
	{
		return $this->arQuestionInfo['question'];
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
		
		foreach (TextFile::getFileList($this->getDB()) as $name) //Делаем массив объектов TextFile
		{
			$tmpFile = new TextFile($this->getDB(), $name);
		
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
	
	static public function getTopQuestionsList($questNum = 5) //Возвращает массив топовых запросов
	{
		if (filter_var($questNum, FILTER_VALIDATE_INT, array( 'options' => array( 'min_range' => 1))))
		{
			$arQuestions = array();
			$db = new mysqli(MYSQL_SERVER, MYSQL_LOGIN, MYSQL_PASSWORD, MYSQL_NAME);
			
			$sql = <<<SQL
SELECT question
	FROM questions
	GROUP BY question
	ORDER BY COUNT(*) DESC
	LIMIT $questNum
SQL;
			if ($qResult = $db->query($sql))
			{
				for ($i = 0 ; $i < $questNum && $row = $qResult->fetch_array(MYSQLI_ASSOC) ; $i++) //Достаем каждую следующую строку как массив
				{
					$arQuestions[] = $row['question'];
				}
				
				return $arQuestions;
			}
		}
	}
}
?>