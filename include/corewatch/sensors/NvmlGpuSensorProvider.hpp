#pragma once

#include "corewatch/sensors/ISensorProvider.hpp"

#include <memory>

namespace corewatch::sensors {

class NvmlGpuSensorProvider final : public ISensorProvider {
public:
    NvmlGpuSensorProvider();
    ~NvmlGpuSensorProvider() override;

    NvmlGpuSensorProvider(const NvmlGpuSensorProvider&) = delete;
    NvmlGpuSensorProvider& operator=(const NvmlGpuSensorProvider&) = delete;

    [[nodiscard]] std::string_view provider_name() const noexcept override { return "NVIDIA NVML"; }
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool is_available() const noexcept override { return available_; }
    void poll(corewatch::core::SystemSnapshot& snapshot) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool available_{false};
};

} // namespace corewatch::sensors
