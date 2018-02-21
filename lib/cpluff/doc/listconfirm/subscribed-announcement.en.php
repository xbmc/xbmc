<?php echo '<?xml version="1.0" encoding="UTF-8"?>'; ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML Basic 1.0//EN" "http://www.w3.org/TR/xhtml-basic/xhtml-basic10.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
  <head>
    <meta http-equiv="Content-Type" content="text/html;charset=UTF-8"/>
    <meta name="author" content="Johannes Lehtinen"/>
    <meta name="copyright" content="Copyright 2007 Johannes Lehtinen"/>
    <title>Subscription completed</title>
    <link rel="Stylesheet" type="text/css" href="../cpluff_style.css"/>
  </head>
  <body>
    <div id="content">

    <h1>Subscription completed</h1>

    <p>
      Welcome to the C-Pluff announcement mailing list!
      You have successfully subscribed to the list and forthcoming
      announcements will be delivered to your
      e-mail address <?php echo htmlspecialchars($_REQUEST['address']); ?>.
    </p>
    <p>
      You can unsubscribe from this mailing list at any time by visiting
      <a href="../lists.en">the C-Pluff mailing list page</a>.
    </p>
    <p>
      Now you may return to <a href="../index.en">the main C-Pluff page</a>.
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
