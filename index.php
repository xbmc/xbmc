<!DOCTYPE html > <head> <title> My Hit counter Test</title> </head> <body bgcolor = "#99CC99" > <h1> this is my Test </h1> </body> </html>
<?php
$hits = intval(file_get_contents('counter.txt'));
file_put_contents('counter.txt',$hits+1);
echo $hits;
?>
