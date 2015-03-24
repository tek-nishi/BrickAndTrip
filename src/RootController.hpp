#pragma once

//
// アプリ最上位のController定義
//

#include "ControllerBase.hpp"
#include <boost/range/algorithm_ext/erase.hpp>
#include "FieldController.hpp"


namespace ngs {

class RootController : public ControllerBase {
  ci::JsonTree& params_;
  Event<std::vector<ngs::Touch> >& touch_event_;
  
  Event<const std::string> event_;
  
  using ControllerPtr = std::unique_ptr<ControllerBase>;
  std::vector<ControllerPtr> children_;


public:
  RootController(ci::JsonTree& params, Event<std::vector<ngs::Touch> >& touch_event) :
    params_(params),
    touch_event_(touch_event)
  {
    // TODO:最初のControllerを追加
    auto controller = std::unique_ptr<ControllerBase>(new FieldController(params, touch_event_));
    children_.push_back(std::move(controller));
  }


private:
  bool isActive() const override { return true; }

  Event<const std::string>& event() override { return event_; }

  void resize() {
    for (auto& child : children_) {
      child->resize();
    }
  }
  
  void update(const double progressing_seconds) override {
    // 更新しつつ、無効なControllerを削除
    boost::remove_erase_if(children_,
                           [progressing_seconds](ControllerPtr& child) {
                             child->update(progressing_seconds);
                             return !child->isActive();
                           });
  }
  
  void draw() override {
    for (auto& child : children_) {
      child->draw();
    }
  }
  
};

}
