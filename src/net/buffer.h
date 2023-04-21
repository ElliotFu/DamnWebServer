#pragma once

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
class Buffer
{
public:
    // prependable 初始大小，readIndex 初始位置
    static const size_t k_cheap_prepend = 8;
    // writeable 初始大小，writeIndex 初始位置
    // 刚开始 readerIndex 和 writerIndex 处于同一位置
    static const size_t k_initial_size = 1024;

    explicit Buffer(size_t initial_size = k_initial_size)
        :   buffer_(k_cheap_prepend + initial_size),
            reader_index_(k_cheap_prepend),
            writer_index_(k_cheap_prepend)
        {}

    /**
     * k_cheap_prepend | reader | writer |
     * writer_index_ - reader_index_
     */
    size_t readable_bytes() const { return writer_index_ - reader_index_; }
    /**
     * k_cheap_prepend | reader | writer |
     * buffer_.size() - writer_index_
     */
    size_t writable_bytes() const { return buffer_.size() - writer_index_; }
    /**
     * k_cheap_prepend | reader | writer |
     * wreader_index_
     */
    size_t prependable_bytes() const { return reader_index_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + reader_index_;
    }

    void retrieve_until(const char *end)
    {
        retrieve(end - peek());
    }

    // onMessage string <- Buffer
    // 需要进行复位操作
    void retrieve(size_t len)
    {
        // 应用只读取可读缓冲区数据的一部分(读取了len的长度)
        if (len < readable_bytes())
        {
            // 移动可读缓冲区指针
            reader_index_ += len;
        }
        // 全部读完 len == readableBytes()
        else
        {
            retrieve_all();
        }
    }

    // 全部读完，则直接将可读缓冲区指针移动到写缓冲区指针那
    void retrieve_all()
    {
        reader_index_ = k_cheap_prepend;
        writer_index_ = k_cheap_prepend;
    }

    // DEBUG使用，提取出string类型，但是不会置位
    std::string get_buffer_all_as_string()
    {
        size_t len = readable_bytes();
        std::string result(peek(), len);
        return result;
    }

    // 将onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieve_all_as_string()
    {
        // 应用可读取数据的长度
        return retrieve_as_string(readable_bytes());
    }

    std::string retrieve_as_string(size_t len)
    {
        // peek()可读数据的起始地址
        std::string result(peek(), len);
        // 上面一句把缓冲区中可读取的数据读取出来，所以要将缓冲区复位
        retrieve(len);
        return result;
    }

    // buffer_.size() - writeIndex_
    void ensure_writable_bytes(size_t len)
    {
        if (writable_bytes() < len)
        {
            // 扩容函数
            make_space(len);
        }
    }

    // string::data() 转换成字符数组，但是没有 '\0'
    void append(const std::string &str)
    {
        append(str.data(), str.size());
    }

    // void append(const char *data)
    // {
    //     append(data, sizeof(data));
    // }

    // 把[data, data+len]内存上的数据添加到缓冲区中
    void append(const char *data, size_t len)
    {
        ensure_writable_bytes(len);
        std::copy(data, data+len, begin_write());
        writer_index_ += len;
    }

    const char* find_CRLF() const
    {
        // FIXME: replace with memmem()?
        const char* crlf = std::search(peek(), begin_write(), k_CRLF, k_CRLF+2);
        return crlf == begin_write() ? NULL : crlf;
    }

    char* begin_write()
    {
        return begin() + writer_index_;
    }

    const char* begin_write() const
    {
        return begin() + writer_index_;
    }

    // 从fd上读取数据
    ssize_t read_fd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t write_fd(int fd, int *saveErrno);

private:
    char* begin()
    {
        // 获取buffer_起始地址
        return &(*buffer_.begin());
    }

    const char* begin() const
    {
        return &(*buffer_.begin());
    }

    // TODO:扩容操作
    void make_space(int len)
    {
        /**
         * k_cheap_prepend | reader | writer |
         * k_cheap_prepend |       len         |
         */
        // 整个buffer都不够用
        if (writable_bytes() + prependable_bytes() < len + k_cheap_prepend)
        {
            buffer_.resize(writer_index_ + len);
        }
        else // 整个buffer够用，将后面移动到前面继续分配
        {
            size_t readable = readable_bytes();
            std::copy(begin() + reader_index_,
                    begin() + writer_index_,
                    begin() + k_cheap_prepend);
            reader_index_ = k_cheap_prepend;
            writer_index_ = reader_index_ + readable;
        }
    }

    /**
     * 采取 vector 形式，可以自动分配内存
     * 也可以提前预留空间大小、
     */
    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
    static const char k_CRLF[];
};