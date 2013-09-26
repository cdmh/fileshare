<?php

//!!! setcookie('userid','',time()-(3600 * 25),'/');  // -25 hours will delete the cookie

class Database
{
    private $db_username;
    private $db_password;
    private $db_schema;
    private $db_server;
    private $_db_connection = NULL; // don't use directly

    function __construct()
    {
        if ($_SERVER["SERVER_NAME"] == 'localhost'  ||  substr($_SERVER["SERVER_NAME"],0,12) == '192.168.123.')
        {
            $this->db_username  = "pyfli";
            $this->db_password  = "PYFl1";
            $this->db_schema    = "pyfli";
            $this->db_server    = "localhost";
        }
        else
        {
            $this->db_username  = "whereilive";
            $this->db_password  = "3v3ex1201";
            $this->db_schema    = "whereilive";
            $this->db_server    = "213.171.200.51";
        }
    }
    
    function __destruct()
    {
        $this->close_database();
    }

    function userid()
    {
        if (isset($_SESSION['pyfliuserid']))
            return $_SESSION['pyfliuserid'];
        return 0;
    }

    function save_twitter_auth($twitter)
    {
        $access_token = $twitter->get_access_token();

	    $this->db_query("DELETE FROM pyfli_social_networks WHERE network='twitter' AND screen_name='{$access_token['screen_name']}';", $this->_db_connection);

        $this->db_query("INSERT INTO pyfli_social_networks (network,oauth_token,oauth_token_secret,screen_name) VALUES ('?','?','?','?');",
            'twitter', $access_token['oauth_token'], $access_token['oauth_token_secret'], $access_token['screen_name']);

        $_SESSION['pyfliuserid'] = $this->insert_id();
    }

    function get_oauth_tokens($network)
    {
        $auth = false;
        $result = $this->db_query("SELECT screen_name,oauth_token,oauth_token_secret".
                                  " FROM pyfli_social_networks".
                                  " WHERE user_id=? AND network='?';", $this->userid(), $network);
        if ($result  &&  ($row = mysql_fetch_row($result)))
        {
            $auth = array();
            $auth['screen_name']        = $row[0];
            $auth['oauth_token']        = $row[1];
            $auth['oauth_token_secret'] = $row[2];
            $access_token[$network]     = $auth;
        }

        return $auth;
    }

    /*
      PROTECTED
    */
    protected function db_connect()
    {
        if ($this->_db_connection == NULL)
        {
            $this->_db_connection = mysql_connect($this->db_server,$this->db_username,$this->db_password);
            if (!@mysql_select_db($this->db_schema, $this->_db_connection))
                $this->close_database();
        }
    }

    protected function db_query($orig_query)
    {
        $this->db_connect();

	    $args  = func_get_args();
	    $query = array_shift($args);
	    $query = str_replace("?", "%s", str_replace("%", "%%", $query));

	    foreach($args as &$arg)
	        $arg = mysql_real_escape_string($arg, $this->_db_connection);

	    array_unshift($args,$query);
	    $query = call_user_func_array('sprintf',$args);
	    $result = @mysql_query($query, $this->_db_connection);
	    if ($result === FALSE)
            $this->fatal_error("mysql: $query\n".mysql_error($this->_db_connection));
	    return $result;
    }

    protected function insert_id()
    {
        return @mysql_insert_id($this->_db_connection);
    }

    protected function db_query_no_die($orig_query)
    {
        $this->db_connect();

	    $args  = func_get_args();
	    $query = array_shift($args);
	    $query = str_replace("?", "%s", str_replace("%", "%%", $query));

	    foreach($args as &$arg)
	        $arg = mysql_real_escape_string($arg, $this->_db_connection);

	    array_unshift($args,$query);
	    $query = call_user_func_array('sprintf',$args);
	    $result = @mysql_query($query, $this->_db_connection);
	    return $result;
    }

    protected function db_delete($query)
    {
        $this->db_query($query);
        return mysql_affected_rows($this->_db_connection);
    }

    protected function escape_string($str)
    {
        $this->db_connect();
        return mysql_real_escape_string($str, $this->_db_connection);
    }

    /*
      PRIVATE
    */
    private function fatal_error($msg)
    {
        // !!! todo
        echo "<pre style='font-size:1.2em;'>"; debug_print_backtrace();
        die($msg);
    }

    private function close_database()
    {
        if ($this->_db_connection != NULL)
        {
            mysql_close($this->_db_connection);
            $this->_db_connection = NULL;
        }
    }
}

class pyfli extends Database
{
    function increment_download_count($link)
    {
        $this->db_query("UPDATE pyfli_files SET download_count=download_count+1 WHERE link='?';", $link);
    }

    function get_link_info($link)
    {
        $result = $this->db_query("SELECT mime_type,redirect_url,content_length,original_filename,local_pathname,UNIX_TIMESTAMP(ts) AS `timestamp` FROM pyfli_files WHERE link='?';", $link);
        if (!$result  ||  !($row = mysql_fetch_assoc($result)))
            return FALSE;
        return $row;
    }

	function get_uploaded_files_list()
	{
		$rows = array();
		$result = $this->db_query("SELECT link,".
								  "redirect_url,".
								  "mime_type,".
								  "content_length,".
								  "original_filename,".
								  "UNIX_TIMESTAMP(ts) as upload_ts,".
								  "download_count,".
								  "floor((unix_timestamp(current_timestamp)-unix_timestamp(ts))/86400) as age_days".
								  " FROM pyfli_files".
								  " ORDER BY upload_ts DESC".
								  " LIMIT 0,200;");
		while ($row = mysql_fetch_assoc($result))
			$rows[] = $row;
		mysql_free_result($result);
		return $rows;
	}

    function hyperlink($text)
    {
        // match protocol://address/path/
        $text = ereg_replace("[a-zA-Z]+://([.]?[a-zA-Z0-9_/-\?&,-])*", "<a target='_blank' href=\"\\0\">\\0</a>", $text);

        // match www.something
        $text = ereg_replace("(^| )(www([.]?[a-zA-Z0-9_/-\?&,-])*)", "\\1<a target='_blank' href=\"http://\\2\">\\2</a>", $text);

        // return $text
        return $text;
    }

    function is_browser_ie()
    {
        return (strstr($_SERVER['HTTP_USER_AGENT'], "MSIE") !== FALSE);
    }

    function is_browser_chrome()
    {
        return (strstr($_SERVER['HTTP_USER_AGENT'], "Chrome") !== FALSE);
    }

    function is_browser_safari()
    {
        return (strstr($_SERVER['HTTP_USER_AGENT'], "Safari") !== FALSE);
    }

    function is_browser_firefox()
    {
        return (strstr($_SERVER['HTTP_USER_AGENT'], "Firefox") !== FALSE);
    }

    function is_browser_opera()
    {
        return (strstr($_SERVER['HTTP_USER_AGENT'], "Opera") !== FALSE);
    }

    function is_mobile_device()
    {
        // from http://detectmobilebrowser.com/
        $useragent = $_SERVER['HTTP_USER_AGENT'];
        $this->mobile_device = false;
        if (preg_match('/android|avantgo|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|symbian|treo|up\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino/i',$useragent)||preg_match('/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|e\-|e\/|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(di|rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|xda(\-|2|g)|yas\-|your|zeto|zte\-/i',substr($useragent,0,4)))
            $this->mobile_device = true;
        return $this->mobile_device;
    }

    function time_ago($timestamp)
    {
	    $difference = time() - $timestamp;

	    if ($difference < 60)
	    {
		    $text = $difference . " seconds ago";
	    }
	    else
	    {
		    $difference = round($difference / 60);
		    if ($difference < 60)
		    {
			    $text = $difference . " minute";
			    if ($difference <> 1)
				    $text = $text . "s";
			    $text = $text . " ago";
		    }
		    else
		    {
			    $difference = round($difference / 60);
			    if ($difference < 24)
			    {
				    $text = $difference . " hour";
				    if ($difference <> 1)
					    $text = $text . "s";
				    $text = $text . " ago";
			    }
			    else
			    {
				    $difference = round($difference / 24);
				    if ($difference < 7)
				    {
				    	if ($difference == 1)
					    	$text = "yesterday";
				    	else
					    	$text = $difference . " days ago";
				    }
				    else
				    {
					    if (0)
					    {
						    $difference = round($difference / 7);
						    $text = $difference . " week";
						    if ($difference <> 1)
							    $text = $text . "s";
						    $text = $text . " ago";
					    }
					    else
						    $text = "on " . date("d-M-y G:i", $timestamp);
				    }
			    }
		    }
	    }
	    return $text;
    }

    function send_tweets()
    {
        $result = $this->db_query("SELECT user_id,tweet_text,id,f.tweet_screen_name,n.oauth_token,n.oauth_token_secret".
                                  " FROM pyfli_files f, pyfli_social_networks n".
                                  " WHERE f.tweet_screen_name=n.screen_name;");

        while ($result  &&  ($row = mysql_fetch_row($result)))
        {
            $access_token = array();
            $access_token['screen_name']        = $row[3];
            $access_token['oauth_token']        = $row[4];
            $access_token['oauth_token_secret'] = $row[5];
            $twitter = new PyfliTwitter;
            $twitter->login($access_token);

            if (!($_SERVER["SERVER_NAME"] == 'localhost'  ||  substr($_SERVER["SERVER_NAME"],0,12) == '192.168.123.'))
                if ($twitter->post_message($row[1]))
                    $this->db_query("UPDATE pyfli_files SET tweet_screen_name=NULL WHERE id={$row[2]};");
        }
    }
}

?>