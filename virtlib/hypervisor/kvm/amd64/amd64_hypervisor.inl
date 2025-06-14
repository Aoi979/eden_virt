        void arch_init() const {
            constexpr uint64_t identity_addr = 0xFEF0C000;
            const auto vm_fd_ptr = vm_fd_.get();
            if (auto res = vm_fd_ptr->set_identity_map_address(identity_addr)) {

            }
            if (auto res2 = vm_fd_ptr->set_tss_address(identity_addr+0x1000)) {

            }
        }