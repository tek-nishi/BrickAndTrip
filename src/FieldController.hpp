#pragma once

//
// ゲーム舞台のController
//

#include "ControllerBase.hpp"
#include "FieldView.hpp"
#include "FieldEntity.hpp"


namespace ngs {

class FieldController : public ControllerBase {
  ci::JsonTree& params_;
  Event<std::vector<ngs::Touch> >& touch_event_;

  bool active_;
  
  Event<const std::string> event_;

  FieldView view_;
  FieldEntity entity_;
  

public:
  FieldController(ci::JsonTree& params, Event<std::vector<ngs::Touch> >& touch_event) :
    params_(params),
    touch_event_(touch_event),
    active_(true),
    view_(params, event_, touch_event),
    entity_(params, event_)
  {
    entity_.addCubeStage("startline.json");
    entity_.prepareStage();
  }


private:
  bool isActive() const override { return active_; }

  Event<const std::string>& event() override { return event_; }

  
  void resize() {
    view_.resize();
  }

  
  void update(const double progressing_seconds) override {
  }

  void draw() override {
    const auto field = entity_.fieldData();
    view_.draw(field);
  }

};

}
