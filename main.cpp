#include <iostream>
#include "virtlib/util/macro.h"
import eden_machine;
import eden_hypervisor;
import util;

using namespace eden_virt::hypervisor::kvm;
using namespace eden_virt::util;


int main() {
    LOG(trace) << "start";
    auto eden_machine = eden_virt::machine::eden_machine<kvm_hypervisor>();
    LOG(trace) << "end";
    return 0;
}