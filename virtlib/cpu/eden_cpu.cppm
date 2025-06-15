//
// Created by aoikajitsu on 25-6-12.
//
module;
#include <condition_variable>
#include <tuple>
#include <memory>

#include "../util/macro.h"
export module eden_cpu;
import util;
#define  tuple(...) std::tuple<__VA_ARGS__>
import kvm_util;
#if defined(__amd64__)
export import :amd64cpu;
using arch_cpu = eden_virt::cpu::amd64::amd64_cpu_state;
#endif
using namespace eden_virt::util;
using namespace eden_virt::util::kvm;
export namespace eden_virt::cpu {

    struct cpu_hypervisor_ops {
        uint8_t id{255};
        std::shared_ptr<vcpu_fd> fd{nullptr};
        virtual ~cpu_hypervisor_ops() = default;

        virtual auto get_hypervisor_type() -> hypervisor_type = 0;

        virtual void init_pmu() = 0;

        virtual void vcpu_init() = 0;
    };



    enum class cpu_lifecycle_state:uint8_t {
        CREATED,
        RUNNING,
        PAUSED,
        STOPPING,
        STOPPED,
    };

    struct eden_cpu {
        uint8_t id;
        std::shared_ptr<mutex_data<arch_cpu>> arch_cpu_{nullptr};
        std::shared_ptr<mutex_data<tuple(cpu_lifecycle_state,std::condition_variable)>> state{nullptr};
        std::shared_ptr<DYNAMIC cpu_hypervisor_ops> hypervisor_cpu{nullptr};
        eden_cpu(uint8_t id,std::shared_ptr<DYNAMIC cpu_hypervisor_ops>&& hypervisor_cpu_):id(id) {
            arch_cpu_ = std::make_shared<mutex_data<arch_cpu>>(id,1);
            state = std::make_shared<mutex_data<tuple(cpu_lifecycle_state,std::condition_variable)>>();
            hypervisor_cpu = std::move(hypervisor_cpu_);
        }

    };
}