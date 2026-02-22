#pragma once
#include <deque>
#include <type_traits>
#include <mutex>
#include <utility>

enum class endian
{
#if defined(_MSC_VER) && !defined(__clang__)
    little = 0,
    big    = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

class Buffer {
public:

    size_t remaining() {
        return _stream.size();
    }

    void write_buffer(const char* buffer, size_t size) {
        std::lock_guard<std::mutex> lock(_mutex);
        _stream.insert(_stream.end(), buffer, buffer + size);
    }

    void write_buffer(Buffer& buffer, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            this->write(buffer.read<char>());
        }
    }

    void peek_buffer(char* buffer, size_t size, bool reverse = false) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto i = 0; i < size; ++i) {
            buffer[i] = _stream[i];
        }
    }

    void read_buffer(char* buffer, size_t size, bool reverse = false) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto i = 0; i < size; ++i) {
            buffer[i] = _stream.front();
            _stream.pop_front();
        }
    }

    template<typename T>
    void write(T data) {
        static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
        write_buffer((char*) &data, sizeof(T));
    }

    template<typename T>
    T peek() {
        static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
        T buf{};
        peek_buffer((char*) &buf, sizeof(T), endian::native == endian::little);
        return buf;
    }

    template<typename T>
    T read() {
        static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
        T buf{};
        read_buffer((char*) &buf, sizeof(T), endian::native == endian::little);
        return buf;
    }
private:
    std::mutex _mutex;
    std::deque<char> _stream;
};