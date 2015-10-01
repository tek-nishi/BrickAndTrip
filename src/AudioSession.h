#pragma once

//
// AudioSession設定
//


#if defined(CINDER_COCOA_TOUCH)

void beginAudioSession() noexcept;
void endAudioSession() noexcept;

#else

// iOS以外は実装無し
#define beginAudioSession()
#define endAudioSession()

#endif
