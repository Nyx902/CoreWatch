#pragma once

#include "corewatch/core/Types.hpp"

#include <string_view>

namespace corewatch::sensors {

/// Common interface for all read-only hardware sensor providers.
class ISensorProvider {
public:
    virtual ~ISensorProvider() = default;

    [[nodiscard]] virtual std::string_view provider_name() const noexcept = 0;
    [[nodiscard]] virtual bool initialize() = 0;
    [[nodiscard]] virtual bool is_available() const noexcept = 0;
    virtual void poll(corewatch::core::SystemSnapshot& snapshot) = 0;
};

} // namespace corewatch::sensors
