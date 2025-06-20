//
// Created by aoikajitsu on 25-6-13.
//
module;
#include "../util/macro.h"
#include <memory>
#include <vector>
export module eden_machine;
import eden_cpu;
import util;
import eden_hypervisor;
using namespace eden_virt::cpu;
using namespace eden_virt::hypervisor;
using namespace eden_virt::util;
using arch_cpu = eden_virt::cpu::amd64::amd64_cpu_state;

#if defined(__amd64__)
using cpu_topology = amd64::amd64_cpu_topology;
using namespace eden_virt::util;
#endif

export namespace eden_virt::machine {

    template<typename hypervisor>
    requires hypervisor_ops<hypervisor>
    struct eden_machine {
        cpu_topology topology;
        std::vector<eden_cpu> cpus;
        std::unique_ptr<mutex_data<hypervisor>> hyper{};
        eden_machine() {
            LOG(info) << get_type_name<hypervisor>();
            LOG(trace) << "eden_machine default contribute";
            hyper = std::make_unique<mutex_data<hypervisor>>();
            auto [lock,hypervisor_] = hyper->lock();
            if (auto cpu = hypervisor_.create_hypervisor_cpu(0);cpu) {
                cpus.push_back(eden_cpu{0,std::move(cpu.value())});
            }
        }
    };
}