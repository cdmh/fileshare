<?php
require_once './library/stdhdr.php';
require_once 'library/database.php';
require_once 'config.php';

// Test the pyf.li httpd process is running
$res = curl_init($pyfli_internal);
curl_setopt($res, CURLOPT_NOBODY, true);
curl_setopt($res, CURLOPT_FOLLOWLOCATION, true);
curl_exec($res);
$service_available = (curl_getinfo($res, CURLINFO_HTTP_CODE) == 200);
curl_close($res);

$pyfli  = new pyfli;
$uploaded = $pyfli->get_uploaded_files_list();
//echo "<pre>"; var_dump($uploaded); echo "</pre>";
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>pyf.li | System Dashboard</title>
<link rel="stylesheet" href="./main.css" type="text/css" media="screen" />
<style type="text/css">
div.content {
    width:90%;
    margin-left:auto;
    margin-right:auto;
}

div.mainpanel {
    width:80%;
    height:auto;
}

a, a:visited {
text-decoration:none;
color:blue;
}

table {
font-size:10pt;
width:100%;
margin-top:20px;
}

tr.odd {
background-color:#bdb;
}

th {
background-color:#ccc;
line-height:200%;
text-align:left;
}

tr.even {
background-color:#ddb;
}

td {
padding-left:10px;
padding-right:10px;
}

tr.expired, tr.expired a {
color:#966;
}

img {
border:none;
}

p.service {
font-weight:bold;
font-size:125%;
}

p.unavailable {
width:100%;
background-color:red;
color:yellow;
text-align:center;
margin-top:35px;
padding:3px;
}

</style>
</head>

<body>
<div class='content'>
<div class='logo'><img alt='' src='./images/title.png' height='117' /></div>
<div class='mainpanel'>
<?php
	if ($service_available)
		;//echo "<p class='service'>Service is good</p>";
	else
		echo "<p class='service unavailable'>Service is not available</p>";
?>
<table cellspacing='0' cellpadding='2'>
<tr>
<th></th>
<th>Upload time</th>
<th>Age</th>
<th>Original Filename</th>
<th>Size</th>
<th>Mime Type</th>
<th>Downloads</th>
</tr>
<?php
for ($i=0; $i<sizeof($uploaded); ++$i)
{
	echo "<tr class='";
	if ($uploaded[$i]['age_days'] > 5)
		echo 'expired ';
	echo ($i % 2 == 0)? 'even':'odd';
	echo "'>";

	$link = $uploaded[$i]['redirect_url'];
	if ($link == '')
		$link = "/{$uploaded[$i]['link']}";
	$ts = date('d M Y H:i:s',$uploaded[$i]['upload_ts']);

	$rownum = sizeof($uploaded)-$i;
	echo "<td>$rownum</td>";
	echo "<td nowrap>$ts</td>";
	echo "<td>{$uploaded[$i]['age_days']}</td>";
	echo "<td><a href='$link'>{$uploaded[$i]['original_filename']}</a></td>";
	echo "<td align='right'>" . number_format($uploaded[$i]['content_length']) . "</td>";
	echo "<td>{$uploaded[$i]['mime_type']}</td>";
	echo "<td>{$uploaded[$i]['download_count']}</td>";
	echo "</tr>";
}
?>
</table>
</div>
</div>
</body>
</html>
