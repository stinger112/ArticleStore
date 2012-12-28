<!DOCTYPE html>
<html>
<head>
<title>Main page</title>
<link type="text/css" rel="stylesheet" href="/css/mainstyle.css">
</head>

<body>
	<div id="content">
		<div id="upload">
			<h3>Upload form:</h3>
			<?php $this->displayForm('./forms/uplform.inc') ?>
		</div>
		<div id="search">
			<h3>Search:</h3>
			<?php $this->displayForm('./forms/searchform.inc') ?>
			<a href="?exit" id="exitButtom"><input type="button" value="Выход" />
			</a>
		</div>
	</div>
	<div id="footer">
		<h4>Result:</h4>
		<?php $this->displayNotification() ?>
	</div>
</body>
</html>
