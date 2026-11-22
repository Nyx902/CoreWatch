#include "corewatch/config/ConfigStore.hpp"
#include "corewatch/config/JsonSerializer.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

#include <fstream>
#include <iterator>
#include <string>

namespace corewatch::config {

namespace {
constexpr const wchar_t* kRunSubkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* kRunValueName = L"CoreWatch";
} // namespace

ConfigStore::ConfigStore(std::filesystem::path config_path)
    : path_(std::move(config_path)) {}

std::filesystem::path ConfigStore::default_config_path() {
    PWSTR local_app_data = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_app_data)) &&
        local_app_data != nullptr) {
        base = std::filesystem::path(local_app_data);
        CoTaskMemFree(local_app_data);
    } else {
        base = std::filesystem::current_path();
    }
    return base / L"CoreWatch" / L"config.json";
}

AppConfig ConfigStore::load() const {
    std::error_code ec;
    if (!std::filesystem::exists(path_, ec)) {
        return AppConfig{};
    }

    std::ifstream file(path_, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return AppConfig{};
    }

    const std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (auto parsed = deserialize_config_json(contents)) {
        return *parsed;
    }
    return AppConfig{};
}

bool ConfigStore::save(const AppConfig& config) const {
    std::error_code ec;
    if (path_.has_parent_path()) {
        std::filesystem::create_directories(path_.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream file(path_, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    const std::string json = serialize_config_json(config);
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    return file.good();
}

bool ConfigStore::set_windows_startup(bool enabled, const std::wstring& exe_path) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubkey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }

    LSTATUS status = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring quoted = L"\"" + exe_path + L"\"";
        status = RegSetValueExW(
            key,
            kRunValueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(quoted.c_str()),
            static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t)));
    } else {
        status = RegDeleteValueW(key, kRunValueName);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

bool ConfigStore::is_windows_startup_enabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubkey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    const LSTATUS status = RegQueryValueExW(key, kRunValueName, nullptr, &type, nullptr, nullptr);
    RegCloseKey(key);
    return status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ);
}

} // namespace corewatch::config
