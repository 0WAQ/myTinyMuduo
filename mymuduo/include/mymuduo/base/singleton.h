#ifndef MYMUDUO_BASE_SINGLETON_H
#define MYMUDUO_BASE_SINGLETON_H

#include <utility>

namespace mymuduo {

template <typename T>
class Singleton {
public:
    template <typename... Args>
    static T& instance(Args&&... args) {
        // 懒汉模式, 在第一次调用时才创建对象
        static T ins(std::forward<Args>(args)...);  // c++11以后, 使用局部变量懒汉不用加锁
        return ins;
    }

    ~Singleton() = default;

protected:
    template <typename... Args>
    explicit Singleton(Args&&... args) { }

private:
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator= (const Singleton<T>&) = delete;
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_SINGLETON_H