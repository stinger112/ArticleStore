<!DOCTYPE html>
<html>
<head>
<title>Text details page</title>
</head>

<body>
	<div id="fileinfo">
		<h3>File info:</h3>
		<p>Автор: <?php echo $data->getAuthor() ?></p>
		<p>Оригинальное имя: <?php echo $data->getFileInfo('original_name') ?></p>
		<p>Дата добавления: <?php echo $data->getFileInfo('date') ?></p>
		<a href="text/" style="text-decoration: none"><input type="button" value="Скачать оригинал" /></a>
	</div>
	<div id="text">
		<h3>Text:</h3>
		<pre><?php echo htmlspecialchars($data, ENT_NOQUOTES|ENT_HTML5) ?></pre>
	</div>
	<div id="comment">
		<h3>Comment:</h3>
		<pre><?php echo $data->getFileInfo('comment') ?></pre>
	</div>
</body>
</html>