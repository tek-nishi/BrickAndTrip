#pragma once

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

  ci::TimelineRef timeline_;
  bool paused_;

  ConnectionHolder connections_;
  
  bool active_;

  FieldView view_;
  FieldEntity entity_;

  bool stage_cleard_;
  bool stageclear_agree_;
  

public:
  FieldController(ci::JsonTree& params,
                  Event<std::vector<Touch> >& touch_event,
                  Event<EventParam>& event) :
    params_(params),
    touch_event_(touch_event),
    event_(event),
    timeline_(ci::Timeline::create()),
    paused_(false),
    active_(true),
    view_(params, timeline_, event_, touch_event),
    entity_(params, timeline_, event_),
    stage_cleard_(false),
    stageclear_agree_(false)
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

    // pickablecubeの1つがstartlineを越えたらcollapse開始
    connections_ += event_.connect("first-pickable-started",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "first-pickable-started" << std::endl;
                                     entity_.startStageCollapse();
                                   });
    
    connections_ += event_.connect("all-pickable-started",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "all-pickable-started" << std::endl;
                                   });

    connections_ += event_.connect("all-pickable-finished",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "all-pickable-finished" << std::endl;
                                     entity_.completeBuildAndCollapseStage();
                                     view_.enableTouchInput(false);
                                   });

    // stage-clearedとstageclear-agreeの両方が発行されたら次のステージへ
    connections_ += event_.connect("stage-cleared",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "stage-cleared" << std::endl;
                                     stage_cleard_ = true;

                                     if (stage_cleard_ && stageclear_agree_) {
                                       startNextStage();
                                     }
                                   });
    
    connections_ += event_.connect("stageclear-agree",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "stageclear-agree" << std::endl;
                                     stageclear_agree_ = true;                                     
                                     view_.enableTouchInput();

                                     if (stage_cleard_ && stageclear_agree_) {
                                       startNextStage();
                                     }
                                   });


    connections_ += event_.connect("build-finish-line",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "build-finish-line" << std::endl;
                                     entity_.entryPickableCubes();
                                   });

    connections_ += event_.connect("fall-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "fall-pickable" << std::endl;
                                     // view_.cancelPicking(boost::any_cast<u_int>(param["id"]));
                                   });

    // pickablecubeの1つがfallでgameover
    connections_ += event_.connect("first-fallen-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "first-fallen-pickable" << std::endl;
                                     view_.enableTouchInput(false);
                                     entity_.gameover();
                                   });

#if 0
    connections_ += event_.connect("fall-all-pickable",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "fall-all-pickable" << std::endl;
                                     view_.calcelAllPickings();
                                     entity_.gameover();
                                   });
#endif

    connections_ += event_.connect("startline-opened",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "startline-opened" << std::endl;
                                     entity_.enableRecordPlay();
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

    connections_ += event_.connect("pause-start",
                                   [this](const Connection&, EventParam& param) {
                                     view_.enableTouchInput(false);
                                     paused_ = true;
                                   });

    connections_ += event_.connect("game-continue",
                                   [this](const Connection&, EventParam& param) {
                                     view_.enableTouchInput();
                                     paused_ = false;
                                   });
    
    connections_ += event_.connect("game-abort",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "game-abort" << std::endl;
                                     view_.enableTouchInput();
                                     paused_ = false;
                                     entity_.cleanupField();
                                   });

#ifdef DEBUG
    connections_ += event_.connect("force-collapse",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.startStageCollapse();
                                   });
    
    connections_ += event_.connect("stop-build-and-collapse",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.stopBuildAndCollapse();
                                   });
#endif
  }

  ~FieldController() {
    // 再生途中のものもあるので、手動で取り除く
    timeline_->removeSelf();;
  }


private:
  bool isActive() const override { return active_; }

  Event<EventParam>& event() override { return event_; }

  
  void resize() override {
    view_.resize();
  }

  
  void update(const double progressing_seconds) override {
    if (paused_) return;
    
    timeline_->step(progressing_seconds);
    entity_.update(progressing_seconds);
    view_.update(progressing_seconds);
  }

  void draw(FontHolder& fonts) override {
    const auto field = entity_.fieldData();
    view_.draw(field);
  }

  
  void setup() {
    view_.enableTouchInput();
    entity_.setupStartStage();

    connections_ += event_.connect("pickable-moved",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.startStageBuild();
                                     timeline_->add([this]() {
                                         event_.signal("begin-progress", EventParam());
                                       },
                                       timeline_->getCurrentTime() + 1.0f);
                                       
                                     connection.disconnect();
                                   });
  }

  void startNextStage() {
    view_.enableTouchInput();
    entity_.startStageBuild();

    stage_cleard_     = false;
    stageclear_agree_ = false;
  }
  
};

}
