#include "log_file.h"
#include "file_util.h"

Log_File::Log_File(const std::string& basename,
                   off_t roll_size,
                   int flush_interval,
                   int check_every_n)
    : basename_(basename),
      roll_size_(roll_size),
      flush_interval_(flush_interval),
      check_every_n_(check_every_n),
      count_(0),
      mutex_(new std::mutex),
      start_of_period_(0), last_roll_(0), last_flush_(0){
    roll_file();
}

Log_File::~Log_File() = default;

void Log_File::append(const char* data, int len)
{
    std::lock_guard<std::mutex> lock(*mutex_);
    append_in_lock(data,len);
}

void Log_File::append_in_lock(const char* data, int len)
{
    file_->append(data, len);
    if (file_->written_bytes() > roll_size_) {
        roll_file();
    }
    else {
        ++count_;
        if (count_ >= check_every_n_)
        {
            count_ = 0;
            time_t now = ::time(nullptr);
            time_t this_period = now / ROLL_INTERVAL * ROLL_INTERVAL;
            if (this_period != start_of_period_) {
                roll_file();
            }
            else if (now - last_flush_ > flush_interval_) {
                last_flush_ = now;
                file_->flush();
            }
        }
    }
}

void Log_File::flush()
{
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
}


bool Log_File::roll_file()
{
    time_t now = ::time(nullptr);
    std::string filename = get_log_file_name(basename_, &now);
    time_t cur_period = now / ROLL_INTERVAL * ROLL_INTERVAL;
    if (now > last_roll_) {
        last_roll_ = now;
        last_flush_ = now;
        start_of_period_ = cur_period;
        file_.reset(new File_Util(filename));
        return true;
    }
    return false;
}

// basename + time + ".log"
std::string Log_File::get_log_file_name(const std::string& basename, time_t* now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(nullptr);
    localtime_r(now, &tm);
    // 写入时间
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}