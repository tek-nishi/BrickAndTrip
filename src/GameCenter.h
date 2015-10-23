#pragma once

//
// GameCenter操作
//

namespace ngs { namespace GameCenter {

#if defined(CINDER_COCOA_TOUCH)

void authenticateLocalPlayer() noexcept;
void showBord() noexcept;

#else

template <typename T = void>
void authenticateLocalPlayer() noexcept {}

template <typename T = void>
void showBord() noexcept {}

#endif

} }
