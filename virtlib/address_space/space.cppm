//
// Created by aoikajitsu on 25-6-20.
//
module;
#include <atomic>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <optional>
#include <sys/mman.h>
export module address_space:space;
import util;
import :address;
using namespace eden_virt::util;
export namespace eden_virt::address_space {
    struct address_space;

    struct file_backend {
        file_descriptor file{file_descriptor::INVALID};
        uint64_t offset{0};
        uint64_t page_size{0};
    };

    struct host_mem_mapping {
        address_range a_ran{static_cast<guest_address>(-1), static_cast<uint64_t>(-1)};
        void *h_a;
        std::optional<file_backend> file_back{std::nullopt};
        bool is_share{false};

        host_mem_mapping(const host_mem_mapping &) = delete;

        host_mem_mapping(host_mem_mapping &&) = default;


        host_mem_mapping(guest_address g_addr, uint64_t size, std::optional<uint64_t> host_addr,
                         std::optional<file_backend> file_back,
                         bool dump_guest_core, bool is_share_, bool read_only): is_share(is_share_) {
            if (host_addr) {
                a_ran = address_range{g_addr, size};
                h_a = reinterpret_cast<void *>(host_addr.value());
            } else {
                // unsupported!
                std::abort();
            }
        }

        [[nodiscard]] auto size() const -> uint64_t {
            return a_ran.size;
        }

        ~host_mem_mapping() {
            munmap(h_a, a_ran.size);
        }
    };

    enum class region_type {
        ram,
        io,
        container,
        rom_device,
        ram_device,
        alias,
    };

    // Only valid for Mmio Region
    using read_fn = std::unique_ptr<std::function<bool(std::vector<uint8_t> &, guest_address, uint64_t)> >;
    using write_fn = std::unique_ptr<std::function<bool(const std::vector<uint8_t> &, guest_address, uint64_t)> >;

    struct region_ops {
        read_fn read{nullptr};
        write_fn write{nullptr};
    };

    // represents a memory region, used by mem-mapped IO, Ram or Rom.
    struct region {
        std::string name;
        region_type type;
        int32_t priority{0};
        uint64_t size;
        guest_address offset{0};

        std::optional<host_mem_mapping> mem_mapping{std::nullopt};

        // Only valid for Mmio Region
        std::optional<region_ops> ops{std::nullopt};

        /// 指向父地址空间的弱指针
        shared_mutex_data<std::weak_ptr<address_space> > space{};

        /// List of sub-areas (keep sorted)
        std::vector<shared_mutex_data<std::shared_ptr<region> > > subregions{};

        // ROM device mode flag (true: read-only mode, false: IO mode)
        bool rom_dev_romd{false};

        // 支持的最大访问尺寸
        std::optional<uint64_t> max_access_size{std::nullopt};

        // 原始内存区域别名
        std::optional<std::shared_ptr<region> > alias{std::nullopt};

        // 在别名区域中的偏移量
        uint64_t alias_offset{0};

        static auto init_ram_region(std::string_view n, host_mem_mapping &&mem_mapping_) -> region {
            return region{n, mem_mapping_.size(), region_type::ram, std::move(mem_mapping_), std::nullopt};
        }

    private:
        region(std::string_view name_, uint64_t size_, region_type type_,
               std::optional<host_mem_mapping> &&mem_mapping_,
               std::optional<region_ops> &&ops_): name(name_), type(type_), size(size_),
                                                  mem_mapping(std::move(mem_mapping_)),
                                                  ops(std::move(ops_)) {
        }
    };

    struct flat_range {
        address_range addr_range{guest_address(0), 0};
        region *owner{nullptr};
        uint64_t offset_in_region{0};
        std::optional<bool> rom_dev_romd{false};
    };

    struct flat_view {
        std::vector<flat_range> view{};
        // auto find_flat_range() -> eden_result<flat_range&> {
        // };
    };

    struct address_space {
        std::string space_name;
        region root;
        shared_mutex_data<std::shared_ptr<flat_view>> view;
        /// The backend memory region tree, used for migrate.
        std::optional<std::shared_ptr<region> > machine_ram;
    };
}
