<?php
$root = '/';
$GLOBALS["DOCUMENT_ROOT"] = $_SERVER['DOCUMENT_ROOT'];
if (strlen($DOCUMENT_ROOT) > 4  &&  substr($DOCUMENT_ROOT,strlen($DOCUMENT_ROOT)-4) == '.php')
    $DOCUMENT_ROOT = substr($DOCUMENT_ROOT,0,strrpos(str_replace('\\','/',$DOCUMENT_ROOT), $root));

if (stristr(PHP_OS, 'WIN'))
    $path_separator = ';';
else
    $path_separator = ':';

ini_set('display_errors', 0);
if ($_SERVER["SERVER_NAME"] == 'localhost' || (isset($_COOKIE['userid']) && $_COOKIE['userid'] == 1))
    ini_set('display_errors', 1);

@session_start();
date_default_timezone_set('Europe/London');
set_include_path(get_include_path() . $path_separator . $GLOBALS['DOCUMENT_ROOT'] . $root);

function __autoload($class_name)
{
    require_once "library/".strtolower($class_name).".php";
}

function remove_parameter($del,$query_string=NULL)
{
    if (is_array($del))
        $to_remove = $del;
    else
        $to_remove = array($del);

    $new_query_string = '';
    if ($query_string == NULL)
        $query_string = $_SERVER["QUERY_STRING"];
    if ($query_string != '')
    {
        $params = explode('&', $query_string);
        foreach ($params as $param)
        {
            list($key,$value) = explode('=',$param);
            $found = false;
            foreach ($to_remove as $remove)
            {
                if ($key == $remove)
                    $found = true;
            }
            if (!$found)
            {
                if ($new_query_string != '')
                    $new_query_string .= '&';
                $new_query_string .= "$key=$value";
            }
        }
    }

    return $new_query_string;
}

?>