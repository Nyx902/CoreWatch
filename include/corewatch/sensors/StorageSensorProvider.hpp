#pragma once

#include "corewatch/sensors/ISensorProvider.hpp"

#include <memory>

namespace corewatch::sensors {

class StorageSensorProvider final : public ISensorProvider {
public:
    StorageSensorProvider();
    ~StorageSensorProvider() override;

    StorageSensorProvider(const StorageSensorProvider&) = delete;
    StorageSensorProvider& operator=(const StorageSensorProvider&) = delete;

    [[nodiscard]] std::string_view provider_name() const noexcept override { return "Win32 Storage"; }
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool is_available() const noexcept override { return initialized_; }
    void poll(corewatch::core::SystemSnapshot& snapshot) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_{false};
};

} // namespace corewatch::sensors
