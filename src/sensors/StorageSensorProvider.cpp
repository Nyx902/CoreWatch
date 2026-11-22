#include "corewatch/sensors/StorageSensorProvider.hpp"
#include "corewatch/core/Format.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>

#include <algorithm>
#include <string>
#include <vector>

namespace corewatch::sensors {

namespace {

struct PdhQueryDeleter {
    void operator()(PDH_HQUERY query) const noexcept {
        if (query != nullptr) {
            PdhCloseQuery(query);
        }
    }
};

using UniquePdhQuery = std::unique_ptr<std::remove_pointer_t<PDH_HQUERY>, PdhQueryDeleter>;

} // namespace

struct StorageSensorProvider::Impl {
    UniquePdhQuery pdh_query{nullptr};
    PDH_HCOUNTER active_time_counter{nullptr};
    PDH_HCOUNTER read_bytes_counter{nullptr};
    PDH_HCOUNTER write_bytes_counter{nullptr};
    bool pdh_primed{false};
};

StorageSensorProvider::StorageSensorProvider()
    : impl_(std::make_unique<Impl>()) {}

StorageSensorProvider::~StorageSensorProvider() = default;

bool StorageSensorProvider::initialize() {
    PDH_HQUERY raw_query = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &raw_query) == ERROR_SUCCESS) {
        impl_->pdh_query.reset(raw_query);
        const bool ok_active = PdhAddEnglishCounterW(
            impl_->pdh_query.get(),
            L"\\PhysicalDisk(_Total)\\% Disk Time",
            0,
            &impl_->active_time_counter) == ERROR_SUCCESS;
        const bool ok_read = PdhAddEnglishCounterW(
            impl_->pdh_query.get(),
            L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec",
            0,
            &impl_->read_bytes_counter) == ERROR_SUCCESS;
        const bool ok_write = PdhAddEnglishCounterW(
            impl_->pdh_query.get(),
            L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec",
            0,
            &impl_->write_bytes_counter) == ERROR_SUCCESS;

        if (ok_active || ok_read || ok_write) {
            PdhCollectQueryData(impl_->pdh_query.get());
            impl_->pdh_primed = true;
        }
    }

    initialized_ = true;
    return true;
}

void StorageSensorProvider::poll(corewatch::core::SystemSnapshot& snapshot) {
    auto& storage = snapshot.storage;
    storage.volumes.clear();

    // Enumerate fixed logical drives
    const DWORD needed = GetLogicalDriveStringsW(0, nullptr);
    if (needed > 0) {
        std::wstring drives(needed + 1, L'\0');
        if (GetLogicalDriveStringsW(needed, drives.data()) > 0) {
            const wchar_t* ptr = drives.c_str();
            while (*ptr != L'\0') {
                const std::wstring root(ptr);
                if (GetDriveTypeW(root.c_str()) == DRIVE_FIXED) {
                    corewatch::core::StorageVolumeSnapshot vol{};
                    vol.mount_point = corewatch::core::wide_to_utf8(root);

                    wchar_t label[MAX_PATH + 1]{};
                    wchar_t fs_name[MAX_PATH + 1]{};
                    if (GetVolumeInformationW(
                            root.c_str(),
                            label, MAX_PATH,
                            nullptr, nullptr, nullptr,
                            fs_name, MAX_PATH)) {
                        vol.volume_label = corewatch::core::wide_to_utf8(label);
                        vol.file_system = corewatch::core::wide_to_utf8(fs_name);
                    }

                    ULARGE_INTEGER avail_to_caller{};
                    ULARGE_INTEGER total_bytes{};
                    ULARGE_INTEGER free_bytes{};
                    if (GetDiskFreeSpaceExW(root.c_str(), &avail_to_caller, &total_bytes, &free_bytes)) {
                        const std::uint64_t total = total_bytes.QuadPart;
                        const std::uint64_t free_b = free_bytes.QuadPart;
                        const std::uint64_t used = (total >= free_b) ? (total - free_b) : 0ULL;
                        vol.total_bytes = corewatch::core::SensorValue<std::uint64_t>::available(total);
                        vol.free_bytes = corewatch::core::SensorValue<std::uint64_t>::available(free_b);
                        vol.used_bytes = corewatch::core::SensorValue<std::uint64_t>::available(used);
                        if (total > 0) {
                            const double pct = std::clamp(
                                (static_cast<double>(used) / static_cast<double>(total)) * 100.0,
                                0.0, 100.0);
                            vol.usage_percent = corewatch::core::SensorValue<double>::available(pct);
                        }
                    }

                    storage.volumes.push_back(std::move(vol));
                }
                ptr += root.size() + 1;
            }
        }
    }

    // Collect physical disk performance counters
    if (impl_->pdh_query && impl_->pdh_primed) {
        if (PdhCollectQueryData(impl_->pdh_query.get()) == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE val{};
            if (impl_->active_time_counter &&
                PdhGetFormattedCounterValue(impl_->active_time_counter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS &&
                val.CStatus == ERROR_SUCCESS) {
                storage.disk_active_time_percent =
                    corewatch::core::SensorValue<double>::available(std::clamp(val.doubleValue, 0.0, 100.0));
            }
            if (impl_->read_bytes_counter &&
                PdhGetFormattedCounterValue(impl_->read_bytes_counter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS &&
                val.CStatus == ERROR_SUCCESS) {
                storage.read_bytes_per_sec =
                    corewatch::core::SensorValue<double>::available(std::max(0.0, val.doubleValue));
            }
            if (impl_->write_bytes_counter &&
                PdhGetFormattedCounterValue(impl_->write_bytes_counter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS &&
                val.CStatus == ERROR_SUCCESS) {
                storage.write_bytes_per_sec =
                    corewatch::core::SensorValue<double>::available(std::max(0.0, val.doubleValue));
            }
        }
    }
}

} // namespace corewatch::sensors
