//
// Created by aoikajitsu on 25-6-16.
//
module;
#include <cstdint>
#include <optional>
#include <cassert>
#include <tuple>
#include <algorithm>
export module address_space:address;
export namespace eden_virt::address_space {
       struct guest_address {
        std::uint64_t address;

        auto operator<=>(const guest_address &) const = default;

        operator uint64_t() const noexcept { return address; }

        auto offset_from(guest_address other) const -> uint64_t {
            return address - other.address;
        }

        auto checked_add(uint64_t offset) const -> std::optional<guest_address> {
            if (offset > UINT64_MAX - address)
                return std::nullopt;
            return guest_address{address + offset};
        }

        auto checked_sub(uint64_t offset) const -> std::optional<guest_address> {
            if (offset > address)
                return std::nullopt;
            return guest_address{address - offset};
        }

        auto unchecked_add(uint64_t offset) const -> guest_address {
            return guest_address{address + offset};
        }

        auto unchecked_sub(uint64_t offset) const -> guest_address {
            return guest_address{address - offset};
        }

        auto align_down(uint64_t alignment) const -> guest_address {
            assert((alignment & (alignment - 1)) == 0 && "alignment must be power of two");
            return guest_address{address & ~(alignment - 1)};
        }

        auto align_up(uint64_t alignment) const -> guest_address {
            assert((alignment & (alignment - 1)) == 0 && "alignment must be power of two");
            return guest_address{(address + alignment - 1) & ~(alignment - 1)};
        }
    };

    struct address_range {
        guest_address base;
        uint64_t size;

        address_range() = default;

        auto operator<=>(const address_range &) const = default;

        address_range(guest_address b, uint64_t s) : base(b), size(s) {
        }

        address_range(const std::tuple<guest_address, uint64_t> &tup)
            : base(std::get<0>(tup)), size(std::get<1>(tup)) {
        }

        operator std::tuple<guest_address, uint64_t>() const {
            return std::make_tuple(base, size);
        }

        static auto find_intersection(const address_range &a, const address_range &b)->std::optional<address_range> {
            uint64_t start1 = a.base;
            uint64_t end1 = start1 + a.size;

            uint64_t start2 = b.base;
            uint64_t end2 = start2 + b.size;

            uint64_t inter_start = std::max(start1, start2);
            uint64_t inter_end = std::min(end1, end2);

            if (inter_end > inter_start) {
                return address_range{guest_address{inter_start}, inter_end - inter_start};
            }
            return std::nullopt;
        }
    };


    template<std::size_t N>
    decltype(auto) get(const address_range &r) {
        if constexpr (N == 0) return r.base;
        else if constexpr (N == 1) return r.size;
        else static_assert(N < 2, "Index out of range");
    }

    template<std::size_t N>
    decltype(auto) get(address_range &r) {
        if constexpr (N == 0) return r.base;
        else if constexpr (N == 1) return r.size;
        else static_assert(N < 2, "Index out of range");
    }
}

export namespace std {
    template<>
    struct tuple_size<eden_virt::address_space::address_range> : std::integral_constant<std::size_t, 2> {
    };

    template<>
    struct tuple_element<0, eden_virt::address_space::address_range> {
        using type = eden_virt::address_space::address_range;
    };

    template<>
    struct tuple_element<1, eden_virt::address_space::address_range> {
        using type = uint64_t;
    };
}
