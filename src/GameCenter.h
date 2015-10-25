#pragma once

//
// GameCenter操作
//

#include "Defines.hpp"
#include <functional>


namespace ngs { namespace GameCenter {

#if defined(CINDER_COCOA_TOUCH)

void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback) noexcept;

bool isAuthenticated() noexcept;

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback) noexcept;

void submitStageScore(const int stage,
                      const int score, const double clear_time) noexcept;

void submitScore(const int score) noexcept;

#else

template <typename T1, typename T2>
void authenticateLocalPlayer(T1 start_callback,
                             T2 finish_callback) noexcept {}

template <typename T = void>
bool isAuthenticated() noexcept { return false; }

template <typename T1, typename T2>
void showBoard(T1 start_callback,
               T2 finish_callback) noexcept {}

template <typename T1, typename T2, typename T3>
void submitStageScore(const T1 stage,
                      const T2 score, const T3 clear_time) noexcept {};

template <typename T>
void submitScore(const T score) noexcept {};

#endif

} }
