<?php
require_once './library/stdhdr.php';
require_once "library/alphaID.inc.php";
require_once "config.php";
require_once 'library/database.php';

$pyfli = new pyfli;

function write_upload_form()
{
    global $pyfli_external;
    echo "<h1 id='lets_go'>Let's do it</h1>";
    echo "<form id='file_upload_form' method='post' enctype='multipart/form-data' action='$pyfli_external/?form=submit'>";
    echo "<input name='id' id='id' size='27' type='hidden' />";
    echo "<input name='file' id='file' size='27' type='file' onchange='upload_file();'/><br />";
    echo "<iframe id='upload_target' style='display:none' name='upload_target' src='' style='width:800px;height:400px;border:1px solid #000;'></iframe>";
    echo "</form>";
}

// Test the pyf.li httpd process is running
$res = curl_init($pyfli_internal);
curl_setopt($res, CURLOPT_NOBODY, true);
curl_setopt($res, CURLOPT_FOLLOWLOCATION, true);
curl_exec($res);
$service_available = (curl_getinfo($res, CURLINFO_HTTP_CODE) == 200);
curl_close($res);

// Query Progress request proxy
if (isset($_GET['query_progress']))
{
    $pyfli->send_tweets();
    if ($service_available)
    {
        $res = @curl_init("$pyfli_internal/?query_progress={$_GET['query_progress']}");
        @curl_setopt($res, CURLOPT_URL, "$pyfli_internal/?query_progress={$_GET['query_progress']}");
        curl_exec($res);
        @curl_close($res);
    }
    else
        echo "{\"version\":1,error:\"The service is currently unavailable.\"}";
    exit;
}

// http://docs.amazonwebservices.com/AmazonS3/latest/dev/index.html?UploadObjSingleOpPHP.html
// use REDUCED_REDUNDANCY

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>pyf.li | Post Your Files</title>
<meta name="keywords" content="pyfli, pyf.li, realtime, real-time, file sharing, sharing, files, filesharing, publishing, audio, music, text, documents, cloud-based file sharing, ge.tt" />
<meta name="description" content="cloud-based real-time file sharing. no plugins, no registration, no account" />
<link rel="stylesheet" href="./main.css" type="text/css" media="screen" />
<script type="text/javascript" language="javascript" src='./pyfli.js'></script>
</head>

<body onload="init();" onbeforeunload="return canunload();">

<div class='content'>
<div class='logo'><img alt='' src='./images/title.png' height='117' /></div>
<div class='mainpanel'>
<h1>Real-time file sharing.</h1>
<h2>Don't wait for files to upload before downloads can begin.</h2>

<div class='try_twitter'>
<p>Have you tried <b>Tweet and Share</b>, our <a href='./tweetandshare.php' target='_blank'>real-time file sharing on Twitter</a>?</p>
</div>

<div id='instructions'>
<p>Use the file selector below to pick which file you want to share.<br/>
We'll give you a URL to share with your friends to download the file. Immediately.</p>
</div>

<div id='progress'>
<div id='pbar'><div id="pcent">&nbsp;</div></div>
<div id='pcent2'>&nbsp;</div>
</div>

<div id='download'>
<p>Share this link with your friends to download your file.<br />
There's no need to wait for the upload to finish first, they can start to download straight away !<br />
The link is valid for 5 days.</p>
<p class='download_link'><span id='download_link'></span> <span id='copy'></span></p>
</div>
<div id='failed' class='unavailable'>
<p>Sorry, an unrecoverable error occurred uploading this file.</p>
<p>We are in Alpha Testing and it looks like we've suffered a problem.</p>
</div>

<div id='go_again'>
<?php
if ($service_available)
    write_upload_form();
else
{
    echo "<div class='unavailable'>";
    echo "<p>Sorry, the service is currently unavailable.</p>";
    echo "<p>We are in Alpha Testing and it looks like we've suffered a problem.<br />We'll fix it and restore our service as soon as possible.</p>";
    echo "<p>Please check back soon.</p>";
    echo "</div>";
}
?>
</div>

<div id='uploading'>0</div>
<div id='DEBUG'></div>
</div>
</div>

<div class='footnote'>
<p>Copyright &copy; 2011 Craig Henderson. All rights reserved.&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href='./about.php'>About pyf.li</a></p>
</div>

</body>
</html>
