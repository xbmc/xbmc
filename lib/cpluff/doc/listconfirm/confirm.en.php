<?php echo '<?xml version="1.0" encoding="UTF-8"?>'; ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML Basic 1.0//EN" "http://www.w3.org/TR/xhtml-basic/xhtml-basic10.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
  <head>
    <meta http-equiv="Content-Type" content="text/html;charset=UTF-8"/>
    <meta name="author" content="Johannes Lehtinen"/>
    <meta name="copyright" content="Copyright 2007 Johannes Lehtinen"/>
    <title>Confirm subscription</title>
    <link rel="Stylesheet" type="text/css" href="../cpluff_style.css"/>
  </head>
  <body>
    <div id="content">

    <h1>Confirm subscription</h1>

    <p>
      Your subscription request has been processed successfully but your
      mailing list subscription has not yet been activated. To complete your
      subscription you have to visit the link included in the verification
      e-mail that has just been sent to your e-mail address
      <?php echo htmlspecialchars($_REQUEST['address']); ?>. It might take
      some time for the e-mail to arrive into your inbox.
    </p>
    <p>
      Return to
      <a href="../lists.en">the C-Pluff mailing list page</a>.
    </p>

    <p class="footer">
      Copyright 2007 <a href="http://www.jlehtinen.net/">Johannes Lehtinen</a><br/>
      Validation:
        <a href="http://validator.w3.org/check?uri=referer">XHTML Basic 1.0</a>,
        <a href="http://jigsaw.w3.org/css-validator/check/referer">CSS 2</a>
    </p>

    </div>
  </body>
</html>
