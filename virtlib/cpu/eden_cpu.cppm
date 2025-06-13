//
// Created by aoikajitsu on 25-6-12.
//
module;
#include <condition_variable>
#include <tuple>
#include <memory>
export module eden_cpu;
import util;
#define  tuple(...) std::tuple<__VA_ARGS__>

#if defined(__amd64__)
export import :amd64cpu;
using arch_cpu = eden_virt::cpu::amd64::amd64_cpu_state;
#endif

using eden_virt::util::mutex_data;
export namespace eden_virt::cpu {
    enum class cpu_lifecycle_state:uint8_t {
        CREATED,
        RUNNING,
        PAUSED,
        STOPPING,
        STOPPED,
    };

    struct eden_cpu {
        uint8_t id;
        std::shared_ptr<mutex_data<arch_cpu>> arch_cpu;
        std::shared_ptr<mutex_data<tuple(cpu_lifecycle_state,std::condition_variable)>> state;
    private:

    };
}