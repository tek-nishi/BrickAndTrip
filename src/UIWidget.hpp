#pragma once

//
// UI部品
//

#include "cinder/Timeline.h"
#include "CubeText.hpp"
#include "Autolayout.hpp"
#include "JsonUtil.hpp"
#include "CubeTextDrawer.hpp"
#include "EasingUtil.hpp"
#include "TweenUtil.hpp"
#include "FontHolder.hpp"
#include <set>
#include <boost/noncopyable.hpp>


namespace ngs {

class UIWidget : private boost::noncopyable {
  // FIXME:大元のデータが破棄されるので、コピーを保持している
  const ci::JsonTree params_;

  std::string name_;

  CubeText text_;
  std::string font_name_;

  ci::Vec3f padding_;

  std::string model_;
  
  ci::Anim<ci::Vec3f> pos_;
  ci::Anim<ci::Vec3f> scale_;
  std::vector<ci::Anim<float> > rotate_;

  ci::Anim<ci::Color> base_color_;
  ci::Anim<ci::Color> text_color_;

  ci::Anim<ci::Vec3f> base_color_hsv_;
  ci::Anim<ci::Vec3f> text_color_hsv_;
  
  Autolayout::WidgetRef layout_;
  ci::TimelineRef timeline_;
  
  // dispは表示のON/OFF
  // activeはタッチイベントのON/OFF
  bool disp_;
  bool active_;

  bool hsv_;
  
  bool touch_event_;
  std::string event_message_;
  bool touch_sound_;
  std::string sound_message_;

  
public:
  UIWidget(const ci::JsonTree& params,
           ci::TimelineRef timeline,
           Autolayout& autolayout,
           const float padding) :
    params_(params),
    name_(params["name"].getValue<std::string>()),
    text_(params["text"].getValue<std::string>(),
          params["size"].getValue<float>(),
          params["spacing"].getValue<float>(),
          params["chara_split"].getValue<size_t>()),
    font_name_(Json::getValue(params, "font", std::string("default"))),
    padding_(padding, padding, 0.0f),
    model_(Json::getValue(params, "model", std::string("text"))),
    pos_(ci::Vec3f::zero()),
    scale_(ci::Vec3f::one()),
    rotate_(text_.text().size()),
    timeline_(ci::Timeline::create()),
    disp_(true),
    active_(false),
    touch_event_(false),
    touch_sound_(false)
  {
    std::fill(std::begin(rotate_), std::end(rotate_), 0.0f);
    
    layout_ = autolayout.makeWidget(Json::getVec3<float>(params["pos"]),
                                    text_.textSize(),
                                    Autolayout::getType(params["origin"].getValue<std::string>()),
                                    Autolayout::getType(params["layout"].getValue<std::string>()));

    if (params.hasChild("event_message")) {
      event_message_ = params["event_message"].getValue<std::string>();
      touch_event_   = true;
    }
    if (params.hasChild("sound_message")) {
      sound_message_ = params["sound_message"].getValue<std::string>();
      touch_sound_   = true;
    }
    
    if (params.hasChild("padding")) {
      // 初期値を上書き
      float value = params["padding"].getValue<float>();
      padding_ = ci::Vec3f(value, value, 0.0f);
    }

    disp_ = Json::getValue(params, "disp", disp_);
    hsv_  = Json::getValue(params, "hsv", false);

    if (hsv_) {
      base_color_hsv_ = Json::getHsvColor(params["base_color"]);
      text_color_hsv_ = Json::getHsvColor(params["text_color"]);
    }
    else {
      base_color_ = Json::getColor<float>(params["base_color"]);
      text_color_ = Json::getColor<float>(params["text_color"]);
    }
    
    // touchEventがなければactiveは常にfalse
    active_ = touch_event_ ? Json::getValue(params, "active", active_) : false;

    timeline_->setStartTime(timeline->getCurrentTime());
    timeline->apply(timeline_);
  }

  ~UIWidget() {
    // 再生途中のものもあるので、自分で取り除く
    timeline_->removeSelf();
  }

  
  const std::string& getName() const noexcept { return name_; }

  ci::Color getBaseColor() const noexcept {
    if (hsv_) {
      const auto& v = base_color_hsv_();
      auto color = ci::Vec3f(std::fmod(v.x, 1.0f), v.y, v.z);
      return ci::hsvToRGB(color);
    }
    else {
      return base_color_;
    }
  }
  
  ci::Color getTextColor() const noexcept {
    if (hsv_) {
      const auto& v = text_color_hsv_();
      auto color = ci::Vec3f(std::fmod(v.x, 1.0f), v.y, v.z);
      return ci::hsvToRGB(color);
    }
    else {
      return text_color_;
    }
  }

  void setBaseColor(const ci::Color& color) noexcept {
    base_color_ = color;
  }
  
  void setBaseColor(const ci::Vec3f& color) noexcept {
    base_color_hsv_ = color;
  }

  void setTextColor(const ci::Color& color) noexcept {
    text_color_ = color;
  }

  void setTextColor(const ci::Vec3f& color) noexcept {
    text_color_hsv_ = color;
  }

  
  CubeText& getCubeText() noexcept { return text_; }

  void setText(const std::string& text, const bool resize = true) noexcept {
    size_t chara_num = text_.getNumCharactors();
    
    text_.setText(text);

    if (!resize) return;
    
    // 文字数が変わったら回転演出のバッファをレイアウトを更新
    size_t new_chara_num = text_.getNumCharactors();
    if (new_chara_num != chara_num) {
      rotate_.resize(new_chara_num);
      std::fill(std::begin(rotate_), std::end(rotate_), 0.0f);

      layout_->resizeWidget(text_.textSize());
    }
  }
  

  bool isDisp() const noexcept { return disp_; }
  void setDisp(const bool value) noexcept { disp_ = value; }
  
  bool isActive() const noexcept { return active_; }
  void setActive(const bool value) noexcept {
    // touch_eventがない場合は常にfalse
    active_ = touch_event_ ? value : false;
  }

  bool isTouchEvent() const noexcept { return touch_event_; }
  const std::string& eventMessage() const noexcept { return event_message_; }

  bool isTouchSound() const noexcept { return touch_sound_; }
  const std::string& soundMessage() const noexcept { return sound_message_; }

  
  bool intersects(const ci::Ray& ray) const noexcept {
    auto pos = pos_() + layout_->getPos();
    auto bbox = ci::AxisAlignedBox3f(pos + text_.minPos(), pos + text_.maxPos());

    return bbox.intersects(ray);
  }


  void draw(FontHolder& fonts, ModelHolder& models) noexcept {
    CubeTextDrawer::draw(text_, fonts.getFont(font_name_),
                         models.get(model_),
                         pos_() + layout_->getPos(), scale_(),
                         getTextColor(), getBaseColor(),
                         rotate_);
  }

  
#ifdef DEBUG
  void drawDebugInfo() noexcept {
    ci::gl::color(0, 1, 0);

    auto pos = pos_() + layout_->getPos();
    auto bbox = ci::AxisAlignedBox3f(pos + text_.minPos() - padding_, pos + text_.maxPos() + padding_);
    ci::gl::drawStrokedCube(bbox);
  }
#endif
  

  void startTween(const std::string& name) noexcept {
    if (!params_.hasChild("tween." + name)) return;
    
    const auto& tween = params_["tween." + name];
    makeTween(name, tween);
  }

  void resetTweens() noexcept {
    pos_.stop();
    scale_.stop();
    for (auto& r : rotate_) {
      r.stop();
    }

    base_color_.stop();
    text_color_.stop();

    pos_ = ci::Vec3f::zero();
    scale_ = ci::Vec3f::one();
    std::fill(std::begin(rotate_), std::end(rotate_), 0.0f);
    
    base_color_ = Json::getColor<float>(params_["base_color"]);
    text_color_ = Json::getColor<float>(params_["text_color"]);
  }

  
private:
  void makeTween(const std::string& name, const ci::JsonTree& tween) noexcept {
    std::set<std::string> apply;
    
    const auto& body = tween["body"];
    for (const auto& b : body) {
      std::map<std::string,
                      std::function<void (const ci::JsonTree&, const bool)> > func_tbl = {
        { "pos",
          [this](const ci::JsonTree& param, const bool is_first) {
            setVec3Tween(*timeline_, pos_, param, is_first);
          }
        },
        { "scale",
          [this](const ci::JsonTree& param, const bool is_first) {
            setVec3Tween(*timeline_, scale_, param, is_first);
          }
        },
        { "base_color",
          [this](const ci::JsonTree& param, const bool is_first) {
            if (hsv_) {
              setHsvTween(*timeline_, base_color_hsv_, param, is_first);
            }
            else {
              setColorTween(*timeline_, base_color_, param, is_first);
            }
          }
        },
        { "text_color",
          [this](const ci::JsonTree& param, const bool is_first) {
            if (hsv_) {
              setHsvTween(*timeline_, text_color_hsv_, param, is_first);
            }
            else {
              setColorTween(*timeline_, text_color_, param, is_first);
            }
          }
        },
        { "rotate",
          [this](const ci::JsonTree& param, const bool is_first) {
            setRotationTween(*timeline_, rotate_, param, is_first);
          }
        }
      };

      const auto& target = b["target"].getValue<std::string>();
      bool is_first = isFirstApply(target, apply);
      func_tbl[target](b, is_first);
    }

    if (tween.hasChild("next")) {
      const auto& name = tween["next"].getValue<std::string>();
      timeline_->add([this, name]() {
          startTween(name);
        },
        timeline_->getDuration());
    }
  }

  static void setRotationTween(ci::Timeline& timeline,
                               std::vector<ci::Anim<float> >& values, const ci::JsonTree& param,
                               const bool is_first) noexcept {
    float delta_interval = Json::getValue(param, "interval", 0.0f);
    float interval       = 0.0f;
    
    for (auto& value : values) {
      auto start = param.hasChild("start") ? ci::toRadians(param["start"].getValue<float>())
                                           : value();
      auto end = param.hasChild("end") ? ci::toRadians(param["end"].getValue<float>())
                                       : value();

      auto option = is_first ? timeline.apply(&value,
                                              start, end,
                                              param["duration"].getValue<float>(),
                                              getEaseFunc(param["type"].getValue<std::string>()))
                             : timeline.appendTo(&value,
                                                 end,
                                                 param["duration"].getValue<float>(),
                                                 getEaseFunc(param["type"].getValue<std::string>()));

      if (is_first) value = start;

      setTweenOption<float>(option, param);
      option.delay(interval);                       // delayは加算される
      interval += delta_interval;
    }
  }
  
};

}
