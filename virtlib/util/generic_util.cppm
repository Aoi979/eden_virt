//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <mutex>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <expected>
export module util;

export namespace eden_virt::util {

    enum class hypervisor_type {
        kvm,
        test,
    };

    template<typename First, typename... Rest>
    struct first_type {
        using type = First;
    };

    template<typename... Args>
    using first_type_t = typename first_type<Args...>::type;

    template<typename T>
    struct mutex_data {
        T data;
        std::mutex mtx;

        mutex_data() = delete;

        template<typename U>
            requires std::is_constructible_v<T, std::initializer_list<U>>
        mutex_data(std::initializer_list<U> il) : data(il) {}

        template<typename... Args>
            requires std::is_constructible_v<T, Args...>
        explicit mutex_data(Args &&... args) : data(std::forward<Args>(args)...) {}

        mutex_data(const mutex_data &d) = delete;

        mutex_data(mutex_data &&d) noexcept = delete;

        auto lock() -> std::tuple<std::unique_lock<std::mutex>, T &> {
            return {std::unique_lock(mtx), data};
        }
    };


    template<typename T>
    using eden_result = std::expected<T,std::error_code>;

    class file_descriptor {
    public:
        static constexpr int INVALID = -1;

        file_descriptor() noexcept = delete;

        explicit file_descriptor(int fd) noexcept : fd_(fd) {}

        file_descriptor(const file_descriptor&) = delete;

        file_descriptor(file_descriptor&& other) noexcept
            : fd_(std::exchange(other.fd_, INVALID)) {}

        ~file_descriptor() { reset(); }

        file_descriptor& operator=(const file_descriptor&) = delete;

        file_descriptor& operator=(file_descriptor&& other) noexcept {
            reset();
            fd_ = std::exchange(other.fd_, INVALID);
            return *this;
        }

        void reset() noexcept {
            if (fd_ != INVALID) {
                ::close(fd_);
                fd_ = INVALID;
            }
        }

        int release() noexcept {
            return std::exchange(fd_, INVALID);
        }

        int get() const noexcept { return fd_; }

        explicit operator bool() const noexcept { return fd_ != INVALID; }

        [[nodiscard]] file_descriptor duplicate() const {
            if (fd_ == INVALID) throw std::logic_error("Cannot duplicate invalid fd");
            int new_fd = ::dup(fd_);
            if (new_fd < 0) throw std::system_error(errno, std::generic_category());
            return file_descriptor(new_fd);
        }

        void replace_with(int target_fd) {
            if (fd_ == INVALID) throw std::logic_error("Cannot replace with invalid fd");
            if (::dup2(fd_, target_fd) < 0) {
                throw std::system_error(errno, std::generic_category());
            }
            reset();
        }

    private:
        int fd_ = INVALID;
    };
}
