#pragma once

// 
// 評価ダイアログ
//


namespace ngs { namespace Rating {

#if defined(CINDER_COCOA_TOUCH)

// ダイアログ表示
void popup(std::function<void()> start_callback,
           std::function<void()> finish_callback) noexcept;
// 元に戻す
void reset() noexcept;

#else

template <typename T1, typename T2>
void popup(T1, T2) noexcept {}

template <typename T = void>
void reset() noexcept {}

#endif

} }
