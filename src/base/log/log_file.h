#pragma once

#include <mutex>
#include <memory>

class File_Util;

class Log_File
{
public:
    Log_File(const std::string& basename,
            off_t rollSize,
            int flush_interval = 3,
            int check_every_n = 1024);
    ~Log_File();

    void append(const char* data, int len);
    void flush();
    bool roll_file();

private:
    static std::string get_log_file_name(const std::string& basename, time_t* now);
    void append_in_lock(const char* data, int len);

    const std::string basename_;
    const off_t roll_size_;
    const int flush_interval_;
    const int check_every_n_;

    int count_;

    std::unique_ptr<std::mutex> mutex_;
    time_t start_of_period_;
    time_t last_roll_;
    time_t last_flush_;
    std::unique_ptr<File_Util> file_;

    constexpr static int ROLL_INTERVAL = 60*60*24;
};