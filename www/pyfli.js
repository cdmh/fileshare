function create_xhr()
{
    var xhr;
    if (window.XMLHttpRequest)
        xhr = new XMLHttpRequest;
    else if (window.ActiveXObject)
    {
        try
        {
            xhr = new ActiveXObject("Microsoft.XMLHTTP");
        }
        catch (e)
        {
        }
    }
    return xhr;
}

var first_time = true;
function update_progress()
{
    var xhr = create_xhr();
    var id = document.getElementById('id').value;
    xhr.open('GET', './?query_progress='+id, true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4)
        {
            var success  = false;
            var finished = false;
            var fname = document.getElementById('file').value.match(/[^\/\\]+$/);
            if (xhr.status == 200)
            {
                document.getElementById('DEBUG').innerHTML = xhr.responseText;
                try
                {
                    var j = JSON.parse(xhr.responseText);

                    if (j.error == 0)
                    {
                        document.getElementById('pcent').style.width = j.percent_complete+'%';
                        if (j.percent_complete == 100)
                        {
                            finished = true;
                            document.getElementById('uploading').innerHTML = 0;
                            document.getElementById('pcent2').innerHTML = fname + ' upload complete.';
                            document.getElementById('file').value = '';
                            document.getElementById('go_again').style.display = 'block';
							if (document.getElementById('tweet'))
							{
								document.getElementById('lets_go').innerHTML = 'Tweet again?';
								document.getElementById('tweet').value = '';
								document.getElementById('tweet').focus();
								update_tweet();
							}
							else
								document.getElementById('lets_go').innerHTML = 'Want to share another file?';
						}
                        else
                            document.getElementById('pcent2').innerHTML = 'Uploading ' + fname + ' ... ' +j.percent_complete+'%';
                        if (j.link)
						{
                            document.getElementById('download').style.display = 'block';

                            var links;
                            links = "<a target='_blank' href='mailto:?subject="+fname+"&body=Here is a link to download "+fname+" via pyf.li:%0A" + j.link + "'>email this link</a>";
                            links += "<a href='" + j.link + "' target='_blank'>" + j.link + "</a>";
                            document.getElementById('download_link').innerHTML = links;

							if (first_time)
							{
								document.getElementById('copy').innerHTML =
									"<object classid='clsid:d27cdb6e-ae6d-11cf-96b8-444553540000' width='110' height='14' id='clippy'>"
									+ "<param name='movie' value='clippy.swf'/>"
									+ "<param name='allowScriptAccess' value='always' />"
									+ "<param name='quality' value='high' />"
									+ "<param name='scale' value='noscale' />"
									+ "<param name='FlashVars' value='text=" + j.link + "'>"
									+ "<param name='bgcolor' value='#44f'>"
									+ "<embed src='clippy.swf'"
									+ "width='110'"
									+ "height='14'"
									+ "name='clippy'"
									+ "quality='high'"
									+ "allowScriptAccess='always'"
									+ "type='application/x-shockwave-flash'"
									+ "pluginspage='http://www.macromedia.com/go/getflashplayer'"
									+ "FlashVars='text=" + j.link + "'"
									+ "bgcolor='#44f'"
									+ "/></object>";
							}
						}

                        first_time = false;
                        success = true;
                    }
                }
                catch (e)
                {
                    success = false;
                }
            }

            if (!success)
            {
                document.getElementById('pcent2').innerHTML = 'Error uploading ' + fname;
                document.getElementById('pcent2').style.color = 'red';
                document.getElementById('download').style.display = 'none';
                document.getElementById('failed').style.display = 'block';
				if (!document.getElementById('tweet'))
                    document.getElementById('go_again').style.display = 'block';
            }

            if (success && !finished)
                setTimeout('update_progress();',1000);
        }
    };
    xhr.send();
}

function canunload()
{
    if (document.getElementById('uploading').innerHTML == 1)
        return 'Warning! Leaving this page will cancel the file sharing and the link will not work.';
}

function upload_file()
{
    set_id();

    // reset UI
    document.getElementById('pcent').style.width = '0%';
    document.getElementById('pcent2').innerHTML = '';
    document.getElementById('download').style.display = 'none';
    document.getElementById('failed').style.display = 'none';
    document.getElementById('pcent2').style.color = '#000';

    // update the UI for upload in progress
    document.getElementById('uploading').innerHTML = 1;
    document.getElementById('file_upload_form').submit();
    document.getElementById('instructions').style.display = 'none';
    document.getElementById('go_again').style.display = 'none';
    document.getElementById('progress').style.display = 'block';
    document.getElementById('pcent2').innerHTML = 'Preparing upload';
    setTimeout('update_progress();',500);
}

function set_id()
{
    var dt = new Date();
    document.getElementById('id').value = dt.valueOf() + (((1+Math.random())*0x10000)|0).toString(16).substring(1);
}

function init()
{
    document.getElementById('file_upload_form').target = 'upload_target';
	if (document.getElementById('tweet'))
	{
		document.getElementById('tweet').focus();
		update_tweet();
	}
}

function update_tweet()
{
    var tweet_text = document.getElementById('tweet');
	var len = 115 - tweet_text.value.length;
	var tweet_text_len = document.getElementById('tweetlen');
	tweet_text_len.innerHTML = len;
	if (len < 0)
		tweet_text_len.className = 'toolong';
	else
		tweet_text_len.className = '';

	if (len == 115  ||  len < 0)
		document.getElementById('file').disabled = true;
	else
		document.getElementById('file').disabled = false;
}