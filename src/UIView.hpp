#pragma once

//
// UI View
//

#include "cinder/Json.h"
#include "Autolayout.hpp"
#include "CubeTextDrawer.hpp"
#include "JsonUtil.hpp"
#include "Touch.hpp"
#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include "UIWidget.hpp"
#include "Event.hpp"
#include "EventParam.hpp"
#include "ConnectionHolder.hpp"
#include "FontHolder.hpp"
#include "ModelHolder.hpp"
#include "SoundRequest.hpp"


namespace ngs {

class UIView : private boost::noncopyable {
  bool disp_;
  bool active_;
#ifdef DEBUG
  bool hide_;
  bool debug_info_;
#endif

  ci::Camera& camera_;

  Event<EventParam>& event_;
  ConnectionHolder connections_;

  std::vector<std::unique_ptr<UIWidget> > widgets_;

  // 入力処理用
  bool  touching_;
  u_int touching_id_;
  UIWidget* touching_widget_;
  bool  touch_in_;
  
  
public:
  explicit UIView(const ci::JsonTree& params,
                  const ci::JsonTree& widget_params,
                  ci::TimelineRef timeline,
                  ci::Camera& camera,
                  Autolayout& autolayout,
                  Event<EventParam>& event,
                  Event<std::vector<Touch> >& touch_event) noexcept :
    disp_(true),
    active_(true),
#ifdef DEBUG
    hide_(false),
    debug_info_(params["ui_view.debug_info"].getValue<bool>()),
#endif
    camera_(camera),
    event_(event),
    touching_(false)
  {
    DOUT << "UIView" << std::endl;

    float padding = params["ui_view.widget.padding"].getValue<float>();
    for (const auto p : widget_params) {
      auto widget = std::unique_ptr<UIWidget>(new UIWidget(p, timeline, autolayout, padding));
      widgets_.push_back(std::move(widget));
    }

    // Touch Eventを登録
    connections_ += touch_event.connect("touches-began",
                                        std::bind(&UIView::touchesBegan, 
                                                  this, std::placeholders::_1, std::placeholders::_2));
    connections_ += touch_event.connect("touches-moved",
                                        std::bind(&UIView::touchesMoved, 
                                                  this, std::placeholders::_1, std::placeholders::_2));
                                        
    connections_ += touch_event.connect("touches-ended",
                                        std::bind(&UIView::touchesEnded, 
                                                  this, std::placeholders::_1, std::placeholders::_2));

#ifdef DEBUG
    connections_ += event_.connect("toggle-ui-hide",
                                   [this](const Connection&, EventParam& param) {
                                     hide_ = !hide_;
                                   });
#endif
  }


  bool isDisp() const noexcept { return disp_; }
  void setDisp(const bool value) noexcept { disp_ = value; }
  
  bool isActive() const noexcept { return active_; }
  void setActive(const bool active) noexcept {
    active_ = active;

    // OFFにする時はタッチ中のWidgetの後処理を
    if (!active && touching_) {
      touching_widget_->startTween("tween-touch-out");
    }
    touching_ = false;
  }
  
  
  void draw(FontHolder& fonts, ModelHolder& models) noexcept {
#ifdef DEBUG
    if (hide_) return;
#endif
    if (!disp_) return;

    ci::gl::disableDepthRead();
    ci::gl::disableDepthWrite();
    ci::gl::disable(GL_LIGHTING);

    ci::gl::setMatrices(camera_);

    for (auto& widget : widgets_) {
      if (!widget->isDisp()) continue;
      
      widget->draw(fonts, models);

#ifdef DEBUG
      if (debug_info_) {
        widget->drawDebugInfo();
      }
#endif
    }
  }

  
  void startWidgetTween(const std::string& name) noexcept {
    for (auto& widget : widgets_) {
      widget->startTween(name);
    }
  }

  
  UIWidget& getWidget(const std::string& name) noexcept {
    auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
                           [name](const std::unique_ptr<UIWidget>& w) {
                             return w->getName() == name;
                           });
    assert(it != std::end(widgets_));
    return *(*it);
  }

  void resetWidgetTween() noexcept {
    for (auto& widget : widgets_) {
      widget->resetTweens();
    }
  }

  
private:
  void touchesBegan(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!active_ || touching_) return;

    for (const auto& touch : touches) {
      float u = touch.pos.x / (float)ci::app::getWindowWidth();
      float v = touch.pos.y / (float)ci::app::getWindowHeight();

      auto ray = camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
    
      for (auto& widget : widgets_) {
        if (!widget->isActive() || !widget->isTouchEvent()) continue;

        if (widget->intersects(ray)) {
          touching_        = true;
          touching_id_     = touch.id;
          touching_widget_ = &(*widget);
          touch_in_ = true;

          widget->startTween("tween-touch-in");
          
          return;
        }
      }
    }
  }
  
  void touchesMoved(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!touching_) return;

    auto it = std::find(std::begin(touches), std::end(touches), touching_id_);
    if (it == std::end(touches)) return;

    float u = it->pos.x / (float)ci::app::getWindowWidth();
    float v = it->pos.y / (float)ci::app::getWindowHeight();

    auto ray = camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
    bool touch_in = touching_widget_->intersects(ray);
    if (touch_in != touch_in_) {
      touching_widget_->startTween(touch_in ? "tween-touch-in" : "tween-touch-out");
      touch_in_ = touch_in;
    }
  }
  
  void touchesEnded(const Connection&, std::vector<Touch>& touches) noexcept {
    if (!touching_) return;
    
    auto it = std::find(std::begin(touches), std::end(touches), touching_id_);
    if (it == std::end(touches)) return;

    float u = it->pos.x / (float)ci::app::getWindowWidth();
    float v = it->pos.y / (float)ci::app::getWindowHeight();

    auto ray = camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
    bool touch_in = touching_widget_->intersects(ray);
    if (touch_in || (touch_in != touch_in_)) {
      touching_widget_->startTween(touch_in ? "tween-touch-end" : "tween-touch-out");
    }
    
    // signal先で自分自身のsetActiveが呼ばれるので、ここで更新
    touching_ = false;

    if (touch_in) {
      EventParam params = {
        { "widget", touching_widget_->getName() },
      };
      event_.signal(touching_widget_->eventMessage(), params);

      if (touching_widget_->isTouchSound()) {
        requestSound(event_, touching_widget_->soundMessage());
      }
    }
  }
  
};

}
