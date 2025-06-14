//
// Created by aoikajitsu on 25-6-14.
//
module;
#include <concepts>
#include <memory>

#define DYNAMIC
// only support kvm backends!
export module eden_hypervisor;
export import :kvm;
import util;
import eden_cpu;
using namespace eden_virt::cpu;
export namespace eden_virt::hypervisor {
    template<typename T>
    concept hypervisor_ops = requires(T& t) {
        {t.get_hypervisor_type()} -> std::same_as<hypervisor_type>;
        {t.init_machine()} -> std::same_as<void>;
        {t.create_hypervisor_cpu(0)} ->std::same_as<eden_result<std::shared_ptr<DYNAMIC cpu_hypervisor_ops>>>;
    };


}