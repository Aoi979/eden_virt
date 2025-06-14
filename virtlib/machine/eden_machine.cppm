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
        std::shared_ptr<mutex_data<hypervisor>> hyper{nullptr};
        eden_machine() {
            LOG(info) << get_type_name<hypervisor>();
            LOG(trace) << "eden_machine default contribute";
            hyper = std::make_shared<mutex_data<hypervisor>>();
        }
    };
}