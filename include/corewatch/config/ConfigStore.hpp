#pragma once

#include "corewatch/config/AppConfig.hpp"

#include <filesystem>
#include <string>

namespace corewatch::config {

class ConfigStore {
public:
    explicit ConfigStore(std::filesystem::path config_path = default_config_path());

    [[nodiscard]] static std::filesystem::path default_config_path();

    [[nodiscard]] AppConfig load() const;
    bool save(const AppConfig& config) const;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    /// Configures Windows HKCU Run registry entry for automatic startup.
    static bool set_windows_startup(bool enabled, const std::wstring& exe_path);
    [[nodiscard]] static bool is_windows_startup_enabled();

private:
    std::filesystem::path path_;
};

} // namespace corewatch::config
