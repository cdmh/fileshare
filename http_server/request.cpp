#include <boost/filesystem.hpp>
#include "request.hpp"

namespace fileshare {
std::string const store_file_details(std::string     const &mime_type,
                                     boost::uint64_t const &content_length,
                                     std::string     const &original_filename,
                                     std::string     const &local_pathname,
                                     std::string     const &redirect_url,
                                     std::string     const &twitter_screen_name,
                                     std::string     const &tweet_text);
bool const delete_download(std::string const &link);
bool const on_upload_complete(std::string const &link);
}

namespace http {

namespace server3 {

file_store::file_store() : store_(INVALID_HANDLE_VALUE), buffer_position_(0)
{
#ifdef _WIN32
    strcpy(filename_, "./uploads");
    GetTempFileNameA(filename_, "pf_", 0, filename_);

    store_ = CreateFileA(filename_, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
#error unsupported platform
#endif
}

file_store::~file_store()
{
    if (store_ != INVALID_HANDLE_VALUE)
    {
        flush();
        CloseHandle(store_);
    }
    store_ = 0;
}

void file_store::operator()(char const ch)
{
    if (buffer_position_ == sizeof(buffer_))
        flush();
    assert(buffer_position_ < sizeof(buffer_));
    buffer_[buffer_position_++] = ch;
}

void file_store::flush(void)
{
    if (buffer_position_ > 0)
    {
        DWORD written;
        if (store_ == INVALID_HANDLE_VALUE
        ||  !WriteFile(store_, buffer_, buffer_position_, &written, NULL)
        ||  written != buffer_position_)
        {
            LOG_MEM << "*** Error " << GetLastError() << " @ " << __FILE__ << " (" << __LINE__ << ")";
            throw std::runtime_error("Unable to write to file");
        }
        buffer_position_ = 0;
   }
}

request::requests_t request::requests_;
boost::mutex        request::requests_lock_;

request::~request()
{
    if (!id_.empty())
        delete_request(id_);
    close();
}

void request::close(void)
{
    if (file_store_.get())
    {
        file_store_.reset();
        fileshare::on_upload_complete(link_);
    }
    id_.clear();
}

void request::fail(void)
{
    LOG_MEM << "*** Error: File upload for id " << id_ << " failed.";
    delete_request(id_);
    this->close();
    fileshare::delete_download(link_);

    short tries = 0;
    while (++tries <= 3)
    {
        try
        {
            boost::filesystem::remove(filename_);
            LOG_MEM << "Temporary file " << filename_ << " removed.";
            break;
        }
        catch (std::exception &e)
        {
            LOG_MEM << "*** Attempt " << tries << ": Unable to delete temporary file " << filename_ << "\n" << (e.what()?e.what():"");
            Sleep(1500);
        }
    }
}

void request::on_form_data(void)
{
    headers.push_back(header("C9A86238-A4C7-48cc-BF6F-095476CFC015"));
    file_store_.reset();

    std::string mime_type("text/plain");
    for (headers_t::const_reverse_iterator it=headers.rbegin(); it!=headers.rend(); ++it)
    {
        if (stricmp(it->name.c_str(), "Content-Type") == 0)
        {
            if (mime_type == "text/plain")
                mime_type = it->value;
        }
        else if (stricmp(it->name.c_str(), "Content-Disposition") == 0)
        {
            std::size_t offset = it->value.find("filename=\"");
            if (offset != std::string::npos)
            {
                offset += 10;
                size_t len = it->value.find('\"', offset+1) - offset;
                uploaded_file_original_filename = it->value.substr(offset,len);

                offset_ += boundary_marker.length() + 3;        // \r\n + first character of the boundary marker text
                file_store_.reset(new file_store);
                filename_ = file_store_->filename();
                headers.back().value = filename_;
                uploaded_file_mime_type = mime_type;
            }
            else if (strncmp(it->value.c_str(), "form-data; name=\"",17) == 0)
            {
                headers_t::const_reverse_iterator disposition_value = it;
                if (--disposition_value != headers.rend())
                {
                    if (strcmp(disposition_value->name.c_str(), "C9A86238-A4C7-48cc-BF6F-095476CFC015") == 0)
                    {
                        if (strcmp(it->value.c_str()+17, "id\"") == 0)
                        {
                            if (!disposition_value->value.empty())
                            {
                                id_ = disposition_value->value;
                                request::add_request(id_, shared_from_this());
                            }
                        }
                        else if (strcmp(it->value.c_str()+17, "tweet\"") == 0)
                        {
                            if (!disposition_value->value.empty())
                            {
                                tweet_ = disposition_value->value;
                                request::add_request(id_, shared_from_this());
                            }
                        }
                        else if (strcmp(it->value.c_str()+17, "twitter_screen_name\"") == 0)
                        {
                            if (!disposition_value->value.empty())
                            {
                                twitter_screen_name_ = disposition_value->value;
                                request::add_request(id_, shared_from_this());
                            }
                        }
                        else
                        {
                            LOG << "Unknown form field " << it->value.c_str()+16;
                        }
                    }
                }
            }
        }
    }

    /*!!! retries in case mysql server connection is dodgy */
    if (!uploaded_file_original_filename.empty()  &&  !filename_.empty()  &&  !id_.empty())
    {
        link_ = fileshare::store_file_details(
            uploaded_file_mime_type,
            uploaded_file_length(),
            uploaded_file_original_filename,
            filename_,
#ifdef _DEBUG
            "http://localhost:8089/?dl="+id_,
#elif CENTRIX_SOFTWARE
            "http://195.224.113.203/?dl="+id_,
#else
            "http://pyf.li:8089/?dl="+id_,
#endif
            twitter_screen_name_,
            tweet_);
    }
}

void request::operator()(char const &ch)
{
    if (file_store_.get())
        (*file_store_)(ch);
    else
        headers.back().value.push_back(ch);
}


bool const request::is_valid_request(std::string const &id)
{
    boost::mutex::scoped_lock lock(requests_lock_);
    requests_t::const_iterator it = requests_.find(id);
    return (it != requests_.end());
}

bool const request::get_request(std::string const &id, boost::shared_ptr<request> &req)
{
    boost::mutex::scoped_lock lock(requests_lock_);
    requests_t::const_iterator it = requests_.find(id);
    if (it == requests_.end())
        return false;
    req = it->second;
    assert(req.get());
    return true;
}

bool const request::add_request(std::string const &id, boost::shared_ptr<request> const &req)
{
    boost::mutex::scoped_lock lock(requests_lock_);
    requests_t::const_iterator it = requests_.find(id);
    if (it != requests_.end())
        return false;
    LOG << "Upload ID " << id << " added.";
    requests_[id] = req;
    return true;
}

bool const request::delete_request(std::string const &id)
{
    boost::mutex::scoped_lock lock(requests_lock_);
    requests_t::iterator it = requests_.find(id);
    if (it == requests_.end())
        return false;
    LOG << "Upload ID " << id << " removed.";
    requests_.erase(it);
    return true;
}

bool const request::clear_requests(void)
{
    while (requests_.size() > 0)
        requests_.erase(requests_.begin());
    return true;
}

}   // namespace server3

}   // namespace http

