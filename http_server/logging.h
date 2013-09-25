//!!!TODO
#include <sstream>
#include <iostream>

#if 0
#define LOG std::cout << this << ": " << std::setw(5) << GetCurrentThreadId() << "  "
#else

class log_line;
class logging
{
  public:
    logging(char const *path);
    ~logging();

    void close(void);
    void flusher(void);
    void log_to_console(void);
    void new_file(void);

    template<typename T>
    logging const &operator <<(T text) const
    {
        return *this;
    }

    logging &operator<<(std::ostringstream &line);
    bool const flush(bool use_lock=true);

  private:
    struct Data;
    Data *data;
};

extern logging logger;

class log_line
{
  public:
    ~log_line()
    {
        logger << line_;
    }

    template<typename T>
    log_line const &operator <<(T text) const
    {
        line_ << text;
        return *this;
    }

  private:
    mutable std::ostringstream line_;
};

#define LOG     log_line() << "          " << std::setw(5) << GetCurrentThreadId() << "  "
#define LOG_MEM log_line() << this << ": " << std::setw(5) << GetCurrentThreadId() << "  "

#endif
