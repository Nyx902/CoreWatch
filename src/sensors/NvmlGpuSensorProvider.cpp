#include "corewatch/sensors/NvmlGpuSensorProvider.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <type_traits>

namespace corewatch::sensors {

namespace {

using nvmlReturn_t = int;
using nvmlDevice_t = struct nvmlDevice_st*;

constexpr nvmlReturn_t kNvmlSuccess = 0;
constexpr nvmlReturn_t kNvmlErrorNotSupported = 3;
constexpr unsigned int kNvmlTemperatureGpu = 0;
constexpr unsigned int kNvmlClockGraphics = 0;
constexpr unsigned int kNvmlClockMem = 2;

struct nvmlUtilization_t {
    unsigned int gpu;
    unsigned int memory;
};

struct nvmlMemory_t {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
};

using FnNvmlInitV2 = nvmlReturn_t (*)();
using FnNvmlShutdown = nvmlReturn_t (*)();
using FnNvmlSystemGetDriverVersion = nvmlReturn_t (*)(char*, unsigned int);
using FnNvmlDeviceGetCountV2 = nvmlReturn_t (*)(unsigned int*);
using FnNvmlDeviceGetHandleByIndexV2 = nvmlReturn_t (*)(unsigned int, nvmlDevice_t*);
using FnNvmlDeviceGetName = nvmlReturn_t (*)(nvmlDevice_t, char*, unsigned int);
using FnNvmlDeviceGetUtilizationRates = nvmlReturn_t (*)(nvmlDevice_t, nvmlUtilization_t*);
using FnNvmlDeviceGetTemperature = nvmlReturn_t (*)(nvmlDevice_t, unsigned int, unsigned int*);
using FnNvmlDeviceGetMemoryInfo = nvmlReturn_t (*)(nvmlDevice_t, nvmlMemory_t*);
using FnNvmlDeviceGetPowerUsage = nvmlReturn_t (*)(nvmlDevice_t, unsigned int*);
using FnNvmlDeviceGetEnforcedPowerLimit = nvmlReturn_t (*)(nvmlDevice_t, unsigned int*);
using FnNvmlDeviceGetFanSpeed = nvmlReturn_t (*)(nvmlDevice_t, unsigned int*);
using FnNvmlDeviceGetClockInfo = nvmlReturn_t (*)(nvmlDevice_t, unsigned int, unsigned int*);

struct ModuleDeleter {
    void operator()(HMODULE mod) const noexcept {
        if (mod != nullptr) {
            FreeLibrary(mod);
        }
    }
};

using UniqueModule = std::unique_ptr<std::remove_pointer_t<HMODULE>, ModuleDeleter>;

template <typename Fn>
Fn load_proc(HMODULE mod, const char* name) noexcept {
    return reinterpret_cast<Fn>(GetProcAddress(mod, name));
}

template <typename T>
corewatch::core::SensorValue<T> map_nvml_error(nvmlReturn_t rc) noexcept {
    if (rc == kNvmlErrorNotSupported) {
        return corewatch::core::SensorValue<T>::unsupported();
    }
    return corewatch::core::SensorValue<T>::unavailable();
}

} // namespace

struct NvmlGpuSensorProvider::Impl {
    UniqueModule module{nullptr};
    bool nvml_initialized{false};

    FnNvmlInitV2 init_v2{nullptr};
    FnNvmlShutdown shutdown{nullptr};
    FnNvmlSystemGetDriverVersion get_driver_version{nullptr};
    FnNvmlDeviceGetCountV2 get_device_count{nullptr};
    FnNvmlDeviceGetHandleByIndexV2 get_handle_by_index{nullptr};
    FnNvmlDeviceGetName get_name{nullptr};
    FnNvmlDeviceGetUtilizationRates get_utilization{nullptr};
    FnNvmlDeviceGetTemperature get_temperature{nullptr};
    FnNvmlDeviceGetMemoryInfo get_memory_info{nullptr};
    FnNvmlDeviceGetPowerUsage get_power_usage{nullptr};
    FnNvmlDeviceGetEnforcedPowerLimit get_power_limit{nullptr};
    FnNvmlDeviceGetFanSpeed get_fan_speed{nullptr};
    FnNvmlDeviceGetClockInfo get_clock_info{nullptr};

    ~Impl() {
        if (nvml_initialized && shutdown != nullptr) {
            shutdown();
        }
    }
};

NvmlGpuSensorProvider::NvmlGpuSensorProvider()
    : impl_(std::make_unique<Impl>()) {}

NvmlGpuSensorProvider::~NvmlGpuSensorProvider() = default;

bool NvmlGpuSensorProvider::initialize() {
    HMODULE mod = LoadLibraryW(L"nvml.dll");
    if (mod == nullptr) {
        // Fallback path used by some NVIDIA driver installations
        wchar_t sys_dir[MAX_PATH]{};
        if (GetSystemDirectoryW(sys_dir, MAX_PATH) > 0) {
            std::wstring full_path = std::wstring(sys_dir) + L"\\nvml.dll";
            mod = LoadLibraryW(full_path.c_str());
        }
    }
    if (mod == nullptr) {
        available_ = false;
        return false;
    }

    impl_->module.reset(mod);
    impl_->init_v2 = load_proc<FnNvmlInitV2>(mod, "nvmlInit_v2");
    impl_->shutdown = load_proc<FnNvmlShutdown>(mod, "nvmlShutdown");
    impl_->get_driver_version = load_proc<FnNvmlSystemGetDriverVersion>(mod, "nvmlSystemGetDriverVersion");
    impl_->get_device_count = load_proc<FnNvmlDeviceGetCountV2>(mod, "nvmlDeviceGetCount_v2");
    impl_->get_handle_by_index = load_proc<FnNvmlDeviceGetHandleByIndexV2>(mod, "nvmlDeviceGetHandleByIndex_v2");
    impl_->get_name = load_proc<FnNvmlDeviceGetName>(mod, "nvmlDeviceGetName");
    impl_->get_utilization = load_proc<FnNvmlDeviceGetUtilizationRates>(mod, "nvmlDeviceGetUtilizationRates");
    impl_->get_temperature = load_proc<FnNvmlDeviceGetTemperature>(mod, "nvmlDeviceGetTemperature");
    impl_->get_memory_info = load_proc<FnNvmlDeviceGetMemoryInfo>(mod, "nvmlDeviceGetMemoryInfo");
    impl_->get_power_usage = load_proc<FnNvmlDeviceGetPowerUsage>(mod, "nvmlDeviceGetPowerUsage");
    impl_->get_power_limit = load_proc<FnNvmlDeviceGetEnforcedPowerLimit>(mod, "nvmlDeviceGetEnforcedPowerLimit");
    impl_->get_fan_speed = load_proc<FnNvmlDeviceGetFanSpeed>(mod, "nvmlDeviceGetFanSpeed");
    impl_->get_clock_info = load_proc<FnNvmlDeviceGetClockInfo>(mod, "nvmlDeviceGetClockInfo");

    if (!impl_->init_v2 || !impl_->get_device_count || !impl_->get_handle_by_index) {
        available_ = false;
        return false;
    }

    if (impl_->init_v2() != kNvmlSuccess) {
        available_ = false;
        return false;
    }
    impl_->nvml_initialized = true;

    unsigned int count = 0;
    if (impl_->get_device_count(&count) != kNvmlSuccess || count == 0) {
        available_ = false;
        return false;
    }

    available_ = true;
    return true;
}

void NvmlGpuSensorProvider::poll(corewatch::core::SystemSnapshot& snapshot) {
    snapshot.gpus.clear();
    if (!available_ || !impl_->nvml_initialized) {
        return;
    }

    unsigned int count = 0;
    if (impl_->get_device_count(&count) != kNvmlSuccess || count == 0) {
        return;
    }

    std::string driver_ver;
    if (impl_->get_driver_version) {
        char buf[80]{};
        if (impl_->get_driver_version(buf, sizeof(buf)) == kNvmlSuccess) {
            driver_ver = buf;
        }
    }

    for (unsigned int i = 0; i < count; ++i) {
        nvmlDevice_t dev = nullptr;
        if (impl_->get_handle_by_index(i, &dev) != kNvmlSuccess || dev == nullptr) {
            continue;
        }

        corewatch::core::GpuSnapshot gpu{};
        gpu.index = i;
        gpu.vendor = "NVIDIA";
        gpu.driver_version = driver_ver;
        gpu.nvml_active = true;

        if (impl_->get_name) {
            char name_buf[96]{};
            if (impl_->get_name(dev, name_buf, sizeof(name_buf)) == kNvmlSuccess) {
                gpu.name = name_buf;
            }
        }

        if (impl_->get_utilization) {
            nvmlUtilization_t util{};
            const nvmlReturn_t rc = impl_->get_utilization(dev, &util);
            gpu.utilization_percent = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(util.gpu))
                : map_nvml_error<double>(rc);
        }

        if (impl_->get_temperature) {
            unsigned int temp = 0;
            const nvmlReturn_t rc = impl_->get_temperature(dev, kNvmlTemperatureGpu, &temp);
            gpu.temperature_celsius = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(temp))
                : map_nvml_error<double>(rc);
        }

        if (impl_->get_memory_info) {
            nvmlMemory_t mem{};
            const nvmlReturn_t rc = impl_->get_memory_info(dev, &mem);
            if (rc == kNvmlSuccess) {
                gpu.vram_used_bytes = corewatch::core::SensorValue<std::uint64_t>::available(mem.used);
                gpu.vram_total_bytes = corewatch::core::SensorValue<std::uint64_t>::available(mem.total);
            } else {
                gpu.vram_used_bytes = map_nvml_error<std::uint64_t>(rc);
                gpu.vram_total_bytes = map_nvml_error<std::uint64_t>(rc);
            }
        }

        if (impl_->get_power_usage) {
            unsigned int mw = 0;
            const nvmlReturn_t rc = impl_->get_power_usage(dev, &mw);
            gpu.power_watts = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(mw) / 1000.0)
                : map_nvml_error<double>(rc);
        }

        if (impl_->get_power_limit) {
            unsigned int mw = 0;
            const nvmlReturn_t rc = impl_->get_power_limit(dev, &mw);
            gpu.power_limit_watts = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(mw) / 1000.0)
                : map_nvml_error<double>(rc);
        }

        if (impl_->get_fan_speed) {
            unsigned int fan = 0;
            const nvmlReturn_t rc = impl_->get_fan_speed(dev, &fan);
            gpu.fan_speed_percent = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(fan))
                : map_nvml_error<double>(rc);
        }

        if (impl_->get_clock_info) {
            unsigned int clk = 0;
            nvmlReturn_t rc = impl_->get_clock_info(dev, kNvmlClockGraphics, &clk);
            gpu.graphics_clock_mhz = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(clk))
                : map_nvml_error<double>(rc);

            rc = impl_->get_clock_info(dev, kNvmlClockMem, &clk);
            gpu.memory_clock_mhz = (rc == kNvmlSuccess)
                ? corewatch::core::SensorValue<double>::available(static_cast<double>(clk))
                : map_nvml_error<double>(rc);
        }

        snapshot.gpus.push_back(std::move(gpu));
    }
}

} // namespace corewatch::sensors
