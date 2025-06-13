//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <cstdint>
#include <memory>
#include <asm/kvm.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
export module kvm;
import util;
import kvm_util;

using namespace eden_virt::util;
using namespace eden_virt::util::kvm;
export namespace eden_virt::hypervisor::kvm {

    struct kvm_hypervisor {
        kvm_w kvm;
        //std::shared_ptr<kvm_vm>

    };

    struct kvm_cpu {
        uint8_t id;
        std::shared_ptr<vcpu_fd> fd{nullptr};
        kvm_cpu(u_int8_t _id, vcpu_fd&& _fd): id(_id) {
            fd = std::make_shared<vcpu_fd>(std::move(_fd));
        }
    };

}
