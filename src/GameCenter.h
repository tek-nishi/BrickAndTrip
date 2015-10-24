#pragma once

//
// GameCenter操作
//

#include <functional>


namespace ngs { namespace GameCenter {

#if defined(CINDER_COCOA_TOUCH)

void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback) noexcept;

bool isAuthenticated() noexcept;

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback) noexcept;

#else

template <typename T1, typename T2>
void authenticateLocalPlayer(T1 start_callback,
                             T2 finish_callback) noexcept {}

template <typename T = void>
bool isAuthenticated() noexcept { return false; }

template <typename T1, typename T2>
void showBoard(T1 start_callback,
               T2 finish_callback) noexcept {}

#endif

} }
