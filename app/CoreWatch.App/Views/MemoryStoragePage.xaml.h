#pragma once

#include "MemoryStoragePage.g.h"

namespace winrt::CoreWatch::App::implementation {

struct MemoryStoragePage : MemoryStoragePageT<MemoryStoragePage> {
    MemoryStoragePage();
    void Refresh();
};

} // namespace winrt::CoreWatch::App::implementation

namespace winrt::CoreWatch::App::factory_implementation {
struct MemoryStoragePage : MemoryStoragePageT<MemoryStoragePage, implementation::MemoryStoragePage> {};
}
