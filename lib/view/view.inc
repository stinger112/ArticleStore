<?php
class Pagination
{
	private $start; //Стартовая позиция
	private $length; //Количество элементов на страницу
	
	public function __construct($start = 0, $lenght = 4)
	{
		$this->start = (int)$start;
		$this->length = (int)$lenght;
	}
	
	public function getStart()
	{
		return $this->start;
	}
	
	public function getLength()
	{
		return $this->length;
	}
}

class View
{
	private $arNotification;
	
	private $question;
	private $pagination;

	###############################################Методы доступа###############################################
	
	public function addNotification($notification)
	{
		$this->arNotification[] = $notification;
	}
	
	public function clearNotification()
	{
		$this->arNotification = NULL;
	}
	
	public function addQuestion(Question $question, Pagination $pagination)
	{
		$this->question = $question;
		$this->pagination = $pagination;
	}
	
	##########################################Отображение результатов###########################################
	
	public function displayPage($pagePath, $data = NULL)
	{	
		if (!@include($pagePath))
			throw new BadMethodCallException("File \"$pagePath\" didn't load");
	}
	
	public function displayMain()
	{
		$this->displayPage('./forms/main.inc');
	}
	
	public function displayText(TextFile $text)
	{
		$this->displayPage('./forms/textdetail.inc', $text);
	}

	private function displayNotification($delimiter = "<br>") //Отображает уведомление
	{
		foreach ($this->arNotification as $value)
		{
			echo $value . $delimiter;
		}
	}
	
	private function displayResults() //Отображает результаты поиска с учетом пагинации
	{
		if ($arTextList = $this->question->getTextList())
		{
			$start = $this->pagination->getStart();
			$end = $start + $this->pagination->getLength();
			
			foreach ($arTextList as $text)
			{
				if ($i >= $start && $i < $end) //Отображаем количество результатов согласно пагинации
				{
					if ($arResult = $text->getResult())
					{
						$num = 1;
						$textName = $text->getFileName();
						$author = $text->getAuthor();
						
						echo "<h4><a href=\"detail/$textName/\" target=\"_blank\">$textName</a></h4>";
						foreach ($arResult as $result)
						{
							echo "$num). $result<br>";
							$num++;
						}
					}
				}
				$i++;
			}
		}
	}
	
	private function displayPaginationRef() //Отображает строку с номерами страниц пагинации (номер - ссылка)
	{
		if ($this->question && $this->pagination)
		{
			extract($this->question->getQuestionInfo());
			$length = $this->pagination->getLength();
			
			$NumOfPages = ceil(count($this->question->getTextList()) / $length);
			$current = ceil($this->pagination->getStart() / $length);
			
			for ($i = 0; $i < $NumOfPages; $i++)
			{
				if ($i == $current)
					echo ($i+1) . ' '; //Делаем текущую страницу числом без ссылки
				else
					echo "<a href=\"?query=$question&case=$case&position=$position&view=$view&start=" . ($i*$length) . "&nostat\">" . ($i+1) . "</a> ";
			}
		}
	}
}
?>