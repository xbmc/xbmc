<html>
<head>
</head>
<body>

<?php
	// echo "<pre>";
	// print_r($_FILES);
	// print_r($_POST);
	// print_r($HTTP_POST_VARS);
	// echo "</pre>";

	$uploaddir = 'c:/tmp/saved/';
	$uploadfile = $uploaddir . basename($_FILES['userfile']['name']);

	if (isset($_POST['MAX_FILE_SIZE'])) {
		// foreach ($_FILES as $key => $value) {
		// 	echo "FILES $key is $value <br />\n";
		// }

		echo "<p>Upload temp file " . $_FILES['userfile']['tmp_name'] . "</p>";

		if (move_uploaded_file($_FILES['userfile']['tmp_name'], $uploadfile)) {
			echo "<p><b>File saved as " . $uploadfile . "</b></p>";
		} else {
		   echo "<b>Could not move uploaded file</b>\n";
		}
	}

?>


<!-- The data encoding type, enctype, MUST be specified as below -->

<form enctype="multipart/form-data" action="/uploadTest.php" method="POST">
<input type="hidden" name="MAX_FILE_SIZE" value="134217728" /> 
Send this file: <input name="userfile" type="file" />
<input type="submit" value="Send File" />
</form> 

<body>
<html>
