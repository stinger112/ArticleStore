<!DOCTYPE html>
<html>
<head>
<title>Авторизация</title>
<link rel="stylesheet" href="/css/auth.css" type="text/css">
</head>

<body>
	<div id="content">
		<h1>Авторизация</h1>
		<p>
			<?php $this->displayNotification() ?>
		</p>
		<form method="post" action="?auth">
			<p>
				<input type="text" name="login" placeholder="Логин">
			
			
			<p>
				<input type="password" name="pass" placeholder="Пароль">
			
			
			<p>
				Запомнить меня <input type="checkbox" name="mem" value="set"> <input
					type="submit" value="Войти">
		
		</form>
		<p>
			<a href=/registration.php>Регистрация</a>
		</p>
	</div>
</body>
</html>
