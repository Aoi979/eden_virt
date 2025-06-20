//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <atomic>
#include <mutex>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <expected>
#include <iostream>
#include <sstream>
#include <string>
#include <shared_mutex>
#include <tuple>
#include <utility>
#include <initializer_list>
export module util;

export namespace eden_virt::util {
    template<typename Container, typename Key, typename KeyFn>
    std::optional<size_t> binary_search_by_key(const Container &container, const Key &search_key, KeyFn key_fn) {
        size_t left = 0;
        size_t right = container.size();

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            auto mid_key = key_fn(container[mid]);

            if (mid_key < search_key) {
                left = mid + 1;
            } else if (mid_key > search_key) {
                right = mid;
            } else {
                return mid; // found
            }
        }

        return std::nullopt; // not found
    }

    enum class log_level {
        trace, debug, info, warn, error, fatal
    };

    inline const char *level_to_string(log_level level) {
        switch (level) {
            case log_level::trace: return "TRACE";
            case log_level::debug: return "DEBUG";
            case log_level::info: return "INFO";
            case log_level::warn: return "WARN";
            case log_level::error: return "ERROR";
            case log_level::fatal: return "FATAL";
        }
        return "UNKNOWN";
    }

    inline const char *level_to_color(log_level level) {
        switch (level) {
            case log_level::trace: return "\033[37m";
            case log_level::debug: return "\033[36m";
            case log_level::info: return "\033[32m";
            case log_level::warn: return "\033[33m";
            case log_level::error: return "\033[31m";
            case log_level::fatal: return "\033[1;31m";
        }
        return "\033[0m";
    }

    struct log_stream {
        std::ostringstream oss;
        log_level level;
        const char *file;
        int line;
        const char *func;

        log_stream(log_level lvl, const char *f, int l, const char *fn)
            : level(lvl), file(f), line(l), func(fn) {
        }

        template<typename T>
        log_stream &operator<<(const T &val) {
            oss << val;
            return *this;
        }

        ~log_stream() {
            std::time_t now = std::time(nullptr);
            char time_buf[32];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&now));
            std::ostream &out = (level >= log_level::error) ? std::cerr : std::cout;
            out << level_to_color(level)
                    << "[" << time_buf << "] "
                    << level_to_string(level) << " "
                    << file << ":" << line << " " << func << "(): "
                    << oss.str()
                    << "\033[0m" << std::endl;
        }
    };


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

        mutex_data() = default;

        template<typename U>
            requires std::is_constructible_v<T, std::initializer_list<U> >
        mutex_data(std::initializer_list<U> il) : data(il) {
        }

        template<typename... Args>
            requires std::is_constructible_v<T, Args...>
        explicit mutex_data(Args &&... args) : data(std::forward<Args>(args)...) {
        }

        mutex_data(const mutex_data &d) = delete;

        mutex_data(mutex_data &&d) noexcept = delete;

        auto lock() -> std::tuple<std::unique_lock<std::mutex>, T &> {
            return {std::unique_lock(mtx), data};
        }
    };

    template<typename T>
    struct shared_mutex_data {
        T data;
        mutable std::shared_mutex mtx;

        shared_mutex_data() = default;

        template<typename U>
            requires std::is_constructible_v<T, std::initializer_list<U> >
        shared_mutex_data(std::initializer_list<U> il) : data(il) {
        }

        template<typename... Args>
            requires std::is_constructible_v<T, Args...>
        explicit shared_mutex_data(Args &&... args)
            : data(std::forward<Args>(args)...) {
        }

        shared_mutex_data(const shared_mutex_data &) = delete;

        shared_mutex_data(shared_mutex_data &&) noexcept = delete;


        auto unique_lock() -> std::tuple<std::unique_lock<std::shared_mutex>, T &> {
            return {std::unique_lock(mtx), data};
        }

        auto shared_lock() const -> std::tuple<std::shared_lock<std::shared_mutex>, const T &> {
            return {std::shared_lock(mtx), data};
        }
    };


    template<typename T>
    using eden_result = std::expected<T, std::error_code>;

    class file_descriptor {
    public:
        static constexpr int INVALID = -114514;

        file_descriptor() noexcept = delete;

        explicit file_descriptor(int fd) noexcept : fd_(fd) {
        }

        file_descriptor(const file_descriptor &) = delete;

        file_descriptor(file_descriptor &&other) noexcept
            : fd_(std::exchange(other.fd_, INVALID)) {
        }

        ~file_descriptor() { reset(); }

        file_descriptor &operator=(const file_descriptor &) = delete;

        file_descriptor &operator=(file_descriptor &&other) noexcept {
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

        [[nodiscard]] int get() const noexcept { return fd_; }

        operator int() const noexcept { return fd_; }

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

    enum class eden_error {
        KVM_CREATE_VM_ERROR,
        KVM_GET_VCPU_MMAP_SIZE_ERROR
    };

    class eden_err_category : public std::error_category {
    public:
        [[nodiscard]] const char *name() const noexcept override {
            return "eden";
        }

        [[nodiscard]] std::string message(int ev) const override {
            switch (static_cast<eden_error>(ev)) {
                case eden_error::KVM_CREATE_VM_ERROR:
                    return "Operation succeeded";
                case eden_error::KVM_GET_VCPU_MMAP_SIZE_ERROR:
                    return "Invalid input provided";
                default:
                    return "Unknown error";
            }
        }

        static const eden_err_category &get() {
            static const eden_err_category instance;
            return instance;
        }
    };

    std::error_code make_error_code(eden_error errcode) {
        return {
            static_cast<int>(errcode),
            eden_err_category::get()
        };
    }
}

namespace std {
    template<>
    struct is_error_code_enum<eden_virt::util::eden_error> : true_type {
    };
}
