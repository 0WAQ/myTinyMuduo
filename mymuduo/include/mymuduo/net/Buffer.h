#ifndef MYMUDUO_NET_BUFFER_H
#define MYMUDUO_NET_BUFFER_H

#include <string>
#include <cstring>
#include <vector>

namespace mymuduo {
namespace net {

class Buffer {
public:
    // TODO: 分割符后缀未处理转义!!!
    enum SepType {
        None,               // 无分割符
        LengthPrefix,       // 四字节的报文头
        DelimiterSuffix     // \r\n\r\n
    };

    Buffer(std::size_t prependable_size = 8, std::size_t writable_size = 1024);
    Buffer(const Buffer& other);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator= (const Buffer& other);
    Buffer& operator= (Buffer&& other);

    /**
     * @brief 分散读和集中写
     */
    std::size_t read_fd(int fd, int* save_errno);
    std::size_t write_fd(int fd, int* save_errno);


    void append_with_sep(const char* data, std::size_t size);
    void append_with_sep(const std::string& msg);

    void append(const char* data, std::size_t size);
    void append(const std::string& msg);

    char* begin() { return &*_buf.begin(); }
    const char* cbegin() const { return &*_buf.begin(); }

    char* end() { return &*_buf.end(); }
    const char* cend() const { return &*_buf.end(); }

    // 从 readable 区域中删除
    std::string erase(std::size_t size);

    void resize(std::size_t len);

    /**
     * @brief 从buf中取出一个报文
     */
    bool pick_datagram(std::string& msg);

    /**
     * @brief 移动 read_idx, 用于取出数据后
     */
    void retrieve(size_t len);
    void retrieve_all();
    std::string retrieve_as_string(size_t len);
    std::string retrieve_all_as_string();

    /**
     * @brief 确保缓冲区有size的大小
     */
    void ensure_writable(std::size_t size);

    /**
     * @brief 写区域增加len的大小
     */
    void has_written(std::size_t size);

    /**
     * @brief 获取缓冲区三个区域的大小
     */
    std::size_t prependable() { return _read_idx; }
    std::size_t readable()    { return _write_idx - _read_idx; }
    std::size_t writable()    { return _buf.size() - _write_idx; }
    SepType sep() { return _sep; }
    void set_sep(SepType sep) { _sep = sep; }

private:
    // prependable的初始大小, 也是 read_idx 和 write_idx 的初始值
    std::size_t _initial_prependable;

    // writable的初始大小
    std::size_t _initial_writable;

    std::size_t _read_idx;
    std::size_t _write_idx;

    // 自动扩容的vector<char>
    std::vector<char> _buf;

    // 分割类型
    SepType _sep = None;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_BUFFER_H