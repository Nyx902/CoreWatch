#include "corewatch/config/JsonSerializer.hpp"

#include <cctype>
#include <charconv>
#include <format>
#include <string>
#include <string_view>

namespace corewatch::config {

namespace {

std::string_view theme_to_string(ThemePreference theme) noexcept {
    switch (theme) {
        case ThemePreference::Light: return "light";
        case ThemePreference::Dark: return "dark";
        case ThemePreference::System:
        default: return "system";
    }
}

ThemePreference string_to_theme(std::string_view str) noexcept {
    if (str == "light") return ThemePreference::Light;
    if (str == "dark") return ThemePreference::Dark;
    return ThemePreference::System;
}

class JsonCursor {
public:
    explicit JsonCursor(std::string_view input) : src_(input) {}

    void skip_ws() noexcept {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
    }

    bool consume(char expected) noexcept {
        skip_ws();
        if (pos_ < src_.size() && src_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    [[nodiscard]] char peek() noexcept {
        skip_ws();
        return pos_ < src_.size() ? src_[pos_] : '\0';
    }

    std::optional<std::string> parse_string() {
        skip_ws();
        if (pos_ >= src_.size() || src_[pos_] != '"') {
            return std::nullopt;
        }
        ++pos_;
        std::string out;
        while (pos_ < src_.size()) {
            const char c = src_[pos_++];
            if (c == '"') {
                return out;
            }
            if (c == '\\') {
                if (pos_ >= src_.size()) return std::nullopt;
                const char esc = src_[pos_++];
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: out.push_back(esc); break;
                }
            } else {
                out.push_back(c);
            }
        }
        return std::nullopt;
    }

    std::optional<bool> parse_bool() {
        skip_ws();
        if (src_.substr(pos_).starts_with("true")) {
            pos_ += 4;
            return true;
        }
        if (src_.substr(pos_).starts_with("false")) {
            pos_ += 5;
            return false;
        }
        return std::nullopt;
    }

    std::optional<double> parse_number() {
        skip_ws();
        const std::size_t start = pos_;
        if (pos_ < src_.size() && (src_[pos_] == '-' || src_[pos_] == '+')) {
            ++pos_;
        }
        bool has_digits = false;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            has_digits = true;
            ++pos_;
        }
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                has_digits = true;
                ++pos_;
            }
        }
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '-' || src_[pos_] == '+')) {
                ++pos_;
            }
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }
        if (!has_digits) {
            return std::nullopt;
        }
        double value = 0.0;
        const auto [ptr, ec] = std::from_chars(src_.data() + start, src_.data() + pos_, value);
        if (ec != std::errc{} || ptr != src_.data() + pos_) {
            return std::nullopt;
        }
        return value;
    }

    bool skip_value() {
        const char c = peek();
        if (c == '"') {
            return parse_string().has_value();
        }
        if (c == 't' || c == 'f') {
            return parse_bool().has_value();
        }
        if (c == 'n') {
            if (src_.substr(pos_).starts_with("null")) {
                pos_ += 4;
                return true;
            }
            return false;
        }
        if (c == '{') {
            ++pos_;
            if (peek() == '}') {
                ++pos_;
                return true;
            }
            while (true) {
                if (!parse_string()) return false;
                if (!consume(':')) return false;
                if (!skip_value()) return false;
                if (consume(',')) continue;
                return consume('}');
            }
        }
        if (c == '[') {
            ++pos_;
            if (peek() == ']') {
                ++pos_;
                return true;
            }
            while (true) {
                if (!skip_value()) return false;
                if (consume(',')) continue;
                return consume(']');
            }
        }
        return parse_number().has_value();
    }

private:
    std::string_view src_;
    std::size_t pos_{0};
};

} // namespace

std::string serialize_config_json(const AppConfig& config) {
    AppConfig sanitized = config;
    sanitized.sanitize();
    return std::format(
        "{{\n"
        "  \"polling_interval_ms\": {},\n"
        "  \"theme\": \"{}\",\n"
        "  \"launch_at_startup\": {},\n"
        "  \"minimize_to_tray\": {},\n"
        "  \"alerts_enabled\": {},\n"
        "  \"gpu_temp_threshold_c\": {:.1f},\n"
        "  \"cpu_temp_threshold_c\": {:.1f},\n"
        "  \"alert_cooldown_seconds\": {}\n"
        "}}\n",
        sanitized.polling_interval_ms,
        theme_to_string(sanitized.theme),
        sanitized.launch_at_startup ? "true" : "false",
        sanitized.minimize_to_tray ? "true" : "false",
        sanitized.alerts_enabled ? "true" : "false",
        sanitized.gpu_temp_threshold_c,
        sanitized.cpu_temp_threshold_c,
        sanitized.alert_cooldown_seconds);
}

std::optional<AppConfig> deserialize_config_json(std::string_view json) {
    JsonCursor cur(json);
    if (!cur.consume('{')) {
        return std::nullopt;
    }

    AppConfig cfg{};
    if (cur.peek() == '}') {
        cur.consume('}');
        return cfg;
    }

    while (true) {
        auto key = cur.parse_string();
        if (!key || !cur.consume(':')) {
            return std::nullopt;
        }

        if (*key == "polling_interval_ms") {
            auto v = cur.parse_number();
            if (!v) return std::nullopt;
            cfg.polling_interval_ms = static_cast<std::uint32_t>(*v);
        } else if (*key == "theme") {
            auto v = cur.parse_string();
            if (!v) return std::nullopt;
            cfg.theme = string_to_theme(*v);
        } else if (*key == "launch_at_startup") {
            auto v = cur.parse_bool();
            if (!v) return std::nullopt;
            cfg.launch_at_startup = *v;
        } else if (*key == "minimize_to_tray") {
            auto v = cur.parse_bool();
            if (!v) return std::nullopt;
            cfg.minimize_to_tray = *v;
        } else if (*key == "alerts_enabled") {
            auto v = cur.parse_bool();
            if (!v) return std::nullopt;
            cfg.alerts_enabled = *v;
        } else if (*key == "gpu_temp_threshold_c") {
            auto v = cur.parse_number();
            if (!v) return std::nullopt;
            cfg.gpu_temp_threshold_c = *v;
        } else if (*key == "cpu_temp_threshold_c") {
            auto v = cur.parse_number();
            if (!v) return std::nullopt;
            cfg.cpu_temp_threshold_c = *v;
        } else if (*key == "alert_cooldown_seconds") {
            auto v = cur.parse_number();
            if (!v) return std::nullopt;
            cfg.alert_cooldown_seconds = static_cast<std::uint32_t>(*v);
        } else {
            if (!cur.skip_value()) return std::nullopt;
        }

        if (cur.consume(',')) {
            continue;
        }
        if (cur.consume('}')) {
            break;
        }
        return std::nullopt;
    }

    cfg.sanitize();
    return cfg;
}

} // namespace corewatch::config
