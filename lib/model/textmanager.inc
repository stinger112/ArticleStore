<?php
require_once './lib/function/addinlog.inc';
require_once './lib/model/model.inc';

class TextFile extends Model_Common //Инкапсулирует структуру файла и методы работы с ним
{
	private $arFileInfo; //Содержит информацию о файле согласно структуре БД
	//Массивы, хранящие результаты последнего поиска
	private $arSentence; //$arSentence[НОМЕР_АБЗАЦА][НОМЕР_ПРЕДЛОЖЕНИЯ] = МАССИВ ФРАЗ В ДАННОМ ПРЕДЛОЖЕНИИ
	private $arPhrase; //$arPhrase[НОМЕР_АБЗАЦА][НОМЕР_ПРЕДЛОЖЕНИЯ] = ФРАЗА

	function __construct(mysqli $DBLink, array $arTextData = array(), $filePath)
	{
		parent::__construct($DBLink/*, 'content'*/);
			
		if (!is_dir('./text'))
			if (!mkdir('./text'))
			throw new RuntimeException("Can't create ./text directory for user {$arTextData['author']}");
			
		if ($filePath)
		{
			$this->associateWithText($filePath);
		}
		else
		{
			$this->addText($arTextData);
		}
	}

	/* protected function CreateDB()
		{
	$sql = 'CREATE DATABASE `content`';
	if (!$this->getDB()->query($sql))
		throw new mysqli_sql_exception($this->getDB()->error);

	$sql = 'CREATE TABLE content.files ('
			. '	`path`		VARCHAR(255)	NOT NULL, '
			. '	`author`	VARCHAR(32)		NOT NULL, '
			. '	`comment`	TEXT			NOT NULL, '
			. '	`date`		TIMESTAMP		NOT NULL	DEFAULT CURRENT_TIMESTAMP, '
			. '	PRIMARY KEY (`path`) '
			. '	) '
	. '	ENGINE = myisam; ';
	if (!$this->getDB()->query($sql))
		throw new mysqli_sql_exception($this->getDB()->error);

	$sql = 'CREATE TABLE content.questions (' //Без первичного ключа
			. '	`date`		TIMESTAMP		NOT NULL	DEFAULT CURRENT_TIMESTAMP, '
			. '	`author`	VARCHAR(32)		NOT NULL, '
			. '	`question`	TINYTEXT		NOT NULL '
			. '	) '
	. '	ENGINE = myisam; ';
	if(!$this->getDB()->query($sql))
		throw new mysqli_sql_exception($this->getDB()->error);

	return TRUE;
	}  */

	###############################################Методы доступа к членам класса###############################################
	private function associateWithText($path) //Ассоциирует объект TextFile с определенным файлом (согласно БД)
	{
		if (is_string($path) && is_file($path))
		{
			//$arPath = glob("./text/*/$filename.txt"); //Делаем поиск по всей папке text и находим адрес файла, поковыряться с настройкой include_path
			//$path = $arPath[0];

			$sql = 'SELECT * '
					. '	FROM `files` '
							. '	WHERE `path` = \'' . $path . '\'';
			if ($qRes = $this->getDB()->query($sql))
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

	public function getTopQuestionsList($NumbersOfQuestions = 5)
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

	public function getResult($mode)
	{
		switch ($mode)
		{
			case 'paragraph':
				$arText = explode("\n", $this->getText()); //Разбиваем текст на абзацы
				//Узнаем номера абзацев и возвращем массив по соотв. номерам
				return $arText["$i"];
				break;
					
			case 'sentence':
				return $this->arSentence;
				break;
					
			case 'phrase':
				return $this->arPhrase;
				break;
					
			default:
				if ($this->arSentence === array())
					return FALSE;
				else
					return TRUE;
				break;
		}
	}
	//Сделать метод возвращения результатов поиска
	############################################################################################################################
	//Определиться с форматом вывода результатов
	public function search($SearchStr, $Position, $Case) //Запилить поддержку русского языка
	{
		//Чистим массивы от предыдущих результатов, что бы они не попали в новый поиск
		unset($this->arPhrase);
		unset($this->arSentence);
			
		if (is_string($SearchStr))
		{
			if ($Case)
				$Case = '';
			else
				$Case = 'i';
			$SearchStr = trim($SearchStr); //Возможно это не задача модели
			$SearchStr = preg_quote($SearchStr, '/'); //И это тоже
			$SentencePattern = '/(?<![A-ZА-Я])[\.?!…‼]+\s*(?=[A-ZА-Я][^\.]|$)/'; //Взял с sanych.net (чтоб не велосипедить)
			$PhrasePattern;
			// 				$IncludedSymbols = '-\"\(\)\w\s';
			switch ($Position)
			{
				case 'nfnl':
					$PhrasePattern = '/((?!\A)|[-\"\(\)\w\s]+)' . $SearchStr . '([-\"\(\)\w\s]+|(?!\Z))/' . $Case;
					break;
				case 'first':
					$PhrasePattern = '/^' . $SearchStr . '[-"\(\)\w\s]*/' . $Case; //возможна добавление игнорируемых знаков препинания ( пока -")( )
					break;
				case 'last':
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '$/' . $Case;
					break;
				default:
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '[-"\(\)\w\s]*/' . $Case;
					break;
			}

			$arText = explode("\n", $this->getText()); //Разбиваем текст на абзацы (можно тоже сделать регуляркой)
			foreach ($arText as $ParagraphNumber => &$arParagraph) //Формируем параграфы
			{
				$arParagraph = preg_split($SentencePattern, $arParagraph, NULL, PREG_SPLIT_NO_EMPTY); //Почему-то все-равно возвращает пустые абзацы
				foreach ($arParagraph as $SentenceNumber => &$Sentence)
				{
					$Sentence = trim($Sentence); //Обязательно! (в частности удаляет виндовый \r в последнем предложении абзаца)
					if(preg_match_all($PhrasePattern, $Sentence, $arTmpPhrase)) //Находит все совпадения в текущем предложении. Сохраняет все фразы в массив $arTmpPhrase[0]
					{
						$this->arSentence[$ParagraphNumber][$SentenceNumber] = $Sentence;
						$this->arPhrase[$ParagraphNumber][$SentenceNumber] = $arTmpPhrase[0];
					}
				}
			}

			return $this;
		}
			
	}

	static public function searchGlobal(mysqli $db, $SearchStr, $Position, $Case)
	{
		$sql = 'SELECT files.path '
				. '	FROM `files` ';
		$arFilePath = $this->getDB()->query($sql)->fetch_assoc(); //Получили массив путей
			
		foreach ($arFilePath as $path) //Делаем массив объектов (ключ массива - путь к файлу)
		{
			$tmpFile = new TextFile($db, $path);
			$tmpFile->search($SearchStr, $Position, $Case);

			if ($tmpFile->getResult()) //Проверяем есть ли в текущем тексте результаты поиска
			{
				$arFiles["$path"] = $tmpFile;
			}
				
		}
			
		//Добавляем запрос в БД

		return $arFiles;
	}
		
	private function addText(array $arTextData)
	{
		unset($this->error); //Чистим от предыдущей ошибки
		extract($arTextData);
			
		//Решил не организовывать никаких дополнительных запросов имени автора, во ибежание лишних обращений к БД
		//Все равно автор хранится внутри аккаунта в оперативной памяти
		//Значительно проще (и целесообразнее) передавать его в массиве $arTextData
		if (is_string($id) && is_string($text) && is_string($comment))
		{
			$destination = "./text/". $author;

			if (!is_dir($destination))
				if (!mkdir($destination))
				throw new RuntimeException("Can't create $destination directory for user $author");

			$filename = hash("md5", $text);
			$destination .= "/" . $filename . '.txt'; //Относительный путь добавляется в БД!
			if (!file_exists($destination)) //Одинаковые тексты не добавляются
			{
				if (file_put_contents($destination, $text))
				{
					$sql = 'INSERT INTO files(path, user_id, comment)'
							. "	VALUES ('$destination', '$id', '$comment')";
					if ($this->getDB()->query($sql))
					{
						//Устанавливаем ассоциацию объекта с файлом
						$this->associateWithText($destination);
						addInLog("File uploaded", $author);
					}
				}
			}
			else
				$this->error = "Такой текст уже существует";

			return $this;
		}
	}
}
?>