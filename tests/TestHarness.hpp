#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace corewatch::test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        registry().push_back(TestCase{std::move(name), std::move(fn)});
    }
};

inline void expect_true(bool cond, const char* expr, const char* file, int line) {
    if (!cond) {
        std::ostringstream oss;
        oss << file << ":" << line << ": Assertion failed: " << expr;
        throw std::runtime_error(oss.str());
    }
}

template <typename A, typename B>
inline void expect_eq(const A& a, const B& b, const char* expr_a, const char* expr_b, const char* file, int line) {
    if (!(a == b)) {
        std::ostringstream oss;
        oss << file << ":" << line << ": Expected (" << expr_a << ") == (" << expr_b << ")";
        throw std::runtime_error(oss.str());
    }
}

} // namespace corewatch::test

#define CW_TEST(name)                                                              \
    static void cw_test_##name();                                                  \
    static ::corewatch::test::Registrar cw_reg_##name(#name, &cw_test_##name);     \
    static void cw_test_##name()

#define CW_EXPECT_TRUE(expr) ::corewatch::test::expect_true(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CW_EXPECT_EQ(a, b) ::corewatch::test::expect_eq((a), (b), #a, #b, __FILE__, __LINE__)
