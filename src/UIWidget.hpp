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
#include <set>


namespace ngs {

class UIWidget {
  const ci::JsonTree params_;

  std::string name_;

  CubeText text_;

  ci::Anim<ci::Vec3f> pos_;
  ci::Anim<ci::Vec3f> scale_;
  std::vector<ci::Anim<float> > rotate_;
  
  ci::Anim<ci::Color> base_color_;
  ci::Anim<ci::Color> text_color_;

  Autolayout::WidgetRef layout_;
  ci::TimelineRef timeline_;

  
  // dispは表示のON/OFF
  // activeはタッチイベントのON/OFF
  bool disp_;
  bool active_;

  bool touch_event_;
  std::string event_message_;

  
public:
  UIWidget(const ci::JsonTree& params, Autolayout& autolayout) :
    params_(params),
    name_(params["name"].getValue<std::string>()),
    text_(params["text"].getValue<std::string>(),
          params["size"].getValue<float>(),
          params["spacing"].getValue<float>(),
          params["chara_split"].getValue<size_t>()),
    pos_(ci::Vec3f::zero()),
    scale_(ci::Vec3f::one()),
    rotate_(text_.text().size()),
    base_color_(Json::getColor<float>(params["base_color"])),
    text_color_(Json::getColor<float>(params["text_color"])),
    timeline_(ci::Timeline::create()),
    disp_(true),
    active_(false),
    touch_event_(false)
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

    disp_   = Json::getValue(params, "disp", disp_);

    // touchEventがなければactiveは常にfalse
    active_ = touch_event_ ? Json::getValue(params, "active", active_) : false;

    timeline_->setStartTime(ci::app::timeline().getCurrentTime());
    ci::app::timeline().apply(timeline_);
  }

  ~UIWidget() {
    // 再生途中のものもあるので、自分で取り除く
    timeline_->removeSelf();
  }

  
  const std::string& getName() const { return name_; }

  const ci::Color& getBaseColor() const { return base_color_; }
  const ci::Color& getTextColor() const { return text_color_; }

  void setBaseColor(const ci::Color& color) {
    base_color_ = color;
  }

  void setTextColor(const ci::Color& color) {
    text_color_ = color;
  }
  
  
  CubeText& getCubeText() { return text_; }

  bool isDisp() const { return disp_; }
  void setDisp(const bool value) { disp_ = value; }
  
  bool isActive() const { return active_; }
  void setActive(const bool value) {
    // touch_eventがない場合は常にfalse
    active_ = touch_event_ ? value : false;
  }

  bool isTouchEvent() const { return touch_event_; }
  const std::string& eventMessage() const { return event_message_; }
  
  bool intersects(const ci::Ray& ray) const {
    auto pos = pos_() + layout_->getPos();
    auto bbox = ci::AxisAlignedBox3f(pos + text_.minPos(), pos + text_.maxPos());

    return bbox.intersects(ray);
  }


  void draw(TextureFont& font) {
    CubeTextDrawer::draw(text_, font,
                         pos_() + layout_->getPos(), scale_(),
                         text_color_(), base_color_(),
                         rotate_);
  }


  void startTween(const std::string& name) {
    if (!params_.hasChild("tween." + name)) return;
    
    const auto& tween = params_["tween." + name];
    makeTween(name, tween);
  }

  
private:
  void makeTween(const std::string& name, const ci::JsonTree& tween) {
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
            setColorTween(*timeline_, base_color_, param, is_first);
          }
        },
        { "text_color",
          [this](const ci::JsonTree& param, const bool is_first) {
            setColorTween(*timeline_, text_color_, param, is_first);
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

  
  static void setVec3Tween(ci::Timeline& timeline,
                           ci::Anim<ci::Vec3f>& value, const ci::JsonTree& param, const bool is_first) {
    auto start = param.hasChild("start") ? Json::getVec3<float>(param["start"])
                                         : value();
    auto end = param.hasChild("end") ? Json::getVec3<float>(param["end"])
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
    setTweenOption<ci::Vec3f>(option, param);
  }
  
  static void setColorTween(ci::Timeline& timeline,
                            ci::Anim<ci::Color>& value, const ci::JsonTree& param, const bool is_first) {
    auto start = param.hasChild("start") ? Json::getColor<float>(param["start"])
                                         : value();
    auto end = param.hasChild("end") ? Json::getColor<float>(param["end"])
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
    setTweenOption<ci::Color>(option, param);
  }

  static void setRotationTween(ci::Timeline& timeline,
                               std::vector<ci::Anim<float> >& values, const ci::JsonTree& param, const bool is_first) {
    float delta_interval = param.hasChild("interval") ? param["interval"].getValue<float>()
                                                      : 0.0f;
    float interval = 0.0f;
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

      float delay = interval;
      if (param.hasChild("delay")) {
        delay += param["delay"].getValue<float>();
      }
      option.delay(delay);

      interval += delta_interval;
    }
  }
  
  template <typename T>
  static void setTweenOption(typename ci::Tween<T>::Options& option, const ci::JsonTree& param) {
    if (param.hasChild("loop")) {
      option.loop(param["loop"].getValue<bool>());
    }
    if (param.hasChild("ping-pong")) {
      option.pingPong(param["ping-pong"].getValue<bool>());
    }
    if (param.hasChild("delay")) {
      option.delay(param["delay"].getValue<float>());
    }
  }
  
  static bool isFirstApply(const std::string& type, std::set<std::string>& apply) {
    auto result = apply.insert(type);
    return result.second;
  }
  
};

}
