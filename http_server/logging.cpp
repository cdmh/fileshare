#include <sstream>
#include <iostream>
#include <iomanip>
#include <list>
#include <windows.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include "logging.h"
#include "platform.h"
#include <io.h>     // _access

struct logging::Data
{
    Data() : running_(true) { }
    volatile bool           running_;
    HANDLE                  file_;
    boost::filesystem::path path_;
    boost::mutex            lock_;
    std::list<std::string>  lines_;
    unsigned                line_count_;
    boost::thread          *thread_;
    bool                    log_to_console_;

    static unsigned const max_lines = 2500000;
};

logging::logging(char const *path)
{
    data = new Data;
    data->path_ = absolute(boost::filesystem::path(path));
    data->line_count_ = 0;
    data->log_to_console_ = false;

    data->file_ = INVALID_HANDLE_VALUE;
    this->new_file();
    data->thread_ = new boost::thread(boost::bind(&logging::flusher, this));
}

logging::~logging()
{
    data->thread_->join();
    CloseHandle(data->file_);
    delete data;
}

void logging::log_to_console(void)
{
    data->log_to_console_ = true;
}

void logging::new_file(void)
{
    if (data->file_ != INVALID_HANDLE_VALUE)
        CloseHandle(data->file_);

    boost::filesystem::path path;
    for (int index=1; path.empty()  ||  (exists(path)  &&  file_size(path) > Data::max_lines*80); ++index)
    {
        std::ostringstream builder;
        builder << "seq_" << std::setw(3) << std::setfill('0') << index << ".log";

        path = data->path_ / builder.str();
    }
    data->file_ = CreateFileA(path.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (data->file_ == INVALID_HANDLE_VALUE)
        std::cerr << "Unable to open log file " << path << "\n";
    else
        std::cerr << "Logging to file " << path << "\n";
}

void logging::flusher(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    while (data->running_)
    {
        flush();
        Sleep(5000);
    }
    flush();
}

void logging::close(void)
{
    data->running_ = false;
}

logging &logging::operator<<(std::ostringstream &line)
{
    time_t current_time = time(0);
    tm *ctm = localtime(&current_time);

    std::ostringstream output;
    output << std::setfill('0')
           << ctm->tm_year+1900 << '-'
           << std::setw(2) << ctm->tm_mon+1 << '-'
           << std::setw(2) << ctm->tm_mday << ' '
           << std::setw(2) << ctm->tm_hour << ':'
           << std::setw(2) << ctm->tm_min << ':'
           << std::setw(2) << ctm->tm_sec << " :"
           << line.str() << "\r\n";

    boost::mutex::scoped_lock lock(data->lock_);
    data->lines_.push_back(output.str());
    return *this;
}

bool const logging::flush(bool use_lock)
{
    std::auto_ptr<boost::mutex::scoped_lock> lock;
    if (use_lock)
        lock.reset(new boost::mutex::scoped_lock);

    for (std::list<std::string>::const_iterator it=data->lines_.begin(); it!=data->lines_.end(); )
    {
        DWORD written;
        if (!WriteFile(data->file_, it->c_str(), it->length(), &written, NULL)  ||  written != it->length())
            return false;

        if (data->log_to_console_)
            std::cout << *it;

        ++it;
        data->lines_.pop_front();

        if (++data->line_count_ >= Data::max_lines)
        {
            data->line_count_ = 0;
            this->new_file();
        }
    }
    return true;
}
