#include "corewatch/sensors/MemorySensorProvider.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>

namespace corewatch::sensors {

void MemorySensorProvider::poll(corewatch::core::SystemSnapshot& snapshot) {
    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex)) {
        snapshot.memory = corewatch::core::MemorySnapshot{};
        return;
    }

    const std::uint64_t total = statex.ullTotalPhys;
    const std::uint64_t avail = statex.ullAvailPhys;
    const std::uint64_t used = (total >= avail) ? (total - avail) : 0ULL;

    snapshot.memory.total_bytes = corewatch::core::SensorValue<std::uint64_t>::available(total);
    snapshot.memory.available_bytes = corewatch::core::SensorValue<std::uint64_t>::available(avail);
    snapshot.memory.used_bytes = corewatch::core::SensorValue<std::uint64_t>::available(used);

    if (total > 0) {
        const double pct = std::clamp(
            (static_cast<double>(used) / static_cast<double>(total)) * 100.0, 0.0, 100.0);
        snapshot.memory.usage_percent = corewatch::core::SensorValue<double>::available(pct);
    }

    const std::uint64_t commit_limit = statex.ullTotalPageFile;
    const std::uint64_t commit_avail = statex.ullAvailPageFile;
    const std::uint64_t commit_used = (commit_limit >= commit_avail) ? (commit_limit - commit_avail) : 0ULL;
    snapshot.memory.commit_limit_bytes = corewatch::core::SensorValue<std::uint64_t>::available(commit_limit);
    snapshot.memory.commit_total_bytes = corewatch::core::SensorValue<std::uint64_t>::available(commit_used);
}

} // namespace corewatch::sensors
