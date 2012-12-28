<?php

class View //Пока заглушка (поэтому содержит одинаковые методы)
{
	private $arNotification;

	public function getNotification()
	{
		return $this->arNotification;
	}

	private function displayForm($formPath)
	{
		if (!@include($formPath))
			throw new BadMethodCallException("File \"$formPath\" didn't load");
	}

	public function displayPage($pagePath)
	{
		if (!@include($pagePath))
			throw new BadMethodCallException("File \"$pagePath\" didn't load");
	}

	public function displayMain()
	{
		$this->displayPage('./forms/main.inc');
	}

	public function addNotification($notification) //Пока просто по номеру
	{
		$this->arNotification[] = $notification;
	}

	public function displayNotification($delimiter = NULL)
	{
		if (!is_string($delimiter))
			$delimiter = "\n";
			
		foreach ($this->getNotification() as $value)
		{
			echo $value . $delimiter;
		}

	}
	public function clearNotification()
	{
		$this->arNotification = __;
	}

}

class MainView extends View
{

	public function displayArray()
	{
			
	}
}
?>