//
// Created by aoikajitsu on 25-6-13.
//
module;
export module kvm;
import util;
#include <cstdint>
#include <memory>
#include <asm/kvm.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
using namespace eden_virt::util;
export namespace eden_virt::hypervisor::kvm {
    struct  vcpu_regs {
        static kvm_regs get_regs(int vcpu_fd) {
            kvm_regs regs = {};
            if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_GET_REGS failed"
                );
            }
            return regs;
        }

        static void set_regs(int vcpu_fd, const kvm_regs &regs) {
            if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_SET_REGS failed"
                );
            }
        }

        static kvm_sregs get_sregs(int vcpu_fd) {
            kvm_sregs sregs = {};
            if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_GET_SREGS failed"
                );
            }
            return sregs;
        }

        static void set_sregs(int vcpu_fd, const kvm_sregs &sregs) {
            if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_SET_SREGS failed"
                );
            }
        }

        static kvm_debugregs get_debug_regs(int vcpu_fd) {
            kvm_debugregs dbgregs = {};
            if (ioctl(vcpu_fd, KVM_GET_DEBUGREGS, &dbgregs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_GET_DEBUGREGS failed"
                );
            }
            return dbgregs;
        }

        static void set_debug_regs(int vcpu_fd, const kvm_debugregs &dbgregs) {
            if (ioctl(vcpu_fd, KVM_SET_DEBUGREGS, &dbgregs) == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_SET_DEBUGREGS failed"
                );
            }
        }
    };

    struct kvm_run {
        kvm_run(const kvm_run &) = delete;

        kvm_run &operator=(const kvm_run &) = delete;

        kvm_run(kvm_run &&other) noexcept
            : kvm_run_ptr_(other.kvm_run_ptr_)
              , mmap_size_(other.mmap_size_) {
            other.kvm_run_ptr_ = MAP_FAILED;
            other.mmap_size_ = 0;
        }

        kvm_run &operator=(kvm_run &&other) noexcept {
            if (this != &other) {
                release();
                kvm_run_ptr_ = other.kvm_run_ptr_;
                mmap_size_ = other.mmap_size_;

                other.kvm_run_ptr_ = MAP_FAILED;
                other.mmap_size_ = 0;
            }
            return *this;
        }

        static kvm_run mmap_from_fd(int fd, size_t size) {
            if (fd < 0) {
                throw std::invalid_argument("Invalid file descriptor");
            }

            void *addr = ::mmap(
                nullptr,
                size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0
            );

            if (addr == MAP_FAILED) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "mmap failed"
                );
            }
            return {addr, size};
        }

        [[nodiscard]] kvm_run &as_mut_ref() const {
            if (!valid()) {
                throw std::runtime_error("KvmRunWrapper is invalid");
            }
            return *static_cast<kvm_run *>(kvm_run_ptr_);
        }


        [[nodiscard]] const kvm_run &as_ref() const {
            if (!valid()) {
                throw std::runtime_error("KvmRunWrapper is invalid");
            }
            return *static_cast<const kvm_run *>(kvm_run_ptr_);
        }

        [[nodiscard]] bool valid() const noexcept {
            return kvm_run_ptr_ != MAP_FAILED && mmap_size_ > 0;
        }


        void release() noexcept {
            if (valid()) {
                ::munmap(kvm_run_ptr_, mmap_size_);
                kvm_run_ptr_ = MAP_FAILED;
                mmap_size_ = 0;
            }
        }


        ~kvm_run() {
            release();
        }

    private:
        kvm_run(void *kvm_run_ptr, size_t mmap_size)
            : kvm_run_ptr_(kvm_run_ptr)
              , mmap_size_(mmap_size) {
        }

        void *kvm_run_ptr_;
        size_t mmap_size_;
    };

    enum class vm_exit:uint8_t {  };
    struct vcpu_fd {
        file_descriptor vcpu;
        kvm_run kvm_run_ptr;
        auto run() -> vm_exit {

        }
        [[nodiscard]] kvm_regs get_registers() const {
            return vcpu_regs::get_regs(vcpu.get());
        }

        void set_registers(const kvm_regs& regs) const {
            vcpu_regs::set_regs(vcpu.get(), regs);
        }

        [[nodiscard]]kvm_sregs get_special_registers() const {
            return vcpu_regs::get_sregs(vcpu.get());
        }

        void set_special_registers(const kvm_sregs& sregs) const {
            vcpu_regs::set_sregs(vcpu.get(), sregs);
        }
    };

    struct kvm_cpu {
        uint8_t id;
        std::shared_ptr<file_descriptor>;
    };
}
