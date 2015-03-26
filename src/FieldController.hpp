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

  bool stage_build_;
  bool stage_collapse_;
  

public:
  FieldController(ci::JsonTree& params, Event<std::vector<ngs::Touch> >& touch_event) :
    params_(params),
    touch_event_(touch_event),
    active_(true),
    view_(params, event_, touch_event),
    entity_(params, event_),
    stage_build_(false),
    stage_collapse_(false)
  {
    entity_.setupStartStage();

    event_.connect("move-pickable",
                   [this](const Connection&, EventParam& param) {
                     entity_.movePickableCube(boost::any_cast<u_int>(param["cube_id"]),
                                              boost::any_cast<int>(param["move_direction"]));
                   });

    event_.connect("all-pickable-started",
                   [this](const Connection& connection, EventParam& param) {
                     DOUT << "all-pickable-started" << std::endl;
                     entity_.startStageCollapse();
                   });

    event_.connect("pickable-moved",
                   [this](const Connection& connection, EventParam& param) {
                     entity_.startStageBuild();
                     connection.disconnect();
                   });

    event_.connect("all-pickable-finished",
                   [this](const Connection& connection, EventParam& param) {
                     DOUT << "all-pickable-finished" << std::endl;
                     entity_.completeBuildAndCollapseStage();
                   });

    event_.connect("stage-cleared",
                   [this](const Connection&, EventParam& param) {
                     DOUT << "stage-cleared" << std::endl;
                     entity_.startStageBuild();
                   });

    event_.connect("fall-pickable",
                   [this](const Connection&, EventParam& param) {
                     view_.cancelPicking(boost::any_cast<u_int>(param["id"]));
                   });
  }


private:
  bool isActive() const override { return active_; }

  Event<EventParam>& event() override { return event_; }

  
  void resize() {
    view_.resize();
  }

  
  void update(const double progressing_seconds) override {
    entity_.update(progressing_seconds);
  }

  void draw() override {
    const auto field = entity_.fieldData();
    view_.draw(field);
  }

};

}
