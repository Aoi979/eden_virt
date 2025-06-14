//
// Created by aoikajitsu on 25-6-13.
//
module;
#include <cstdint>
#include <expected>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <variant>
#include <asm/kvm.h>
#include <iostream>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "macro.h"
#define REJECT_RVALUE
export module kvm_util;
import util;
using namespace eden_virt::util;
export namespace eden_virt::util::kvm {
    using readonly_data = std::span<const uint8_t>;
    using writable_data = std::span<uint8_t>;

    struct io_out {
        uint16_t port;
        readonly_data data;
    };

    struct io_in {
        uint16_t port;
        writable_data data;
    };

    struct mmio_read {
        uint64_t address;
        writable_data data;
    };

    struct mmio_write {
        uint64_t address;
        readonly_data data;
    };

    struct debug_exit {
        kvm_debug_exit_arch arch;
    };

    struct fail_entry {
        uint64_t hardware_entry_failure_reason;
        uint32_t cpu;
    };

    struct system_event {
        uint32_t type;
        uint64_t flags;
    };

    struct ioapic_eoi {
        uint8_t vector;
    };

    struct kvm_run_w {
        kvm_run_w(const kvm_run_w &) = delete;

        kvm_run_w &operator=(const kvm_run_w &) = delete;

        kvm_run_w(kvm_run_w &&other) noexcept
            : kvm_run_ptr_(other.kvm_run_ptr_)
              , mmap_size_(other.mmap_size_) {
            LOG(trace) << "kvm_run_w move constructor";
            other.kvm_run_ptr_ = MAP_FAILED;
            other.mmap_size_ = 0;
        }

        kvm_run_w &operator=(kvm_run_w &&other) noexcept {
            LOG(trace) << "kvm_run_w move assignment";
            if (this != &other) {
                release();
                kvm_run_ptr_ = other.kvm_run_ptr_;
                mmap_size_ = other.mmap_size_;

                other.kvm_run_ptr_ = MAP_FAILED;
                other.mmap_size_ = 0;
            }
            return *this;
        }

        // vcpu_fd, size
        static kvm_run_w mmap_from_fd(int fd, size_t size) {
            LOG(trace)<<"kvm_run_w mmap_from_fd contribute";
            if (fd < 0) {
                LOG(error) << "Invalid file descriptor form: {mmap_from_fd}";
                throw std::invalid_argument("Invalid file descriptor form: {mmap_from_fd}");
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

        // TODO need optimization
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

        [[nodiscard]] kvm_run *as_ptr() const {
            if (!valid()) {
                throw std::runtime_error("KvmRunWrapper is invalid");
            }
            return static_cast<kvm_run *>(kvm_run_ptr_);
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


        ~kvm_run_w() {
            LOG(trace) << "kvm_run_w::~kvm_run_w()";
            release();
        }

    private:
        kvm_run_w(void *kvm_run_ptr, size_t mmap_size)
            : kvm_run_ptr_(kvm_run_ptr)
              , mmap_size_(mmap_size) {
        }

        void *kvm_run_ptr_;
        size_t mmap_size_;
    };

    struct vcpu_exit {
        enum class reason {
            unknown,
            exception,
            hypercall,
            debug,
            hlt,
            irq_window_open,
            shutdown,
            fail_entry,
            intr,
            set_tpr,
            tpr_access,
            s390_sieic,
            s390_reset,
            dcr,
            nmi,
            internal_error,
            osi,
            papr_hcall,
            s390_ucontrol,
            watchdog,
            s390_tsch,
            epr,
            system_event,
            s390_stsi,
            ioapic_eoi,
            hyperv,
            io_out,
            io_in,
            mmio_read,
            mmio_write,
            unsupported
        };


        vcpu_exit() : reason_(reason::unsupported) {
            LOG(trace) << "vcpu_exit default contribute";
        }

        explicit vcpu_exit(const kvm_run_w &run) {
            LOG(trace)<< "vcpu_exit explicit contribute: from kvm_run_w";
            switch (const auto run_ptr = run.as_ptr(); run_ptr->exit_reason) {
                case KVM_EXIT_IO:
                    if (run_ptr->io.direction == KVM_EXIT_IO_OUT) {
                        reason_ = reason::io_out;
                        variant_.emplace<io_out>(
                            run_ptr->io.port,
                            readonly_data(reinterpret_cast<uint8_t *>(run_ptr->io.data_offset + run_ptr),
                                          run_ptr->io.size * run_ptr->io.count)
                        );
                    } else {
                        reason_ = reason::io_in;
                        variant_.emplace<io_in>(
                            run_ptr->io.port,
                            writable_data(reinterpret_cast<uint8_t *>(run_ptr->io.data_offset + run_ptr),
                                          run_ptr->io.size * run_ptr->io.count)
                        );
                    }
                    break;

                case KVM_EXIT_MMIO:
                    if (run_ptr->mmio.is_write) {
                        reason_ = reason::mmio_write;
                        variant_.emplace<mmio_write>(
                            run_ptr->mmio.phys_addr,
                            readonly_data(run_ptr->mmio.data, run_ptr->mmio.len)
                        );
                    } else {
                        reason_ = reason::mmio_read;
                        variant_.emplace<mmio_read>(
                            run_ptr->mmio.phys_addr,
                            writable_data(run_ptr->mmio.data, run_ptr->mmio.len)
                        );
                    }
                    break;

                case KVM_EXIT_UNKNOWN:
                    reason_ = reason::unknown;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_EXCEPTION:
                    reason_ = reason::exception;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_HYPERCALL:
                    reason_ = reason::hypercall;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_DEBUG:
                    reason_ = reason::debug;
                    variant_.emplace<debug_exit>(run_ptr->debug.arch);
                    break;

                case KVM_EXIT_HLT:
                    reason_ = reason::hlt;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_IRQ_WINDOW_OPEN:
                    reason_ = reason::irq_window_open;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_SHUTDOWN:
                    reason_ = reason::shutdown;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_FAIL_ENTRY:
                    reason_ = reason::fail_entry;
                    variant_.emplace<fail_entry>(run_ptr->fail_entry.hardware_entry_failure_reason,
                                                 run_ptr->fail_entry.cpu);
                    break;

                case KVM_EXIT_INTR:
                    reason_ = reason::intr;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_SET_TPR:
                    reason_ = reason::set_tpr;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_TPR_ACCESS:
                    reason_ = reason::tpr_access;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_S390_SIEIC:
                    reason_ = reason::s390_sieic;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_S390_RESET:
                    reason_ = reason::s390_reset;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_DCR:
                    reason_ = reason::dcr;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_NMI:
                    reason_ = reason::nmi;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_INTERNAL_ERROR:
                    reason_ = reason::internal_error;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_OSI:
                    reason_ = reason::osi;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_PAPR_HCALL:
                    reason_ = reason::papr_hcall;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_S390_UCONTROL:
                    reason_ = reason::s390_ucontrol;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_WATCHDOG:
                    reason_ = reason::watchdog;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_S390_TSCH:
                    reason_ = reason::s390_tsch;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_EPR:
                    reason_ = reason::epr;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_SYSTEM_EVENT:
                    reason_ = reason::system_event;
                    variant_.emplace<system_event>(run_ptr->system_event.type, run_ptr->system_event.flags);
                    break;

                case KVM_EXIT_S390_STSI:
                    reason_ = reason::s390_stsi;
                    variant_.emplace<std::monostate>();
                    break;

                case KVM_EXIT_IOAPIC_EOI:
                    reason_ = reason::ioapic_eoi;
                    variant_.emplace<ioapic_eoi>(run_ptr->eoi.vector);
                    break;

                case KVM_EXIT_HYPERV:
                    reason_ = reason::hyperv;
                    variant_.emplace<std::monostate>();
                    break;

                default:
                    reason_ = reason::unsupported;
                    variant_.emplace<uint32_t>(run_ptr->exit_reason);
                    break;
            }
        }

        [[nodiscard]] const io_out *get_as_io_out() const noexcept {
            LOG(trace) << "get_as_io_out";
            return std::get_if<io_out>(&variant_);
        }

        io_in *get_as_io_in() noexcept {
            LOG(trace) << "get_as_io_in";
            return std::get_if<io_in>(&variant_);
        }

        [[nodiscard]] const mmio_read *get_as_mmio_read() const noexcept {
            LOG(trace) << "get_as_mmio_read";
            return std::get_if<mmio_read>(&variant_);
        }

        mmio_write *get_as_mmio_write() noexcept {
            LOG(trace) << "get_as_mmio_write";
            return std::get_if<mmio_write>(&variant_);
        }

        // 通用访问器 - 使用 visitor 模式
        decltype(auto) visit(auto &&visitor) {
            return std::visit(std::forward<decltype(visitor)>(visitor), variant_);
        }

        decltype(auto) visit(auto &&visitor) const {
            return std::visit(std::forward<decltype(visitor)>(visitor), variant_);
        }


        reason reason_;

    private:
        std::variant<
            std::monostate, // 用于无数据的退出原因
            io_out,
            io_in,
            mmio_read,
            mmio_write,
            debug_exit,
            fail_entry,
            system_event,
            ioapic_eoi,
            uint32_t // 用于 unsupported
        > variant_;
    };


    struct vcpu_regs {
        static kvm_regs get_regs(int vcpu_fd) {
            kvm_regs regs = {};
            if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) == -1) {
                LOG(error) << "KVM_GET_REGS failed: " << strerror(errno);
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
                LOG(error) << "KVM_SET_REGS failed";
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
                LOG(error) << "KVM_GET_SREGS failed";
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
                LOG(error) << "KVM_SET_SREGS failed";
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
                LOG(error) << "KVM_GET_DEBUGREGS failed";
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
                LOG(error) << "KVM_SET_DEBUGREGS failed";
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "KVM_SET_DEBUGREGS failed"
                );
            }
        }
    };

    // TODO: add more explicit contributors
    struct vcpu_fd {
        file_descriptor vcpu{file_descriptor::INVALID};
        kvm_run_w kvm_run_ptr;

        vcpu_fd() = delete;

        vcpu_fd(file_descriptor &&vcpu_fd, kvm_run_w &&kvm_run): vcpu(std::move(vcpu_fd)),
                                                                 kvm_run_ptr(std::move(kvm_run)) {
            LOG(trace) << "vcpu_fd contribute: from file_descriptor and kvm_run_w";
        }

        vcpu_fd(vcpu_fd &&other) noexcept : vcpu(std::move(other.vcpu)),
                                            kvm_run_ptr(std::move(other.kvm_run_ptr)) {
            LOG(trace) << "vcpu_fd move contribute: from vcpu_fd&&";
            other.vcpu = file_descriptor{file_descriptor::INVALID};
            other.kvm_run_ptr = kvm_run_w::mmap_from_fd(file_descriptor::INVALID, -1);
        }

        auto operator=(vcpu_fd &&other) noexcept -> vcpu_fd & {
            LOG(trace) << "vcpu_fd move assignment: from vcpu_fd&&";
            vcpu = std::move(other.vcpu);
            kvm_run_ptr = std::move(other.kvm_run_ptr);
            other.vcpu = file_descriptor{file_descriptor::INVALID};
            other.kvm_run_ptr = kvm_run_w::mmap_from_fd(file_descriptor::INVALID, -1);
            return *this;
        }


        [[nodiscard]] auto run() const -> vcpu_exit {
            LOG(trace) << "Running vcpu";
            if (ioctl(vcpu.get(), KVM_RUN, 0) == -1) {
                if (errno == EINTR) {
                    return {};
                }
            }
            return vcpu_exit(kvm_run_ptr);
        }

        [[nodiscard]] kvm_regs get_registers() const {
            return vcpu_regs::get_regs(vcpu.get());
        }

        void set_registers(const kvm_regs &regs) const {
            vcpu_regs::set_regs(vcpu.get(), regs);
        }

        [[nodiscard]] kvm_sregs get_special_registers() const {
            return vcpu_regs::get_sregs(vcpu.get());
        }

        void set_special_registers(const kvm_sregs &sregs) const {
            vcpu_regs::set_sregs(vcpu.get(), sregs);
        }
    };

    // TODO: complete this struct
    struct vm_fd {
        file_descriptor vm;
        size_t run_size;

        [[nodiscard]] auto set_user_memory_region(kvm_userspace_memory_region& region) const -> eden_result<void> {
            if (const auto ret = ioctl(vm.get(),KVM_SET_USER_MEMORY_REGION, &region); ret != 0) return std::unexpected{
                std::error_code{INT8_MAX, std::generic_category()}
            };
            return {};
        }

        [[nodiscard]]auto set_tss_address(size_t offset) const ->eden_result<void>  {
            if (const auto ret = ioctl(vm.get(),KVM_SET_TSS_ADDR, offset);ret != 0) return std::unexpected{
                std::error_code{INT8_MAX, std::generic_category()}
            };
            return {};
        }

        [[nodiscard]]auto set_identity_map_address(u_int64_t address) const->eden_result<void> {
            if (const auto ret = ioctl(vm.get(),KVM_SET_IDENTITY_MAP_ADDR, &address);ret != 0) return std::unexpected{
                std::error_code{INT8_MAX, std::generic_category()}
            };
            return {};
        }



        [[nodiscard]] auto create_vcpu(uint8_t id) const -> eden_result<vcpu_fd> {
            if (const auto vcpu_fd_ = ioctl(vm.get(),KVM_CREATE_VCPU, id); vcpu_fd_ < 0) {
                return std::unexpected{std::error_code{vcpu_fd_, std::generic_category()}};
            } else {
                auto vcpu = file_descriptor{vcpu_fd_};
                auto kvm_run_ptr = kvm_run_w::mmap_from_fd(vcpu.get(), run_size);
                return vcpu_fd{std::move(vcpu), std::move(kvm_run_ptr)};
            }
        }
    };

    struct kvm_w {
        file_descriptor kvm_fd{file_descriptor::INVALID};

        explicit kvm_w() {
            kvm_fd = file_descriptor{open("/dev/kvm", O_RDWR | O_CLOEXEC)};
            std::cout << "KVM opened" << std::endl;
            if (kvm_fd.get() == -1) {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "open /dev/kvm failed"
                );
            }
        }

        [[nodiscard]] auto get_api_version() const -> int32_t {
            return ioctl(kvm_fd.get(), KVM_GET_API_VERSION);
        }

        // cannot accept rvalue, although it's safe!
        [[nodiscard]] REJECT_RVALUE auto get_vcpu_mmap_size(this auto &self) -> eden_result<size_t> {
            auto res = ioctl(self.kvm_fd.get(), KVM_GET_VCPU_MMAP_SIZE,0);
            if (res > 0) {
                return res;
            }
            return std::unexpected{std::error_code{res, std::generic_category()}};
        }

        [[nodiscard]] REJECT_RVALUE auto create_vm_with_type(this auto &self, uint32_t type) -> eden_result<vm_fd> {

            auto ret = ioctl(self.kvm_fd.get(), KVM_CREATE_VM, type);

            if (ret >= 0) {
                auto vm_file = file_descriptor{ret};
                if (auto run_mmap_size = self.get_vcpu_mmap_size();run_mmap_size) {
                    return vm_fd{(std::move(vm_file)), run_mmap_size.value()};
                }
                return std::unexpected{std::error_code{ret, std::generic_category()}};
            }
            return std::unexpected{std::error_code{ret, std::generic_category()}};
        }

        [[nodiscard]] REJECT_RVALUE auto create_vm(this auto &self) -> eden_result<vm_fd> {
            LOG(trace) << "Creating VM";
            return self.create_vm_with_type(0);
        }

        ~kvm_w() = default;
    };
}
