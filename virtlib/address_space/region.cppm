//
// Created by aoikajitsu on 25-6-15.
//
module;
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/mman.h>
export module region;
import util;
import address_space;
using namespace eden_virt::util;
export namespace eden_virt::address_space {
    struct file_backend {
        std::shared_ptr<file_descriptor> file;
        uint64_t offset;
        uint64_t page_size;
    };

    struct host_mem_mapping {
        address_range a_ran{-1,-1};
        void *h_a;
        std::optional<file_backend> file_back{std::nullopt};
        bool is_share{false};


        host_mem_mapping(guest_address g_addr,uint64_t size, std::optional<uint64_t> host_addr, std::optional<file_backend> file_back,
                         bool dump_guest_core, bool is_share_, bool read_only):is_share(is_share_) {
            if (host_addr) {
                a_ran = address_range{g_addr, size};
                h_a = reinterpret_cast<void*>(host_addr.value());
            }else {
                // unsupported!
                std::abort();
            }
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

    // represents a memory region, used by mem-mapped IO, Ram or Rom.
    struct region {
        std::string name;

    private:
        region_type type;
        std::shared_ptr<std::atomic<int32_t> > priority;
        std::shared_ptr<std::atomic<uint64_t> > size;
        std::shared_ptr<mutex_data<guest_address> > offset;

        /// 内存映射 (非 RAM/RomDevice/RamDevice 类型时为 nullopt)
        std::optional<std::shared_ptr<host_mem_mapping> > mem_mapping;

        /// 读写操作函数
        std::optional<region_ops> ops;


        /// 指向父地址空间的弱指针
        // std::shared_ptr<std::shared_mutex> space_rwlock;
        // std::weak_ptr<address_space> space;

        /// 子区域列表 (保持排序)
        // std::shared_ptr<std::shared_mutex> subregions_rwlock;
        // std::shared_ptr<std::vector<std::shared_ptr<region> > > subregions;

        /// ROM 设备模式标志 (true: 只读模式, false: IO 模式)
        std::shared_ptr<std::atomic<bool> > rom_dev_romd;

        /// 支持的最大访问尺寸
        std::optional<uint64_t> max_access_size;

        /// 原始内存区域别名
        std::optional<std::shared_ptr<region> > alias;

        /// 在别名区域中的偏移量
        uint64_t alias_offset;
    };
}
