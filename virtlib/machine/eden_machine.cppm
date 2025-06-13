//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <vector>
export module eden_machine;
import eden_cpu;
using namespace eden_virt::cpu;
#if defined(__amd64__)
using cpu_topology = amd64::amd64_cpu_topology;
#endif

export namespace eden_virt::machine {
    struct eden_machine {
        cpu_topology topology;
        std::pmr::vector<eden_cpu> cpus;
    };
}