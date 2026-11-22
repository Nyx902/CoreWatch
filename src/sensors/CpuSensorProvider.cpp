#include "corewatch/sensors/CpuSensorProvider.hpp"
#include "corewatch/core/Format.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <powerbase.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace corewatch::sensors {

namespace {

struct ProcessorPowerInfo {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
};

struct PdhQueryDeleter {
    void operator()(PDH_HQUERY query) const noexcept {
        if (query != nullptr) {
            PdhCloseQuery(query);
        }
    }
};

using UniquePdhQuery = std::unique_ptr<std::remove_pointer_t<PDH_HQUERY>, PdhQueryDeleter>;

std::uint64_t filetime_to_u64(const FILETIME& ft) noexcept {
    return (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) |
           static_cast<std::uint64_t>(ft.dwLowDateTime);
}

std::string read_cpu_brand_string() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0,
            KEY_READ,
            &key) != ERROR_SUCCESS) {
        return "Unknown CPU";
    }

    wchar_t buffer[256]{};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LSTATUS status = RegQueryValueExW(
        key, L"ProcessorNameString", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        return "Unknown CPU";
    }

    std::string name = corewatch::core::wide_to_utf8(buffer);
    // Trim leading/trailing whitespace
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "Unknown CPU";
    }
    const auto last = name.find_last_not_of(" \t\r\n");
    return name.substr(first, last - first + 1);
}

std::pair<std::uint32_t, std::uint32_t> query_core_counts() {
    DWORD buffer_size = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &buffer_size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || buffer_size == 0) {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        return {si.dwNumberOfProcessors, si.dwNumberOfProcessors};
    }

    std::vector<std::uint8_t> buffer(buffer_size);
    if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
            &buffer_size)) {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        return {si.dwNumberOfProcessors, si.dwNumberOfProcessors};
    }

    std::uint32_t physical_cores = 0;
    std::uint32_t logical_cores = 0;
    std::size_t offset = 0;
    while (offset < buffer_size) {
        const auto* info = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            buffer.data() + offset);
        if (info->Relationship == RelationProcessorCore) {
            ++physical_cores;
            for (WORD g = 0; g < info->Processor.GroupCount; ++g) {
                KAFFINITY mask = info->Processor.GroupMask[g].Mask;
                while (mask != 0) {
                    logical_cores += static_cast<std::uint32_t>(mask & 1ULL);
                    mask >>= 1;
                }
            }
        }
        offset += info->Size;
    }

    return {physical_cores, logical_cores};
}

} // namespace

struct CpuSensorProvider::Impl {
    std::string cpu_name{"Unknown CPU"};
    std::uint32_t physical_cores{0};
    std::uint32_t logical_cores{0};

    bool has_prev_times{false};
    std::uint64_t prev_idle{0};
    std::uint64_t prev_kernel{0};
    std::uint64_t prev_user{0};

    UniquePdhQuery pdh_query{nullptr};
    PDH_HCOUNTER per_core_counter{nullptr};
    bool pdh_primed{false};
};

CpuSensorProvider::CpuSensorProvider()
    : impl_(std::make_unique<Impl>()) {}

CpuSensorProvider::~CpuSensorProvider() = default;

bool CpuSensorProvider::initialize() {
    impl_->cpu_name = read_cpu_brand_string();
    auto [physical, logical] = query_core_counts();
    impl_->physical_cores = physical;
    impl_->logical_cores = logical;

    FILETIME idle_ft{}, kernel_ft{}, user_ft{};
    if (GetSystemTimes(&idle_ft, &kernel_ft, &user_ft)) {
        impl_->prev_idle = filetime_to_u64(idle_ft);
        impl_->prev_kernel = filetime_to_u64(kernel_ft);
        impl_->prev_user = filetime_to_u64(user_ft);
        impl_->has_prev_times = true;
    }

    PDH_HQUERY raw_query = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &raw_query) == ERROR_SUCCESS) {
        impl_->pdh_query.reset(raw_query);
        if (PdhAddEnglishCounterW(
                impl_->pdh_query.get(),
                L"\\Processor(*)\\% Processor Time",
                0,
                &impl_->per_core_counter) == ERROR_SUCCESS) {
            PdhCollectQueryData(impl_->pdh_query.get());
            impl_->pdh_primed = true;
        } else {
            impl_->pdh_query.reset();
            impl_->per_core_counter = nullptr;
        }
    }

    initialized_ = true;
    return true;
}

void CpuSensorProvider::poll(corewatch::core::SystemSnapshot& snapshot) {
    auto& cpu = snapshot.cpu;
    cpu.name = impl_->cpu_name;
    cpu.physical_core_count = impl_->physical_cores;
    cpu.logical_core_count = impl_->logical_cores;

    // 1. Total CPU utilization via GetSystemTimes delta
    FILETIME idle_ft{}, kernel_ft{}, user_ft{};
    if (GetSystemTimes(&idle_ft, &kernel_ft, &user_ft)) {
        const auto idle = filetime_to_u64(idle_ft);
        const auto kernel = filetime_to_u64(kernel_ft);
        const auto user = filetime_to_u64(user_ft);

        if (impl_->has_prev_times) {
            const auto d_idle = idle - impl_->prev_idle;
            const auto d_kernel = kernel - impl_->prev_kernel;
            const auto d_user = user - impl_->prev_user;
            const auto d_total = d_kernel + d_user; // kernel includes idle
            if (d_total > 0) {
                const double busy = static_cast<double>(d_total - d_idle);
                const double pct = std::clamp((busy / static_cast<double>(d_total)) * 100.0, 0.0, 100.0);
                cpu.total_utilization_percent = corewatch::core::SensorValue<double>::available(pct);
            }
        }
        impl_->prev_idle = idle;
        impl_->prev_kernel = kernel;
        impl_->prev_user = user;
        impl_->has_prev_times = true;
    }

    // 2. Logical processor utilization via PDH
    cpu.logical_cores.clear();
    if (impl_->pdh_query && impl_->per_core_counter && impl_->pdh_primed) {
        if (PdhCollectQueryData(impl_->pdh_query.get()) == ERROR_SUCCESS) {
            DWORD buf_size = 0;
            DWORD item_count = 0;
            const PDH_STATUS st = PdhGetFormattedCounterArrayW(
                impl_->per_core_counter, PDH_FMT_DOUBLE, &buf_size, &item_count, nullptr);
            if (st == static_cast<PDH_STATUS>(PDH_MORE_DATA) && buf_size > 0) {
                std::vector<std::uint8_t> buf(buf_size);
                auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
                if (PdhGetFormattedCounterArrayW(
                        impl_->per_core_counter, PDH_FMT_DOUBLE, &buf_size, &item_count, items) == ERROR_SUCCESS) {
                    for (DWORD i = 0; i < item_count; ++i) {
                        if (items[i].szName == nullptr) {
                            continue;
                        }
                        const std::wstring inst_name(items[i].szName);
                        if (inst_name == L"_Total") {
                            continue;
                        }
                        try {
                            const auto idx = static_cast<std::uint32_t>(std::stoul(inst_name));
                            if (items[i].FmtValue.CStatus == ERROR_SUCCESS) {
                                const double val = std::clamp(items[i].FmtValue.doubleValue, 0.0, 100.0);
                                cpu.logical_cores.push_back(corewatch::core::LogicalCoreReading{
                                    idx,
                                    corewatch::core::SensorValue<double>::available(val)
                                });
                            }
                        } catch (...) {
                            // Ignore non-numeric instance names (e.g., NUMA/group prefixes if any)
                        }
                    }
                    std::sort(cpu.logical_cores.begin(), cpu.logical_cores.end(),
                        [](const auto& a, const auto& b) { return a.core_index < b.core_index; });
                }
            }
        }
    }

    // 3. Clock speed via CallNtPowerInformation
    if (impl_->logical_cores > 0) {
        std::vector<ProcessorPowerInfo> power_info(impl_->logical_cores);
        const LONG nt_status = CallNtPowerInformation(
            ProcessorInformation,
            nullptr,
            0,
            power_info.data(),
            static_cast<ULONG>(power_info.size() * sizeof(ProcessorPowerInfo)));
        if (nt_status == 0 && !power_info.empty()) {
            double sum_current = 0.0;
            double max_mhz = 0.0;
            for (const auto& info : power_info) {
                sum_current += static_cast<double>(info.CurrentMhz);
                max_mhz = std::max(max_mhz, static_cast<double>(info.MaxMhz));
            }
            const double avg_current = sum_current / static_cast<double>(power_info.size());
            if (avg_current > 0.0) {
                cpu.current_clock_mhz = corewatch::core::SensorValue<double>::available(avg_current);
            }
            if (max_mhz > 0.0) {
                cpu.max_clock_mhz = corewatch::core::SensorValue<double>::available(max_mhz);
            }
        }
    }

    // Temperature and Power are NOT available via standard unprivileged Win32 APIs without kernel drivers.
    // Never invent sensor readings.
    cpu.temperature_celsius = corewatch::core::SensorValue<double>::unavailable();
    cpu.power_watts = corewatch::core::SensorValue<double>::unavailable();
}

} // namespace corewatch::sensors
