<p>Top 5 queries:<br>
<?php 
foreach (Question::getTopQuestionsList() as $index => $query)
{
	echo ($index + 1) . ' ' . (htmlspecialchars($query, ENT_NOQUOTES|ENT_HTML5)) . '<br>';
}
?>
</p>