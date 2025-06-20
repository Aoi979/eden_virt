//
// Created by aoikajitsu on 25-6-20.
//
module;
#include <string>
#include <memory>
export module address_space:space;
import :region;
export namespace eden_virt::address_space {
    struct flat_view {
        std::vector<flat_range> view;
        auto find_flat_range() -> eden_result<flat_range&> {
        };
    };

    struct address_space {
        std::string space_name;
        region root;
        // [C++20] std::atomic<std::shared_ptr<T>> is specified, is_lock_free:false
        std::atomic<std::shared_ptr<flat_view>> view;
        /// The backend memory region tree, used for migrate.
        std::optional<std::shared_ptr<region>> machine_ram;
    };
}