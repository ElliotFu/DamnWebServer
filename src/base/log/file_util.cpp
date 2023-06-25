#include "file_util.h"
#include "logger.h"

File_Util::File_Util(std::string& file_name)
    : fp_(::fopen(file_name.c_str(), "a")), written_bytes_(0) {
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

File_Util::~File_Util() {
    ::fclose(fp_);
}

void File_Util::append(const char* data, size_t len) {
    // 记录已经写入的数据大小
    size_t written = 0;

    while (written != len) {
        // 还需写入的数据大小
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "FileUtil::append() failed %s\n", get_errno_msg(err));
            }
        }
        // 更新写入的数据大小
        written += n;
    }
    // 记录目前为止写入的数据大小，超过限制会滚动日志
    written_bytes_ += written;
}

void File_Util::flush() {
    ::fflush(fp_);
}

size_t File_Util::write(const char* data, size_t len) {
    return ::fwrite_unlocked(data, 1, len, fp_);
}
