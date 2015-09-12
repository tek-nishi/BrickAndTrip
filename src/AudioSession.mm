//
// AudioSession設定
//

/*

setCategory一覧

NSString *const AVAudioSessionCategoryAmbient;
  別アプリでオーディオ再生中にアプリを起動しても停止しない。

NSString *const AVAudioSessionCategorySoloAmbient;
  デフォルト設定　別アプリでオーディオ再生中にアプリを起動するとオーディオが停止する。

NSString *const AVAudioSessionCategoryPlayback;
  音楽再生用のアプリで利用

NSString *const AVAudioSessionCategoryRecord;
  入力のみで録音用に利用

NSString *const AVAudioSessionCategoryPlayAndRecord;
  Voip,チャット用にマイク入力と音声出力を行う際に利用

NSString *const AVAudioSessionCategoryAudioProcessing;
  再生や録音ではなくオフラインオーディオ処理を行う際に利用。

NSString *const AVAudioSessionCategoryMultiRoute;
  USBオーディオインターフェースやHDMIなどの外部出力を接続したときに
  ヘッドホンへ別系統の音を出力できる機能です。
  
*/

#import <AVFoundation/AVFoundation.h>


void beginAudioSession() {
  // AVFoundationのインスタンス
  AVAudioSession* audioSession = [AVAudioSession sharedInstance];

  // カテゴリの設定
  [audioSession setCategory:AVAudioSessionCategoryAmbient error:nil];

  // AudioSession利用開始
  [audioSession setActive:YES error:nil];
}

void endAudioSession() {
  AVAudioSession* audioSession = [AVAudioSession sharedInstance];
  [audioSession setActive:NO error:nil];
}
