//
// Created by aoikajitsu on 25-6-15.
//
module;
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/mman.h>
export module address_space:region;
import util;
import :address;
import :space;
using namespace eden_virt::util;
export namespace eden_virt::address_space {
    struct file_backend {
        file_descriptor file;
        uint64_t offset;
        uint64_t page_size;
    };

    struct host_mem_mapping {
        address_range a_ran{-1, -1};
        void *h_a;
        std::optional<file_backend> file_back{std::nullopt};
        bool is_share{false};


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
        auto size() const ->uint64_t {
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
    using read_fn = std::unique_ptr<std::function<bool(std::vector<uint8_t>&, guest_address, uint64_t)>>;
    using write_fn = std::unique_ptr<std::function<bool(const std::vector<uint8_t>&, guest_address, uint64_t)>>;
    struct region_ops {
        read_fn read;
        write_fn write;
    };

    // represents a memory region, used by mem-mapped IO, Ram or Rom.
    struct region {
        std::string name;
        region_type type;
        std::atomic<int32_t> priority;
        std::atomic<uint64_t> size;
        mutex_data<guest_address> offset;

        /// 内存映射 (非 RAM/RomDevice/RamDevice 类型时为 nullopt)
        std::optional<std::shared_ptr<host_mem_mapping> > mem_mapping;

        /// 读写操作函数
        std::optional<region_ops> ops;

        /// 指向父地址空间的弱指针
        shared_mutex_data<std::weak_ptr<address_space>> space;

        /// List of sub-areas (keep sorted)
        std::vector<shared_mutex_data<std::shared_ptr<region>>> subregions;

        /// ROM 设备模式标志 (true: 只读模式, false: IO 模式)
        std::atomic<bool> rom_dev_romd;

        /// 支持的最大访问尺寸
        std::optional<uint64_t> max_access_size;

        /// 原始内存区域别名
        std::optional<std::shared_ptr<region> > alias;

        /// 在别名区域中的偏移量
        uint64_t alias_offset;
    };
    struct flat_range {
        address_range addr_range;
        region owner;
        uint64_t offset_in_region;
        std::optional<bool> rom_dev_romd;
    };

}
