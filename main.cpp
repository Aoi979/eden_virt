#include <fcntl.h>
#include <iostream>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <expected>
int main() {
    std::expected<int,int> test = std::unexpected(1);
    std::cout << "Hello, World!" << std::endl;
    return 0;
}