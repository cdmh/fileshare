#include "stdafx.h"
#include <fstream>
#include "../http_server/mime_types.hpp"

typedef std::iterator_traits<char const *>::difference_type length_t;
typedef std::pair<length_t, char const *>      string_t;

template <>
inline bool std::less<string_t>::operator()(string_t const &_Left, string_t const &_Right) const
{
    return (strnicmp(_Left.second, _Right.second, std::min(_Left.first, _Right.first)) < 0);
}

template <>
inline bool std::operator==(string_t const &_Left, string_t const &_Right)
{
    return (_Left.first == _Right.first  &&  strnicmp(_Left.second, _Right.second, _Left.first) == 0);
}

template <>
inline bool std::operator!=(string_t const &_Left, string_t const &_Right)
{
    return !(_Left == _Right);
}

// not a specialisation as the parameter types are different
inline bool operator==(string_t const &_Left, char const * const _Right)
{
    string_t::first_type const len = strlen(_Right);
    return (_Left.first == len  &&  strnicmp(_Left.second, _Right, len) == 0);
}

inline bool operator!=(string_t const &_Left, char const * const _Right)
{
    return !(_Left == _Right);
}

namespace utils {

template<typename It>
bool const find(It &it, It const ite, char ch)
{
    while (it != ite  &&  *it != ch)
        ++it;
    return it != ite;
}

template<typename It>
bool const is_whitespace(It const &it)
{
    return (*it == ' '  ||  *it == '\t'  ||  *it == '\r'  ||  *it == '\n');
}

template<typename It>
bool const skip_whitespace(It &it)
{
    while (*it  &&  is_whitespace(it))
        ++it;
    return *it != 0;
}

template<typename It>
bool const skip_whitespace(It &it, It const ite)
{
    while (it != ite  &&  is_whitespace(it))
        ++it;
    return it != ite;
}

}   // namespace utils

class http_query_server : boost::noncopyable
{
  private:
    typedef std::map<string_t,string_t> params_t;
    static unsigned const max_objects = 100;

  public:
    bool const http_query(
        http::server3::headers_t const &request_headers,
        std::string              const &query_string,
        http::server3::headers_t       &response_headers,
        std::string                    &response,
        boost::shared_ptr<http::server3::connection> const &conn)
    {
        DBG_SCOPE_MEM_FN;

        params_t params;
        decode_query_string(query_string, params);

        params_t::iterator param = params.find(string_t(4,"form"));
        if (param != params.end()  &&  param->second == "submit")
        {
/*
            response = "<html><body>";
            response += "<pre>";
            for (http::server3::headers_t::const_iterator it=request_headers.begin(); it!=request_headers.end(); ++it)
                response += it->name + '.' + it->value + "<br>";
            response += "</pre>";
            response += "<hr><pre>" + query_string + "</pre>";
            response += "</body></html>";
*/
            response_headers.push_back(http::server3::headers_t::value_type("Content-Type", "text/html"));
        }
        else if ((param=params.find(string_t(2,"dl"))) != params.end())
        {
            boost::shared_ptr<http::server3::request> req;
            std::string const id(param->second.second,param->second.first);

            LOG_MEM << "Download request received for id=" << id;

            if (!http::server3::request::get_request(id, req))
            {
                LOG_MEM << __FILE__ << " (" << __LINE__ << ") Bad id: " << id;
                return bad_request(response_headers, response);
            }

            std::string local_filename(req->filename());

            HANDLE file = CreateFileA(local_filename.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (file == INVALID_HANDLE_VALUE)
            {
                LOG_MEM << __FILE__ << " (" << __LINE__ << ") Unable to open local file: Error " << GetLastError();
                return bad_request(response_headers, response);
            }

            std::string     const original_filename = req->uploaded_file_original_filename;
            std::string     const mime_type         = req->uploaded_file_mime_type;
            boost::uint64_t const file_length       = req->uploaded_file_length();

            using http::server3::reply;
            reply rep;
            rep.status = reply::ok;
            rep.headers.push_back(http::server3::header("Content-Length", boost::lexical_cast<std::string>(file_length)));
            rep.headers.push_back(http::server3::header("Content-Type", mime_type));
            rep.headers.push_back(http::server3::header("Content-Disposition", "attachment; filename=\""+original_filename+"\""));
            conn->sync_send_data_buffer(rep.to_buffers());
            conn->sync_send_data(rep.content.c_str(), rep.content.length());

            short last_reported = 0;
            boost::uint64_t total_bytes = 0;
            while (total_bytes != file_length)
            {
                DWORD bytes_read;
                char buffer[1024*100];

                if (!http::server3::request::is_valid_request(id))
                    break;

                // don't read past the end of the file
                DWORD hi = 0;
                DWORD lo = GetFileSize(file, &hi);
                boost::uint64_t current_size = hi;
                current_size = (current_size << 32) | lo;

                #undef max
                DWORD const to_read = static_cast<DWORD>(std::min<boost::uint64_t>(sizeof(buffer),file_length-total_bytes));
                if (ReadFile(file,buffer,to_read,&bytes_read,NULL)  &&  bytes_read > 0)
                {
                    total_bytes += bytes_read;
                    if (conn->sync_send_data(buffer, bytes_read) != bytes_read)
                        return false;

                    short const pcent = static_cast<short>((total_bytes*100)/file_length);
                    if (pcent-last_reported >= 10)
                    {
                        LOG_MEM << "Downloaded " << bytes_read << " bytes. "
                            << total_bytes << " of "
                            << file_length << "\t"
                            << pcent << "%.";
                        last_reported = pcent;
                    }
                }
                else
                {
                    // we are ahead of the upload, so wait a while
                    Sleep(100);
                }
            }
            CloseHandle(file);

            if (total_bytes == file_length)
                LOG_MEM << "Download request for id=" << id << " complete.";
            else
                LOG_MEM << "Download request for id=" << id << " aborted.";

            response_headers.clear();
            response.clear();
            return false;
        }
        else if ((param=params.find(string_t(5,"query_progress"))) != params.end())
        {
            std::string const id(std::string(param->second.second,param->second.first));
            short pcent = 0;
            short error = 0;
            boost::shared_ptr<http::server3::request> req;
            short tries = 0;
            while (++tries <= 3  &&  !req.get())
            {
                if (http::server3::request::get_request(id, req))
                    pcent = req->upload_percent_complete();
                else
                    Sleep(500);
            }

            if (!req.get())
            {
                LOG_MEM << __FILE__ << " (" << __LINE__ << ") Bad id: " << id;
                error = 1;
            }

#ifndef NDEBUG
            std::string const url = "http://localhost:8085/";
#elif CENTRIX_SOFTWARE
            std::string const url(" http://195.224.113.203/");
#else
            std::string const url = "http://pyf.li/";
#endif

            std::string const link = ((req.get() && req->link().length()>0)? (",\"link\":\"" + url + req->link() + "\"") : "");
            response = "{\"version\":1"
                       ",\"error\":\"" + boost::lexical_cast<std::string>(error) + "\""
                       ",\"id\":\"" + id + "\""
                     + link
                     + ", \"percent_complete\":"
                     + boost::lexical_cast<std::string>(pcent) + "}";
            response_headers.push_back(
                http::server3::headers_t::value_type(
                    "Content-Type", "plain/text"));
        }

        response_headers.push_back(
            http::server3::headers_t::value_type(
                "Content-Length",
                boost::lexical_cast<std::string>(response.size())));

        return false;
    }

  private:
    bool const bad_request(http::server3::headers_t &response_headers,
                           std::string              &response) const
    {
        using http::server3::reply;
        response_headers.push_back(http::server3::headers_t::value_type("Content-Type", "text/html"));
        reply rep = reply::stock_reply(reply::bad_request);
        response         = rep.content;
        response_headers = rep.headers;
        return false;
    }

    bool const get_cookie_value(char const *name, http::server3::headers_t const &request_headers, std::string &value)
    {
        DBG_SCOPE_MEM_FN;

        std::string value_list;
        for (http::server3::headers_t::const_iterator it1=request_headers.begin(); it1!=request_headers.end(); ++it1)
        {
            if (stricmp(it1->name.c_str(),"cookie") == 0)
                value_list = it1->value;
        }

        if (value_list.empty())
            return false;

        size_t                      const len = strlen(name);
        std::string::const_iterator       it  = value_list.begin();
        std::string::const_iterator const ite = value_list.end();
        while (it!=value_list.end())
        {
            if (utils::skip_whitespace(it,ite))
            {
                if (strnicmp(&*it, name, len) == 0)
                {
                    it += len;
                    if (utils::skip_whitespace(it,ite))
                    {
                        if (*it++ == '=')
                        {
                            std::string::const_iterator start = it;
                            utils::find(it,ite,';');
                            value.clear();
                            std::copy(start,it,std::back_inserter(value));
                            return true;
                        }
                        else
                        {
                            if (utils::find(it,ite,';'))
                                ++it;
                        }
                    }
                    else
                        return false;
                }
                else if (utils::find(it,ite,';'))
                    ++it;
            }
        }

        return false;
    }

    void decode_query_string(std::string const &query_string, params_t &params)
    {
        DBG_SCOPE_MEM_FN;

        assert(strnicmp(query_string.c_str(),"/?",2) == 0);

        char const *it  = query_string.c_str();
        char const *ite = it + query_string.length();
        it += 2;
        while (it!=ite)
        {
            while (*it == '&')
                ++it;

            string_t name;
            name.second = it;
            char const *next = it;
            utils::find(next,ite,'&');
            utils::find(it,next,'=');
            name.first = std::distance(name.second,it);

            string_t value;
            if (it != ite  &&  *it != '&')
            {
                value.second = ++it;
                utils::find(it,ite,'&');
                value.first = std::distance(value.second,it);
            }

            params.insert(std::make_pair(name,value));
            if (it != ite)
                ++it;
        }
    }
};




namespace {

bool const initialise_sockets(void)
{
    DBG_SCOPE_FN;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    if (LOBYTE(wsaData.wVersion) != 2  ||  HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();
//        throw std::runtime_error("Unable to initialise WinSock 2.2");
        return false;
    }

    char hostname[512] = { 0 };
    gethostname(hostname, sizeof(hostname)/sizeof(hostname[0]));

    char computername[512] = { 0 };
    DWORD size = sizeof(computername)/sizeof(computername[0]);
    GetComputerName(computername, &size);

    TRACE("Initialised WinSock\n", wsaData.wVersion, wsaData.wHighVersion);
    TRACE("  Description   : %hs\n", wsaData.szDescription);
    TRACE("  System Status : %hs\n", wsaData.szSystemStatus);
    TRACE("  Host Name     : \"%hs\"\n", hostname);
    TRACE("  Computer Name : \"%s\"\n", computername);
    return true;
}
boost::function0<void> shutdown_httpd;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    DBG_SCOPE_FN;
    
    switch (ctrl_type)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            LOG << "Shutdown signalled from keyboard...";
            shutdown_httpd();
            return TRUE;

        default:
            return FALSE;
    }
}

void httpd(http::server3::http_query_callback http_query)
{
    DBG_SCOPE_FN;

    try
    {
        // Initialise server
        char const * const doc_root = ".";
        http::server3::server httpd("0.0.0.0", "8089", doc_root, 10, http_query);

        // Set console control handler to allow server to be stopped.
        shutdown_httpd = boost::bind(&http::server3::server::stop, &httpd);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

        // Run the server until stopped.
        httpd.run();
    }
    catch (std::exception& e)
    {
        LOG << "exception: " << e.what();
    }
}

}   // anonymous namespace

logging logger("./logs");

int _tmain(int argc, _TCHAR* argv[])
{
#ifndef NDEBUG
//    _CrtSetBreakAlloc(747);
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

    LOG << "pyf.li server starting";
#if CENTRIX_SOFTWARE
    LOG << "pyf.li server licensed to Centrix Software";
#endif

    if (argc == 2 && stricmp(argv[1],"/stdout") == 0)
    {
        LOG << "Logging to console & logfile";
        logger.log_to_console();
    }
    else
    {
        std::cerr << "Use /stdout command line switch to log to console.";
        LOG << "Logging to logfile. Use /stdout command line switch to log to console too.";
    }

    srand((unsigned)time(0));
    initialise_sockets();

    // start the HTTP daemon thread
    void httpd(http::server3::http_query_callback);

    http_query_server   query_server;
    boost::thread_group threads;
    threads.create_thread(
        boost::bind(
            httpd,
            http::server3::http_query_callback(
                boost::bind(&http_query_server::http_query, &query_server, _1, _2, _3, _4, _5))));

    threads.join_all();

    LOG << "pyf.li server shutting down...";

    http::server3::request::clear_requests();
    logger.close();
    LOG << "pyf.li server terminated.";

	return 0;
}
