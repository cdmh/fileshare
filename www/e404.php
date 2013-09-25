<?php
require_once './library/stdhdr.php';
require_once 'library/database.php';

$pyfli  = new pyfli;
$url    = $_SERVER['REQUEST_URI'];
$link   = substr($url,1);

$info = $pyfli->get_link_info($link);
if ($info === FALSE)
{
    echo "Bad path \"$link\"";
    exit;
}
else
{
    $pyfli->increment_download_count($link);

    if (isset($info['redirect_url']) && $info['redirect_url'] != '')
        header("location: {$info['redirect_url']}",true,307);  // 307 Temporary Redirect
    else if (file_exists($info['local_pathname']))
	{
		header("Expires: 0");
		header("Last-Modified: " . gmdate("D, d M Y H:i:s", $info['timestamp']) . " GMT");
//		header("Cache-Control: no-store, no-cache, must-revalidate");
//		header("Cache-Control: post-check=0, pre-check=0", false);
//		header("Pragma: no-cache");
		header("Content-Length:{$info['content_length']}");
		header("Content-Type:{$info['mime_type']}");
		header("Content-Disposition:attachment; filename=\"{$info['original_filename']}\"");
		ob_clean();
		flush();
		readfile($info['local_pathname']);
	}
	else
	{
		//var_dump($info);
		echo "<p>Link Expired.</p>";
	}

    exit;
}

?>
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>
<head>
<meta http-equiv='Content-type' content='text/html; charset=utf-8' /> 
<meta http-equiv='Content-language' content='en' /> 
<link rel='shortcut icon' href='./images/favicon.ico' />
</head>
<body>
<img src='/eveex.png' alt='' />
<p>Sorry, something went wrong. Please <a href='/'>click here to go to the home page</a>.</p>

<?php

include "beta/library/googleanalytics.php";
?>

</body>
</html>
