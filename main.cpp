#include <fcntl.h>
#include <iostream>
#include <linux/kvm.h>
#include <sys/ioctl.h>
int main() {
    auto kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    auto vm_fd = ioctl(kvm_fd,KVM_CREATE_VM);

    std::cout << "Hello, World!" << std::endl;
    return 0;
}