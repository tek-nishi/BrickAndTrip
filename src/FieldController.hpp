#pragma once

//
// ゲーム舞台のController
//

#include "ControllerBase.hpp"
#include "FieldView.hpp"
#include "FieldEntity.hpp"
#include "EventParam.hpp"


namespace ngs {

class FieldController : public ControllerBase {
  ci::JsonTree& params_;
  Event<std::vector<ngs::Touch> >& touch_event_;

  bool active_;
  
  Event<EventParam> event_;

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

    event_.connect("move-pickable-cube", [this](EventParam& param){
        entity_.movePickableCube(boost::any_cast<u_int>(param["cube_id"]),
                                 boost::any_cast<int>(param["move_direction"]));
      });
  }


private:
  bool isActive() const override { return active_; }

  Event<EventParam>& event() override { return event_; }

  
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
