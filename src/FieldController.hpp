﻿#pragma once

//
// ゲーム舞台のController
//

#include "ControllerBase.hpp"
#include "FieldView.hpp"
#include "FieldEntity.hpp"
#include "EventParam.hpp"
#include "ConnectionHolder.hpp"


namespace ngs {

class FieldController : public ControllerBase {
  ci::JsonTree& params_;
  Event<std::vector<Touch> >& touch_event_;
  Event<EventParam>& event_;

  ConnectionHolder connections_;
  
  bool active_;

  FieldView view_;
  FieldEntity entity_;

  bool stage_build_;
  bool stage_collapse_;
  

public:
  FieldController(ci::JsonTree& params,
                  Event<std::vector<Touch> >& touch_event,
                  Event<EventParam>& event) :
    params_(params),
    touch_event_(touch_event),
    event_(event),
    active_(true),
    view_(params, event_, touch_event),
    entity_(params, event_),
    stage_build_(false),
    stage_collapse_(false)
  {
    setup();

    connections_ += event_.connect("move-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     entity_.movePickableCube(boost::any_cast<u_int>(param["cube_id"]),
                                                              boost::any_cast<int>(param["move_direction"]),
                                                              boost::any_cast<int>(param["move_speed"]));
                                   });

    connections_ += event_.connect("pickable-moved",
                                   [this](const Connection&, EventParam& param) {
                                     entity_.movedPickableCube();
                                   });
    
    connections_ += event_.connect("pickable-on-stage",
                                   [this](const Connection& connection, EventParam& param) {
                                   });
    
    connections_ += event_.connect("all-pickable-started",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "all-pickable-started" << std::endl;
                                     entity_.startStageCollapse();
                                   });

    connections_ += event_.connect("all-pickable-finished",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "all-pickable-finished" << std::endl;
                                     entity_.completeBuildAndCollapseStage();
                                   });

    connections_ += event_.connect("stage-cleared",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "stage-cleared" << std::endl;
                                     entity_.startStageBuild();
                                   });

    connections_ += event_.connect("build-finish-line",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "build-finish-line" << std::endl;
                                     entity_.entryPickableCubes();
                                   });

    connections_ += event_.connect("fall-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "fall-pickable" << std::endl;
                                     view_.cancelPicking(boost::any_cast<u_int>(param["id"]));
                                   });


    connections_ += event_.connect("fall-all-pickable",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "fall-all-pickable" << std::endl;
                                     view_.calcelAllPickings();
                                     entity_.gameover();
                                   });
    
    connections_ += event_.connect("gameover-agree",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "gameover-agree" << std::endl;
                                     entity_.cleanupField();
                                   });

    connections_ += event_.connect("stage-all-collapsed",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "stage-all-collapsed" << std::endl;
                                     entity_.restart();
                                     setup();
                                   });
  }


private:
  bool isActive() const override { return active_; }

  Event<EventParam>& event() override { return event_; }

  
  void resize() override {
    view_.resize();
  }

  
  void update(const double progressing_seconds) override {
    entity_.update(progressing_seconds);
  }

  void draw(FontHolder& fonts) override {
    const auto field = entity_.fieldData();
    view_.draw(field);
  }

  
  void setup() {
    entity_.setupStartStage();
    
    connections_ += event_.connect("pickable-moved",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.startStageBuild();
                                     connection.disconnect();
                                   });
  }
  
};

}
