//
// Created by aoikajitsu on 25-6-13.
//
#include <gtest/gtest.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <print>
import kvm_util;
import util;
using namespace eden_virt::util;
using namespace eden_virt::util::kvm;
#define FAKE_FD 42

// Mock file_descriptor class for testing
class mock_file_descriptor {
public:
    mock_file_descriptor() : fd_(-1) {
    }

    explicit mock_file_descriptor(int fd) : fd_(fd) {
    }

    mock_file_descriptor(mock_file_descriptor &&other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    ~mock_file_descriptor() {
        if (fd_ != -1) {
            close(fd_);
        }
    }

    int get() const { return fd_; }
    operator int() const { return fd_; }

    static constexpr int INVALID = -1;

private:
    int fd_;
};

TEST(KvmRunWrapperTest, MoveSemantics) {
    // Create a temporary file for mapping
    int fd = memfd_create("kvm_run", 0);
    ASSERT_GE(fd, 0);

    // Set size
    const size_t size = 4096;
    ftruncate(fd, size);

    // Create first wrapper
    kvm_run_w wrapper1 = kvm_run_w::mmap_from_fd(fd, size);
    ASSERT_TRUE(wrapper1.valid());

    void *original_ptr = wrapper1.as_ptr();

    // Test move constructor
    kvm_run_w wrapper2 = std::move(wrapper1);
    EXPECT_FALSE(wrapper1.valid());
    EXPECT_TRUE(wrapper2.valid());
    EXPECT_EQ(wrapper2.as_ptr(), original_ptr);

    // Test move assignment
    kvm_run_w wrapper3 = kvm_run_w::mmap_from_fd(FAKE_FD, size);
    wrapper3 = std::move(wrapper2);
    EXPECT_FALSE(wrapper2.valid());
    EXPECT_TRUE(wrapper3.valid());
    EXPECT_EQ(wrapper3.as_ptr(), original_ptr);
}

TEST(KvmRunWrapperTest, InvalidOperations) {
    // Test invalid fd
    EXPECT_THROW(kvm_run_w::mmap_from_fd(-1, 4096), std::invalid_argument);

    // Test invalid mapping
    int fd = memfd_create("kvm_run_invalid", 0);
    ASSERT_GE(fd, 0);

    EXPECT_THROW(kvm_run_w::mmap_from_fd(fd, 0), std::system_error);
}

TEST(VcpuExitTest, DefaultConstruct) {
    vcpu_exit exit;
    EXPECT_EQ(exit.reason_, vcpu_exit::reason::unsupported);
    //EXPECT_TRUE(std::holds_alternative<std::monostate>(exit.variant_));
}

TEST(VcpuExitTest, IoOutHandling) {
    // Prepare mock kvm_run struct in a mapped memory region
    const size_t size = 4096;
    void *mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    ASSERT_NE(mem, MAP_FAILED);

    auto *run = static_cast<kvm_run *>(mem);
    run->exit_reason = KVM_EXIT_IO;
    run->io.direction = KVM_EXIT_IO_OUT;
    run->io.port = 0x60;
    run->io.size = 1;
    run->io.count = 4;
    run->io.data_offset = offsetof(kvm_run, io) + sizeof(run->io.data_offset);

    // Simulate data in the buffer
    uint8_t *data_ptr = reinterpret_cast<uint8_t *>(run) + run->io.data_offset;
    constexpr uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    std::copy_n(test_data, 4, data_ptr);

    // Wrap the memory region
    kvm_run_w wrapper = kvm_run_w::mmap_from_fd(FAKE_FD, size);
    // NOTE: In real code we can't do this, but for testing we'll set private members
    // This would require friend declaration in kvm_run_w or refactoring
    // We'll assume we have a test helper to create from raw pointer for testing

    // Create vcpu_exit
    vcpu_exit exit(wrapper);

    EXPECT_EQ(exit.reason_, vcpu_exit::reason::io_out);

    const io_out *io = exit.get_as_io_out();
    ASSERT_NE(io, nullptr);
    EXPECT_EQ(io->port, 0x60);
    ASSERT_EQ(io->data.size(), 4);
    EXPECT_EQ(io->data[0], 0x01);
    EXPECT_EQ(io->data[1], 0x02);
    EXPECT_EQ(io->data[2], 0x03);
    EXPECT_EQ(io->data[3], 0x04);

    munmap(mem, size);
}

TEST(VcpuExitTest, IoInHandling) {
    // Prepare mock kvm_run struct
    const size_t size = 4096;
    void *mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    ASSERT_NE(mem, MAP_FAILED);

    auto *run = static_cast<kvm_run *>(mem);
    run->exit_reason = KVM_EXIT_IO;
    run->io.direction = KVM_EXIT_IO_IN;
    run->io.port = 0x64;
    run->io.size = 1;
    run->io.count = 2;
    run->io.data_offset = offsetof(kvm_run, io) + sizeof(run->io.data_offset);

    // Wrap the memory region
    kvm_run_w wrapper = kvm_run_w::mmap_from_fd(FAKE_FD, size);
    // [Same note about test helper as previous test]

    // Create vcpu_exit
    vcpu_exit exit(wrapper);

    EXPECT_EQ(exit.reason_, vcpu_exit::reason::io_in);

    io_in *io = exit.get_as_io_in();
    ASSERT_NE(io, nullptr);
    EXPECT_EQ(io->port, 0x64);
    EXPECT_EQ(io->data.size(), 2);

    // Test writable aspect - the data should be backed by our memory
    uint8_t *data_ptr = reinterpret_cast<uint8_t *>(run) + run->io.data_offset;
    data_ptr[0] = 0xAA;
    data_ptr[1] = 0xBB;

    EXPECT_EQ(io->data[0], 0xAA);
    EXPECT_EQ(io->data[1], 0xBB);

    io->data[0] = 0xCC;
    io->data[1] = 0xDD;

    EXPECT_EQ(data_ptr[0], 0xCC);
    EXPECT_EQ(data_ptr[1], 0xDD);

    munmap(mem, size);
}

TEST(VcpuRegsTest, GetSetRegisters) {
    // This test requires KVM enabled and appropriate permissions
    // We'll skip execution on systems without /dev/kvm
    if (access("/dev/kvm", F_OK) == -1) {
        GTEST_SKIP() << "KVM not available, skipping vcpu_regs tests";
    }

    int sys_fd = open("/dev/kvm", O_RDWR);
    ASSERT_GE(sys_fd, 0);

    int vm_fd = ioctl(sys_fd, KVM_CREATE_VM, 0);
    ASSERT_GE(vm_fd, 0);

    // Create VCPU
    int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
    ASSERT_GE(vcpu_fd, 0);

    // Test getting/setting regs
    kvm_regs regs = vcpu_regs::get_regs(vcpu_fd);
    regs.rax = 0x12345678;
    vcpu_regs::set_regs(vcpu_fd, regs);

    kvm_regs regs2 = vcpu_regs::get_regs(vcpu_fd);
    EXPECT_EQ(regs2.rax, 0x12345678);

    // Test getting/setting sregs
    kvm_sregs sregs = vcpu_regs::get_sregs(vcpu_fd);
    sregs.cs.base = 0x1000;
    vcpu_regs::set_sregs(vcpu_fd, sregs);

    kvm_sregs sregs2 = vcpu_regs::get_sregs(vcpu_fd);
    EXPECT_EQ(sregs2.cs.base, 0x1000);

    close(vcpu_fd);
    close(vm_fd);
    close(sys_fd);
}

TEST(VcpuFdTest, MoveSemantics) {
    // Create a temporary fd
    int fd = memfd_create("vcpu", 0);
    ASSERT_GE(fd, 0);

    // Create kvm_run wrapper
    kvm_run_w run_wrapper = kvm_run_w::mmap_from_fd(fd, 4096);
    // Create original vcpu_fd
    file_descriptor original_fd(fd);
    vcpu_fd vcpu1(std::move(original_fd), std::move(run_wrapper));

    int original_fd_value = vcpu1.vcpu.get();

    // Test move constructor
    vcpu_fd vcpu2(std::move(vcpu1));
    EXPECT_EQ(vcpu1.vcpu.get(), file_descriptor::INVALID);
    EXPECT_EQ(vcpu2.vcpu.get(), original_fd_value);
    EXPECT_TRUE(vcpu2.kvm_run_ptr.valid());


    // Test move assignment
    // vcpu_fd vcpu3;
    // vcpu3 = std::move(vcpu2);
    // EXPECT_EQ(vcpu2.vcpu.get(), file_descriptor::INVALID);
    // EXPECT_EQ(vcpu3.vcpu.get(), original_fd_value);
    // EXPECT_TRUE(vcpu3.kvm_run_ptr.valid());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
