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
        kvm_w kvm;
        std::shared_ptr<vm_fd> vm_fd_{nullptr};
        std::shared_ptr<mutex_data<kvm_slots> > mem_slot{nullptr};

        explicit kvm_hypervisor() {
            if (auto result = kvm.create_vm()) {
                vm_fd_ = std::make_shared<vm_fd>(std::move(result.value()));
                mem_slot = std::make_shared<mutex_data<kvm_slots> >(kvm_slots{});
            }
        }

        IMPL
        HYPERVISOR_OPS_BEGIN
#ifdef __amd64__
#define IMPL_FILE "amd64/amd64_hypervisor.inl"
#elif defined(__aarch64__)
#define IMPL_FILE "aarch64_hypervisor.inl"
#else
#error "unsupported"
#endif
#include IMPL_FILE


        void arch_init() {
            uint64_t identity_addr = 0xFEF0C000;
            auto vm_fd_ptr = vm_fd_.get();
        }

        void init_machine(this auto &self) {
            self.arch_init();
        }

        [[nodiscard]] auto create_hypervisor_cpu(const uint8_t vcpu_id) const -> eden_result<std::shared_ptr<DYNAMIC cpu_hypervisor_ops> > {
            if (auto vcpu_fd = vm_fd_->create_vcpu(0)) {
                 return std::make_shared<cpu_hypervisor_ops>(kvm_cpu{vcpu_id,std::move(vcpu_fd.value())});
            }
            return std::unexpected{std::error_code{INT8_MAX,std::generic_category()}};
        }

        HYPERVISOR_OPS_END
    };


}
