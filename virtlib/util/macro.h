//
// Created by aoikajitsu on 25-6-14.
//

#ifndef MACRO_H
#define MACRO_H
#define REJECT_RVALUE
#define IMPL
#define HYPERVISOR_OPS_BEGIN
#define HYPERVISOR_OPS_END
#define DYNAMIC
template <typename T>
constexpr const char* get_type_name() {
#if defined(__clang__)
    return __PRETTY_FUNCTION__;  // Clang 格式
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;  // GCC 格式
#else
    return "Unsupported compiler";
#endif
}

#define LOG(level) log_stream(log_level::level, __FILE__, __LINE__, __func__)
#endif //MACRO_H
