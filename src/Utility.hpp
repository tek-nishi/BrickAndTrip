﻿#pragma once

//
// お役立ち関数群
//

#include <codecvt>
#include <sstream>
#include <sys/stat.h>


namespace ngs {

// utf8文字列の文字数をカウント
size_t strlen(const std::string& s) {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.from_bytes(s).size();
}

// utf8文字列から部分文字列を取り出す
std::string substr(const std::string& s,
                   const size_t pos = 0, const size_t len = std::string::npos) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

    auto u32string = conv.from_bytes(s);            // UTF8 -> UTF32
    auto sub = u32string.substr(pos, len);          // 部分取得

    return conv.to_bytes(sub);                      // UTF32 -> UTF8にして返す
}

// 重複しないidを生成
u_int getUniqueNumber() {
  static u_int unique_number = 0;
  
  return ++unique_number;
}

// 配列の要素数を取得
template <typename T>
std::size_t elemsof(const T& t) {
  return std::distance(std::begin(t), std::end(t));
}


// 時間→書式指定時間
std::string toFormatedString(const double progress_time) {
  // 表示の最大時間は59:59.9
  double output_time = std::min(progress_time, 59 * 60 + 59 + 0.9);
  int minutes = int(output_time) / 60;
  int seconds = int(output_time) % 60;
  int milli_seconds = int(output_time * 10.0) % 10;
      
  std::ostringstream str;
  str << std::setw(2) << std::setfill('0') << minutes
      << ":"
      << std::setw(2) << seconds
      << "."
      << milli_seconds;

  return str.str();
}

// パスの有効判定
bool isValidPath(const std::string& path) {
	struct stat info;
	int result = stat(path.c_str(), &info);
	return (result == 0);
	// TODO: ディレクトリかどうかも判定
}

}
