<form action="" method="GET">
	<p>
		<input type="text" name="query" placeholder="Search query">
		<input type="submit" value="Искать">
	</p>
	<p>Case sensitive<input type="checkbox" name="case" value="set"></p>	
	<p>Search position:<br>
		<input type="radio"	name="position"	value="first"> First<br>
		<input type="radio"	name="position"	value="last"> Last<br>
		<input type="radio"	name="position"	value="nfnl"> Not first and not last<br>
		<input type="radio"	name="position"	value="any" checked> Anywhere<br>
	</p>	
	<p>Result view:<br>
		<input type="radio"	name="view"	value="phrase"	checked> Phrase<br>
		<input type="radio"	name="view"	value="sentence"> Sentence<br>
		<input type="radio"	name="view"	value="paragraph"> Paragraph<br>
	</p>
</form>