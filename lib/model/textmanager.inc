<?php
require_once './lib/function/addinlog.inc';
require_once './lib/model/model.inc';

class TextFile extends Model_Common //Инкапсулирует структуру файла и методы работы с ним
{
	private $arFileInfo; //Содержит информацию о файле согласно структуре БД
	//Массивы, хранящие результаты последнего поиска
	private $arSentence; //$arSentence[НОМЕР_АБЗАЦА][НОМЕР_ПРЕДЛОЖЕНИЯ] = МАССИВ ФРАЗ В ДАННОМ ПРЕДЛОЖЕНИИ
	private $arPhrase; //$arPhrase[НОМЕР_АБЗАЦА][НОМЕР_ПРЕДЛОЖЕНИЯ] = ФРАЗА
	
	function __construct(mysqli $DBLink, $data)
	{
		parent::__construct($DBLink);
			
		if (!is_dir('./text'))
			if (!mkdir('./text'))
				throw new RuntimeException("Can't create ./text directory");
			
		if (is_array($data))
		{
			$this->addText($data);
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
			//$arPath = glob("./text/*/$filename.txt"); //Делаем поиск по всей папке text и находим адрес файла, поковыряться с настройкой include_path
			//$path = $arPath[0];

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

	public function getResult($mode) //Возвращение параграфа пока не работает
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
	############################################################################################################################
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
			$SentencePattern = '/(?<![A-ZА-Я])[\.?!…‼]+\s*(?=[A-ZА-Я][^\.]|$)/u'; //Взял с sanych.net (чтоб не велосипедить)
			$PhrasePattern;
			// 				$IncludedSymbols = '-\"\(\)\w\s';
			switch ($Position)
			{
				case 'nfnl':
					$PhrasePattern = '/((?!\A)|[-\"\(\)\w\s]+)' . $SearchStr . '([-\"\(\)\w\s]+|(?!\Z))/u' . $Case;
					break;
				case 'first':
					$PhrasePattern = '/^' . $SearchStr . '[-"\(\)\w\s]*/u' . $Case; //возможна добавление игнорируемых знаков препинания ( пока -")( )
					break;
				case 'last':
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '$/u' . $Case;
					break;
				default:
					$PhrasePattern = '/[-"\(\)\w\s]*' . $SearchStr . '[-"\(\)\w\s]*/u' . $Case;
					break;
			}

			$arText = explode("\n", $this->getText()); 
			//$arText = preg_split("/[\s,]+/", $this->getText()); //Разбиваем текст на абзацы (можно тоже сделать регуляркой, но так быстрее)
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

	static public function searchGlobal(mysqli $db, $arQuestionData)
	{
		extract($arQuestionData); //Создает: $SearchStr, $Position, $Case, $id, $author
		
		$sql = 'INSERT INTO questions(question, user_id)'
			. "	VALUES ('$SearchStr', '$id')";
		if ($db->query($sql)) //Добавляем запрос в БД. Начинаем процедуру поиска если добавился
		{
			$arFiles = array(); //В случае отсутствия результатов вернется пустой массив
			
			$sql = 'SELECT files.path '
				. '	FROM `files` ';
			$arFilePath = $db->query($sql)->fetch_assoc(); //Получили массив путей
				
			foreach ($arFilePath as $path) //Делаем массив объектов (ключ массива - путь к файлу)
			{
				$tmpFile = new TextFile($db, $path);
				$tmpFile->search($SearchStr, $Position, $Case);
	
				if ($tmpFile->getResult()) //Проверяем есть ли в текущем тексте результаты поиска
				{
					$arFiles["$path"] = $tmpFile;
				}
			}
	
			addInLog("Search question: '$SearchStr' with position '$Position'", $author); //Добавляем запрос в лог
			
			return $arFiles; 
		}
	}
	
	private function addText(array $arTextData)
	{
		unset($this->error); //Чистим от предыдущей ошибки
		extract($arTextData); //Создает: $name, $id, $author; $orig_name, $comment (опционально).
			
		//Решил не организовывать никаких дополнительных запросов имени автора, во ибежание лишних обращений к БД
		//Все равно автор хранится внутри аккаунта в оперативной памяти
		//Значительно проще (и целесообразнее) передавать его в массиве $arTextData
		if (is_string($name) && is_string($id) && is_string($author))
		{
			if (file_exists("./tmp/$name"))
			{
				$destination = "./text/". $author;
				
				if (!is_dir($destination))
					if (!mkdir($destination))
						throw new RuntimeException("Can't create $destination directory for user $author");
				
				$destination .= "/" . $name; //Этот относительный путь потом добавляется в БД
				rename("./tmp/$name", $destination);
				$sql = 'INSERT INTO files(path, original_name, user_id, comment)'
					. "	VALUES ('$destination', '$orig_name', '$id', '$comment')";
				if ($this->getDB()->query($sql))
				{
					//Устанавливаем ассоциацию объекта с файлом
					$this->associateWithText($destination);
					addInLog("File '$name' uploaded", $author);
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
?>