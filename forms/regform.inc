<!DOCTYPE html>
<html>
<head>
<title>Регистрация пользователя</title>
<link rel="stylesheet" href="/css/auth.css" type="text/css">
</head>

<body>
	<div id="content">
		<h1>Регистрация</h1>
		<p>
			<?php $this->displayNotification() ?>
		</p>
		<form action="" method="POST">
			<p>
				*<input type="text" name="login" size="33"
					placeholder="Введите логин">
			</p>
			<p>
				*<input type="password" name="pass" size="33"
					placeholder="Введите пароль">
			</p>
			<p>
				*<input type="password" name="pass2" size="33"
					placeholder="Подтверждение пароля">
			</p>
			<p>
				<input type="submit" value="Зарегистрироваться">
			</p>
		</form>
	</div>
</body>
</html>
