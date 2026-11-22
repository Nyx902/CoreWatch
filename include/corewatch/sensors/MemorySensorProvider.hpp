#pragma once

#include "corewatch/sensors/ISensorProvider.hpp"

namespace corewatch::sensors {

class MemorySensorProvider final : public ISensorProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override { return "Win32 Memory"; }
    [[nodiscard]] bool initialize() override { return true; }
    [[nodiscard]] bool is_available() const noexcept override { return true; }
    void poll(corewatch::core::SystemSnapshot& snapshot) override;
};

} // namespace corewatch::sensors
