#pragma once
#include "boost/cstdint.hpp"

namespace fileshare {

class mysql
{
  public:
    typedef enum
    {
        UNKNOWN_STATEMENT_TYPE = 0,
        INSERT = 0x100,
        SELECT,
    } statement_type_t;

    typedef enum
    {
        UNKNOWN_COLUMN_TYPE = 0,
        STRING = 0x200,
        INTEGER,
        BLOB,
        TIMESTAMP,
    } column_type_t;

    typedef enum
    {
        UNKNOWN_SOURCE = 0,
        PARAMETER = 0x300,
        DATA,
    } column_source_t;

    mysql(std::string const &server,
          std::string const &schema,
          std::string const &username,
          std::string const &password);
    ~mysql();

    char            const *error(void) const;
    bool            const execute_sql(std::string const &sql);
    boost::uint64_t const insert_id(void);
    std::string     const escape_string(std::string const &from);
    void                  set_number_of_columns(unsigned int count);
    void                  set_type(std::string const &table, statement_type_t type);
    void                  set_column(int column, std::string const &name, column_type_t type, column_source_t source, std::string const &parameter);

  protected:
    mysql(mysql const &);
    mysql &operator=(mysql const &);

    void        close(void);
    bool const  open(void);

  private:
    struct data;
    data *data_;
};

}   // namespace fileshare
