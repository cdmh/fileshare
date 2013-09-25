<?php

require_once('twitteroauth/twitteroauth.php');

/*
 Callback URL: http://pyf.li/?cb=1
 Customer Key: f6UWlKMhtvRUjFPBE4FA
 Consumer secret: aZI6VXXAMl1vb0Bcq6fT3K85yb1i4Aipnws4nmc
 Request token URL: https://api.twitter.com/oauth/request_token
 Access token URL:  https://api.twitter.com/oauth/access_token
 Authorize URL:     https://api.twitter.com/oauth/authorize                    // hmac-sha1 signatures

 PHP API Library:   http://github.com/abraham/twitteroauth
*/
class PyfliTwitter
{
    private $key = 'f6UWlKMhtvRUjFPBE4FA';
    private $secret = 'aZI6VXXAMl1vb0Bcq6fT3K85yb1i4Aipnws4nmc';
    private $request_url = 'https://api.twitter.com/oauth/request_token';
    private $access_token_url = 'https://api.twitter.com/oauth/access_token';

    private $access_token = null;

    function connect($uid)
    {
        // Build TwitterOAuth object with client credentials
        $connection = new TwitterOAuth($this->key, $this->secret);
         
        // Get temporary credentials
        $request_token = $connection->getRequestToken("http://{$_SERVER['HTTP_HOST']}{$_SERVER['SCRIPT_NAME']}?twitter_callback=$uid&" . remove_parameter('connect_twitter'));

        // Save temporary credentials to session
        $_SESSION['twitter_oauth_token']        = $request_token['oauth_token'];
        $_SESSION['twitter_oauth_token_secret'] = $request_token['oauth_token_secret'];

        // If last connection failed don't display authorization link
        switch ($connection->http_code)
        {
            case 200:
                // Build authorize URL and redirect user to Twitter
                $url = $connection->getAuthorizeURL($request_token['oauth_token']);
                header('Location: ' . $url);
                return true;
                break;
            default:
                // Show notification if something went wrong
                $_SESSION['error'] = "Unable to connect to Twitter [$connection->http_code].";
        }
        return false;
    }

    function login($tokens)
    {
        $this->access_token = array();
        $this->access_token['screen_name']        = $tokens['screen_name'];
        $this->access_token['oauth_token']        = $tokens['oauth_token'];
        $this->access_token['oauth_token_secret'] = $tokens['oauth_token_secret'];
    }

    function is_connected()
    {
        return ($this->access_token != null);
    }

    function screen_name()
    {
        if ($this->is_connected())
            return $this->access_token['screen_name'];
        return false;
    }

    function get_access_token()
    {
        return $this->access_token;
    }

    function post_message($msg)
    {
        // Create a TwitterOauth object with consumer/user tokens
        $connection = new TwitterOAuth(
            $this->key,
            $this->secret,
            $this->access_token['oauth_token'],
            $this->access_token['oauth_token_secret']);
    
        return $connection->post('statuses/update', array('status' => $msg));
    }

    function get_connect_html()
    {
        global $root;
        if ($this->is_connected())
        {
        	$html = "<img src='./images/twitter-small.png' width='20' alt='' />" .
                    "<a target='_blank' href='http://twitter.com/{$this->screen_name()}'>{$this->screen_name()}</a>";
	    }
	    else
	    {
            $html = "<p><a href='?connect_twitter=1'>" .
                    "<img alt='Connect to Twitter' src='./images/twitter-connect-button-small.png' /></a></p>";
        }
        return $html;
    }

    function callback()
    {
        // Create TwitteroAuth object with app key/secret and token key/secret from default phase
        $connection = new TwitterOAuth($this->key, $this->secret, $_SESSION['twitter_oauth_token'], $_SESSION['twitter_oauth_token_secret']);

        // Request access tokens from twitter
        $this->access_token = $connection->getAccessToken($_REQUEST['oauth_verifier']);

        // Remove no longer needed request tokens
        unset($_SESSION['twitter_oauth_token']);
        unset($_SESSION['twitter_oauth_token_secret']);

        return ($connection->http_code == 200);
    }
}

?>