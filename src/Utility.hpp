#pragma once

//
// お役立ち関数群
//

#include <codecvt>
#include <sstream>
#include <chrono>
#include <sys/stat.h>


namespace ngs {

// utf8文字列の文字数をカウント
size_t strlen(const std::string& s) noexcept {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.from_bytes(s).size();
}

// utf8文字列から部分文字列を取り出す
std::string substr(const std::string& s,
                   const size_t pos = 0, const size_t len = std::string::npos) noexcept {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

    auto u32string = conv.from_bytes(s);            // UTF8 -> UTF32
    auto sub = u32string.substr(pos, len);          // 部分取得

    return conv.to_bytes(sub);                      // UTF32 -> UTF8にして返す
}

// utf8文字列から1文字列を取り出す
char32_t getCharactor(const std::string& s, const size_t pos) noexcept {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

  auto u32string = conv.from_bytes(s);            // UTF8 -> UTF32
  return u32string[pos];
}

// キーワード置換
void replaceString(std::string& text, const std::string& src, const std::string& dst) noexcept {
	std::string::size_type pos = 0;
	while ((pos = text.find(src, pos)) != std::string::npos) {
		text.replace(pos, src.length(), dst);
		pos += dst.length();
	}
}

// 重複しないidを生成
u_int getUniqueNumber() noexcept {
  static u_int unique_number = 0;
  
  return ++unique_number;
}

// 配列の要素数を取得
template <typename T>
std::size_t elemsof(const T& t) noexcept {
  return std::distance(std::begin(t), std::end(t));
}

// 値を [min_value, max_value] にする
template <typename T>
T minmax(const T& value, const T& min_value, const T& max_value) noexcept {
  return std::max(std::min(value, max_value), min_value);
}

// 入力値を出力値に変換
template <typename T>
T remap(const T value, const ci::Vec2<T>& current_range, const ci::Vec2<T>& target_range) noexcept {
  T v = (value - current_range.x) / (current_range.y - current_range.x);
  return (target_range.y - target_range.x) * v + target_range.x;
}


// 時間→書式指定時間
std::string toFormatedString(const double progress_time, const bool has_hours = false) noexcept {
  // 表示の最大時間は99:59:59.9
  double max_time = 59 * 60 + 59 + 0.9;
  if (has_hours) max_time += 99.0 * (60 * 60);
  double output_time = std::min(progress_time, max_time);
  int hours   = int(output_time) / (60 * 60);
  int minutes = int(output_time) / 60;
  int seconds = int(output_time) % 60;
  int milli_seconds = int(output_time * 10.0) % 10;
      
  std::ostringstream str;
  if (has_hours) str << std::setw(2) << std::setfill('0') << hours << ":";
  str << std::setw(2) << std::setfill('0') << minutes
      << ":"
      << std::setw(2) << seconds
      << "."
      << milli_seconds;

  return str.str();
}

std::string toFormatedString(const int value, const int digits) noexcept {
  std::ostringstream str;
  str << std::setw(digits) << std::setfill('0') << value;

  return str.str();
}


// パスの有効判定
bool isValidPath(const std::string& path) noexcept {
	struct stat info;
	int result = stat(path.c_str(), &info);
	return (result == 0);
	// TODO: ディレクトリかどうかも判定
}

std::string createUniquePath() noexcept {
  static int unique_num = 0;
    
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);

  const tm* lt = std::localtime(&t);
    
  std::ostringstream path;
  path << "." << std::put_time(lt, "%F-%T.")
       << unique_num;

  unique_num += 1;

  return path.str();
}

// 拡張子を変更する
// ex) hoge/fuga/piyo.txt -> hoge/fuga/piyo.data
std::string replaceFilenameExt(const std::string& path, const std::string& ext) noexcept {
	std::string::size_type pos(path.rfind('.'));
  return (pos != std::string::npos) ? path.substr(0, path.rfind('.') + 1) + ext : path + "." + ext;
}

}
