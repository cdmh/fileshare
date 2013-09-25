#include <windows.h>
#include <assert.h>
#include <mysql.h>
#include <string>
#include <sstream>
#include "header.hpp"
#include "mysql_access.h"
#include "boost/lexical_cast.hpp"

#pragma comment(lib,"libmysql.lib")

namespace fileshare {

struct mysql::data
{
    struct column_data
    {
        column_data() : error(false), isnull(false), type(UNKNOWN_COLUMN_TYPE)
        {
        }

        my_bool         error;
        my_bool         isnull;
        std::string     column_name;
        std::string     parameter;
        std::string     string_value;
        column_type_t   type;
    };

    MYSQL             *db_connection;
    unsigned int       num_columns;
    std::string const  server;
    std::string const  schema;
    std::string const  username;
    std::string const  password;
    std::string        tablename;
    statement_type_t   statement_type;
    column_source_t    column_source;
    column_type_t      column_type;
    column_data       *columns;
    MYSQL_BIND        *bind;
    std::string        sql;

    data(std::string const &srvr,
         std::string const &schma,
         std::string const &user,
         std::string const &pword)
      : db_connection(0), num_columns(0), server(srvr), schema(schma), username(user), password(pword),
        statement_type(UNKNOWN_STATEMENT_TYPE),
        column_source(UNKNOWN_SOURCE),
        column_type(UNKNOWN_COLUMN_TYPE),
        columns(0),
        bind(0)
    {
    }

    ~data()
    {
        if (db_connection)
        {
	        mysql_close(db_connection);
	        db_connection = 0;
	    }

        delete[] columns;
        delete[] bind;
    }
};

mysql::mysql(std::string const &server,
             std::string const &schema,
             std::string const &username,
             std::string const &password)
  : data_(new data(server, schema, username, password))
{
    //TRACE(L"0x%p %hs ctor - this connector has no configuration\n", this, DBG_CURRENT_FUNCTION);
}

mysql::~mysql()
{
    //TRACE(L"0x%p %hs dtor\n", this, DBG_CURRENT_FUNCTION);
    delete data_;
    data_ = 0;
}

void mysql::set_number_of_columns(unsigned int count)
{
    data_->num_columns = count;
    data_->columns = new data::column_data[count];
    data_->bind    = new MYSQL_BIND[count];
    memset(data_->bind, 0, sizeof(MYSQL_BIND) * count);
    for (unsigned int loop=0; loop<data_->num_columns; ++loop)
    {
        data_->bind[loop].is_null = &data_->columns[loop].isnull;
        data_->bind[loop].error   = &data_->columns[loop].error;
        data_->bind[loop].length  = &data_->bind[loop].length_value;
    }
}

void mysql::set_type(std::string const &table, statement_type_t type)
{
    data_->statement_type = type;
    data_->tablename = table;

    if (data_->tablename.empty())
        throw std::runtime_error("MySQL Connector - missing database tablename");
}

void mysql::set_column(int column, std::string const &name, column_type_t type, column_source_t source, std::string const &parameter)
{
    data_->columns[column].type = type;
    data_->columns[column].column_name = name;
    data_->columns[column].parameter = parameter;
    switch (type)
    {
        case STRING:    data_->bind[column].buffer_type = MYSQL_TYPE_STRING;    break;
        case INTEGER:   data_->bind[column].buffer_type = MYSQL_TYPE_LONG;      break;
        case BLOB:      data_->bind[column].buffer_type = MYSQL_TYPE_LONG_BLOB; break;
        case TIMESTAMP: data_->bind[column].buffer_type = MYSQL_TYPE_TIMESTAMP; break;
        default:        assert(L"Unknown column type");
    }

    if (column == data_->num_columns-1)
    {
        std::ostringstream sql;

        if (data_->statement_type == INSERT)
            sql << L"INSERT INTO " << data_->tablename << L" (";
        else if (data_->statement_type == SELECT)
            sql << L"SELECT ";

        for (unsigned int loop=0; loop<data_->num_columns; ++loop)
        {
            if (loop > 0)
                sql << L",";
            sql << L"`" << data_->columns[loop].column_name << L"`";
        }

        if (data_->statement_type == INSERT)
        {
            sql << L") VALUES (";

            for (unsigned int loop=0; loop<data_->num_columns; ++loop)
            {
                if (loop > 0)
                    sql << L",";
                sql << L"?";
            }
            sql << L");";
        }
        else if (data_->statement_type == SELECT)
            sql << L" FROM " << data_->tablename;

        data_->sql = sql.str();
        //TRACE(L"%s\n", sql.str().c_str());
    }
}

void mysql::close(void)
{
    if (data_->db_connection)
    {
	    mysql_close(data_->db_connection);
	    data_->db_connection = 0;
	}
}

char const *mysql::error(void) const
{
    //TRACE(L"Database Error: %hs\n", mysql_error(data_->db_connection));
    if (!data_->db_connection  ||  mysql_ping(data_->db_connection) == 0)
        return "No Database Connection";
    return mysql_error(data_->db_connection);
}

bool const mysql::execute_sql(std::string const &sql)
{
    if (!data_->db_connection  ||  mysql_ping(data_->db_connection) != 0)
    {
        //TRACE(L"Lost connection to database, mysql_ping() failed to reconnect\n");
        this->close();
        if (!this->open())
            return false;
    }

    //TRACE(L"SQL: %s\n", sql.c_str());
    bool const retval = (mysql_query(data_->db_connection, sql.c_str()) == 0);
    //if (!retval)
    //    TRACE(L"SQL failed to execute: %hs", error());
    return retval;
}

bool const mysql::open(void)
{
    std::string host(data_->server);
    std::string db(data_->schema);
    std::string username(data_->username);
    std::string password(data_->password);

    data_->db_connection = mysql_init(0);
    if (!mysql_real_connect(data_->db_connection, host.c_str(), username.c_str(), password.c_str(), db.c_str(), MYSQL_PORT, NULL, 0))
    {
        //TRACE(L"Unable to connect to the database: %hs\n", error());
        return false;
    }

    if (mysql_select_db(data_->db_connection, db.c_str()) < 0)
    {
        //TRACE(L"Unable to select database: %hs\n", error());
        return false;
    }

    return true;
}

std::string const mysql::escape_string(std::string const &from)
{
    if (!data_->db_connection  &&  !this->open())
        throw std::runtime_error("Cannot connect to MySQL database");
    else if (data_->db_connection  &&  mysql_ping(data_->db_connection) != 0)
    {
        LOG_MEM << "Lost connection to database, mysql_ping() failed to reconnect";
        this->close();
        if (!this->open())
            return false;
    }

    unsigned const len = (from.length() << 1);
    std::auto_ptr<char> to(new char[len]);
    memset(to.get(), 0, len);
    mysql_real_escape_string(data_->db_connection, to.get(), from.c_str(), from.length());
    return to.get();
}

boost::uint64_t const mysql::insert_id(void)
{
    if (data_->db_connection  &&  mysql_ping(data_->db_connection) != 0)
    {
        this->close();
        return false;
    }

    return mysql_insert_id(data_->db_connection);
}

template<unsigned Base, typename T>
std::string const from_base_10(T v)
{
    char const digits[] = "QfgbTqrGmstjkShlBFZzCpDnyXY0J1vw3x2M75d6L8cR9HKNPVW4";
    char result[sizeof(T)*CHAR_BIT + 1];
    char* current = result + sizeof(result);
    *--current = '\0';

    while (v != 0) {
        v--;
        *--current = digits[v % Base];
        v /= Base;
    }
    return current;
}

std::string const store_file_details(std::string     const &mime_type,
                                     boost::uint64_t const &content_length,
                                     std::string     const &original_filename,
                                     std::string     const &local_pathname,
                                     std::string     const &redirect_url,
                                     std::string     const &twitter_screen_name,
                                     std::string     const &tweet_text)
{
    std::string const
    link(
        boost::lexical_cast<std::string>(from_base_10<52>(time(0)-1296169281)) +
        boost::lexical_cast<std::string>(from_base_10<52>(GetCurrentThreadId())));

#ifdef _DEBUG
    mysql ms("localhost", "pyfli", "pyfli", "PYFl1");
#elif CENTRIX_SOFTWARE
    mysql ms("localhost", "pyfli", "pyfli", "M00nshine");
#else
    //mysql ms("213.171.200.51", "whereilive", "whereilive", "3v3ex1201");
    mysql ms("localhost", "pyfli", "pyfli", "pyf.li");
#endif

    try
    {
#ifdef _DEBUG
        std::string url(" http://localhost/"+link);
#elif CENTRIX_SOFTWARE
        std::string url(" http://195.224.113.203/"+link);
#else
        std::string url(" http://pyf.li/"+link);
#endif

        std::string tweet;
        if (tweet_text.length() > 140-url.length())
            tweet = std::string(tweet_text.c_str(), 140-url.length());
        else
            tweet = tweet_text;
        tweet += url;

        std::string sql("INSERT INTO pyfli_files (link,redirect_url,mime_type,content_length,original_filename,local_pathname");
        if (twitter_screen_name.length() > 0)
            sql += ",tweet_screen_name,tweet_text";
        sql += ") VALUES ("
            "'" + link + "'"
            ",'" + ms.escape_string(redirect_url) + "'"
            ",'" + ms.escape_string(mime_type) + "'"
            ",'" + boost::lexical_cast<std::string>(content_length) + "'"
            ",'" + ms.escape_string(original_filename) + "'"
            ",'" + ms.escape_string(local_pathname) + "'";
        if (twitter_screen_name.length() > 0)
        {
            sql += ",'" + ms.escape_string(twitter_screen_name) + "'";
            sql += ",'" + ms.escape_string(tweet) + "'";
        }
        sql += ");";

        if (!ms.execute_sql(sql))
        {
//!!!
            LOG << "*** MySQL error: " << ms.error();
            return std::string();
        }
    }
    catch (std::exception const &e)
    {
//!!!
        std::string err = e.what();
        LOG << "*** SQL execute failed: " << err;
        return std::string();
    }

    return link;
}

// once the upload is complete, we set the redirect_url to NULL so
// the PHP code knows it is to delivered directly from the file
// system, not through this web service
bool const on_upload_complete(std::string const &link)
{
#ifdef _DEBUG
    mysql ms("localhost", "pyfli", "pyfli", "PYFl1");
#elif CENTRIX_SOFTWARE
    mysql ms("localhost", "pyfli", "pyfli", "M00nshine");
#else
    mysql ms("213.171.200.51", "whereilive", "whereilive", "3v3ex1201");
#endif

    try
    {
        std::string sql("UPDATE pyfli_files SET redirect_url=NULL WHERE link='" + link + "';");
        if (!ms.execute_sql(sql))
        {
//!!!
            LOG << "*** MySQL error: " << ms.error();
            return false;
        }
    }
    catch (std::exception const &e)
    {
//!!!
        std::string err = e.what();
        LOG << "*** SQL execute failed: " << err;
        return false;
    }
    return true;
}

bool const delete_download(std::string const &link)
{
#ifdef _DEBUG
    mysql ms("localhost", "pyfli", "pyfli", "PYFl1");
#elif CENTRIX_SOFTWARE
    mysql ms("localhost", "pyfli", "pyfli", "M00nshine");
#else
    mysql ms("213.171.200.51", "whereilive", "whereilive", "3v3ex1201");
#endif

    try
    {
        std::string sql("DELETE FROM pyfli_files WHERE link='" + link + "';");
        if (!ms.execute_sql(sql))
        {
//!!!
            LOG << "*** MySQL error: " << ms.error();
            return false;
        }
    }
    catch (std::exception const &e)
    {
//!!!
        std::string err = e.what();
        LOG << "*** SQL execute failed: " << err;
        return false;
    }
    return true;
}


}   // namespace fileshare
