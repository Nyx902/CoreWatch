#pragma once

#include "corewatch/sensors/ISensorProvider.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace corewatch::sensors {

class CpuSensorProvider final : public ISensorProvider {
public:
    CpuSensorProvider();
    ~CpuSensorProvider() override;

    CpuSensorProvider(const CpuSensorProvider&) = delete;
    CpuSensorProvider& operator=(const CpuSensorProvider&) = delete;

    [[nodiscard]] std::string_view provider_name() const noexcept override { return "Win32 CPU"; }
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool is_available() const noexcept override { return initialized_; }
    void poll(corewatch::core::SystemSnapshot& snapshot) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_{false};
};

} // namespace corewatch::sensors
