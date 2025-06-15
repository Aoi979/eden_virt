//
// Created by aoikajitsu on 25-6-12.
//
module;
#include <array>
#include <cstdint>
#include <tuple>
#include <asm/kvm.h>
#include <linux/kvm.h>
export module eden_cpu:amd64cpu;
import kvm_util;
using namespace eden_virt::util::kvm;
export namespace eden_virt::cpu::amd64 {
    struct amd64_cpu_topology {
        uint8_t threads;
        uint8_t cores;
        uint8_t dies;

        explicit amd64_cpu_topology(): threads(1), cores(1), dies(1) {
        }

        explicit amd64_cpu_topology(std::tuple<uint8_t, uint8_t, uint8_t> topo): threads(std::get<0>(topo)),
            cores(std::get<1>(topo)), dies(std::get<2>(topo)) {
        }

        amd64_cpu_topology(const amd64_cpu_topology &) = default;

        amd64_cpu_topology &operator=(const amd64_cpu_topology &other) = default;
    };

    struct amd64_cpu_state {
    private:
        uint8_t max_vcpus;
        uint8_t nr_threads{1};
        uint8_t nr_cores{1};
        uint8_t nr_dies{1};
        uint8_t nr_sockets{1};

    public:
        uint8_t apic_id;
        kvm_regs regs{};
        kvm_sregs sregs{};
        kvm_fpu fpu{};
        kvm_mp_state mp_state{};
        kvm_lapic_state lapic_state{};
        std::size_t msr_len{};
        std::array<kvm_msr_entry, 256> msr_list{};
        kvm_vcpu_events vcpu_events{};

        kvm_xcrs xcrs{};
        kvm_debugregs debugregs{};
        kvm_xsave_e xsave{};

        explicit amd64_cpu_state(uint8_t vcpu_id, uint8_t max_vcpus): apic_id(vcpu_id), max_vcpus(max_vcpus) {
            if (vcpu_id == 0) {
                mp_state = {.mp_state = KVM_MP_STATE_RUNNABLE};
            } else {
                mp_state = {.mp_state = KVM_MP_STATE_UNINITIALIZED};
            }
        }

        // set topology after default construction
        void set_topo(const amd64_cpu_topology &topo) {
            nr_cores = topo.cores;
            nr_dies = topo.dies;
            nr_threads = topo.threads;
        }
    };
}
