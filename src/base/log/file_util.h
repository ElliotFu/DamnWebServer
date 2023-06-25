#pragma once

#include <string>

class File_Util
{
public:
    explicit File_Util(std::string& filename);
    ~File_Util();

    void append(const char* data, size_t len);
    void flush();
    off_t written_bytes() const { return written_bytes_; }

private:
    size_t write(const char* data, size_t len);

    FILE* fp_;
    char buffer_[64 * 1024]; // 64 KB
    off_t written_bytes_; // 文件偏移量
};