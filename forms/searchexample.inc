<p>Top 5 queries:<br>
<?php 
foreach (Question::getTopQuestionsList() as $index => $query)
{
	echo ($index + 1) . ' ' . $query . '<br>';
}
?>
</p>