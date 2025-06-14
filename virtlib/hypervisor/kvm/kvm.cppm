//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <expected>
#include <memory>
#include <unordered_map>
#include <asm/kvm.h>
#include <linux/kvm.h>
#include <print>

#include "../../util/macro.h"
export module eden_hypervisor:kvm;
import util;
import kvm_util;
import eden_cpu;
#define IMPL
#define HYPERVISOR_OPS_BEGIN
#define HYPERVISOR_OPS_END
#define DYNAMIC
using namespace eden_virt::util;
using namespace eden_virt::util::kvm;
using namespace eden_virt::cpu;
using kvm_mem_slot = kvm_userspace_memory_region;
using kvm_slots = std::unordered_map<uint32_t, kvm_mem_slot>;
export namespace eden_virt::hypervisor::kvm {

    struct kvm_cpu:  cpu_hypervisor_ops {
        uint8_t id;
        std::shared_ptr<vcpu_fd> fd{nullptr};

        kvm_cpu(u_int8_t _id, vcpu_fd &&_fd): id(_id) {
            fd = std::make_shared<vcpu_fd>(std::move(_fd));
        }

        auto get_hypervisor_type() -> hypervisor_type override {return hypervisor_type::kvm;}


        void init_pmu() override {
            std::println("amd64 init pmu: ok");
        }

        void vcpu_init() override {
            std::println("amd64 init vcpu: ok");
        }

    };

    struct kvm_hypervisor {
        kvm_w kvm{};
        std::shared_ptr<vm_fd> vm_fd_{nullptr};
        std::shared_ptr<mutex_data<kvm_slots> > mem_slot{nullptr};

        explicit kvm_hypervisor() {
            LOG(trace) << "kvm_hypervisor default contribute";
            if (auto result = kvm.create_vm(); result) {
                vm_fd_ = std::make_shared<vm_fd>(std::move(result.value()));
                mem_slot = std::make_shared<mutex_data<kvm_slots> >(kvm_slots{});
            }
        }

        IMPL
        HYPERVISOR_OPS_BEGIN
// #ifdef __amd64__
// #define IMPL_FILE "amd64/amd64_hypervisor.inl"
// #elif defined(__aarch64__)
// #define IMPL_FILE "aarch64_hypervisor.inl"
// #else
// #error "unsupported"
// #endif
// #include IMPL_FILE
        void arch_init() const {
            constexpr uint64_t identity_addr = 0xFEF0C000;
            const auto vm_fd_ptr = vm_fd_.get();
            if (auto res = vm_fd_ptr->set_identity_map_address(identity_addr)) {

            }
            if (auto res2 = vm_fd_ptr->set_tss_address(identity_addr+0x1000)) {

            }
        }
        void init_machine(this auto &self) {
            // TODO sys_mem sys_io
            self.arch_init();
        }

        static auto get_hypervisor_type() -> hypervisor_type  {
            return hypervisor_type::kvm;
        }

        [[nodiscard]] auto create_hypervisor_cpu(const uint8_t vcpu_id) const -> eden_result<std::shared_ptr<DYNAMIC cpu_hypervisor_ops> > {
            if (auto vcpu_fd = vm_fd_->create_vcpu(0)) {
                 return std::make_shared<kvm_cpu>(kvm_cpu{vcpu_id,std::move(vcpu_fd.value())});
            }
            return std::unexpected{std::error_code{INT8_MAX,std::generic_category()}};
        }

        HYPERVISOR_OPS_END
    };


}
