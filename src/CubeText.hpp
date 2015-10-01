#pragma once

//
// 立方体文字列
//

#include <string>
#include <functional>
#include "Utility.hpp"
#include "cinder/Easing.h"
#include "cinder/AxisAlignedBox.h"
#include <boost/noncopyable.hpp>


namespace ngs {

class CubeText : private boost::noncopyable {
  std::vector<std::string> text_;

  float size_;
  float spacing_;
  
  size_t chara_num_;

  ci::Vec3f min_pos_;
  ci::Vec3f max_pos_;
  

public:
  CubeText() = default;
  
  CubeText(const std::string& text,
           const float size, const float spacing,
           const size_t chara_num) :
    size_(size),
    spacing_(spacing),
    chara_num_(chara_num),
    min_pos_(ci::Vec3f::zero()),
    max_pos_(ci::Vec3f::zero())
  {
    makeText(text);
  }


  void setText(const std::string& text) noexcept {
    text_.clear();
    makeText(text);
  }
  
  const std::vector<std::string>& text() const noexcept { return text_; }

  float size() const noexcept { return size_; }
  float spacing() const noexcept { return spacing_; }

  size_t getNumCharactors() const noexcept { return text_.size(); }
  
  ci::Vec3f textSize() const noexcept {
    return max_pos_ - min_pos_;
  }

  float textWidth() const noexcept {
    return max_pos_.x - min_pos_.x; 
  }


  const ci::Vec3f& minPos() const noexcept { return min_pos_; }
  const ci::Vec3f& maxPos() const noexcept { return max_pos_; }
  
  
private:
  void makeText(const std::string& text) noexcept {
    // 文字列を分解
    size_t text_length = strlen(text);
    for (size_t it = 0; it < text_length; it += chara_num_) {
      text_.emplace_back(substr(text, it, chara_num_));
    }

    if (!text_.empty()) {
      min_pos_ = ci::Vec3f(0.0f, 0.0f, -size_ / 2);
      max_pos_ = ci::Vec3f(size_ * text_.size() + spacing_ * (text_.size() - 1), size_, -size_ / 2);
    }
  }
  
};

}
