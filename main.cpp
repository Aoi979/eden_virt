#include <iostream>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "virtlib/util/macro.h"
import eden_machine;
import eden_hypervisor;
import util;
#define GUEST_CODE_SIZE 0x1000
using namespace eden_virt::hypervisor::kvm;
using namespace eden_virt::util;


int main() {
    auto eden_machine = eden_virt::machine::eden_machine<kvm_hypervisor>();
    void *guest_mem = mmap(NULL, GUEST_CODE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    uint8_t *code = static_cast<uint8_t *>(guest_mem);
    code[0] = 0xf4; // HLT

    struct kvm_userspace_memory_region region = {
        .slot = 0,
        .guest_phys_addr = 0x1000,
        .memory_size = GUEST_CODE_SIZE,
        .userspace_addr = reinterpret_cast<uint64_t>(guest_mem),
    };

    auto [lock,hypervisor] = eden_machine.hyper->lock();
    auto vm{hypervisor.vm_fd_->vm.get()};
    ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region);

    auto vcpu = eden_machine.cpus[0].hypervisor_cpu->fd->vcpu.get();
    auto run = eden_machine.cpus[0].hypervisor_cpu->fd->kvm_run_ptr.as_ptr();

    // Set up sregs
    struct kvm_sregs sregs;
    ioctl(vcpu, KVM_GET_SREGS, &sregs);
    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    ioctl(vcpu, KVM_SET_SREGS, &sregs);

    // Set up regs
    struct kvm_regs regs = {
        .rip = 0x1000,
        .rax = 0,
        .rbx = 0,
        .rflags = 0x2,
    };
    eden_machine.cpus[0].hypervisor_cpu->fd->set_registers(regs);

    int ret = ioctl(vcpu, KVM_RUN, 0);
    switch (run->exit_reason) {
        case KVM_EXIT_HLT:
            printf("KVM_EXIT_HLT\n");
            break;
        case KVM_EXIT_INTERNAL_ERROR:
            printf("KVM_EXIT_INTERNAL_ERROR\n");
            printf("  suberror = 0x%x\n", run->internal.suberror);
            printf("  n_data = %d\n", run->internal.ndata);
            for (int i = 0; i < run->internal.ndata; ++i) {
                printf("  data[%d] = 0x%llx\n", i, (unsigned long long) run->internal.data[i]);
            }
            break;
        default:
            printf("Unhandled exit reason: %d\n", run->exit_reason);
            break;
    }
    return 0;
}

