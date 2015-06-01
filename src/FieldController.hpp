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
                  Event<EventParam>& event,
                  Records& records) :
    params_(params),
    touch_event_(touch_event),
    event_(event),
    timeline_(ci::Timeline::create()),
    paused_(false),
    active_(true),
    view_(params, timeline_, event_, touch_event),
    entity_(params, timeline_, event_, records),
    stage_cleard_(false),
    stageclear_agree_(false)
  {
    DOUT << "FieldController()" << std::endl;
    
    connections_ += event_.connect("picking-start",
                                   [this](const Connection&, EventParam& param) {
                                     entity_.pickPickableCube(boost::any_cast<u_int>(param["cube_id"]));
                                   });
    
    connections_ += event_.connect("move-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     entity_.movePickableCube(boost::any_cast<u_int>(param["cube_id"]),
                                                              boost::any_cast<int>(param["move_direction"]),
                                                              boost::any_cast<int>(param["move_speed"]));
                                   });

    connections_ += event_.connect("pickable-moved",
                                   [this](const Connection&, EventParam& param) {
                                     const auto& block_pos = boost::any_cast<const ci::Vec3i&>(param["block_pos"]);
                                     const auto id = boost::any_cast<u_int>(param["id"]);
                                     entity_.movedPickableCube(id, block_pos);
                                   });
    
    connections_ += event_.connect("pickable-on-stage",
                                   [this](const Connection& connection, EventParam& param) {
                                   });

    // pickablecubeの1つがstartlineを越えたらcollapse開始
    connections_ += event_.connect("first-pickable-started",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "first-pickable-started" << std::endl;
                                     const auto& color = boost::any_cast<const ci::Color&>(param["bg_color"]);
                                     view_.setStageBgColor(color);
                                     
                                     const auto& light_tween = boost::any_cast<const std::string&>(param["light_tween"]);
                                     view_.setStageLightTween(light_tween);

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
                                     entity_.cancelPickPickableCubes();
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

    
    connections_ += event_.connect("regular-stage-clear-out",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "regular-stage-clear-out" << std::endl;
                                     view_.enableTouchInput();
                                     
                                   });

    connections_ += event_.connect("all-stage-clear-out",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "all-stage-clear-out" << std::endl;
                                     entity_.riseAllPickableCube();
                                   });

    connections_ += event_.connect("collapse-stage",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "collapse-stage" << std::endl;
                                     entity_.collapseStage();
                                   });
    
    connections_ += event_.connect("back-to-title",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "back-to-title" << std::endl;
                                     entity_.cleanupField();
                                   });


    connections_ += event_.connect("build-one-line",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "build-one-line" << std::endl;
                                     int active_top_z = boost::any_cast<int>(param["active_top_z"]);
                                     entity_.entryStageObjects(active_top_z);
                                   });

#if 0
    connections_ += event_.connect("collapse-one-line",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "collapse-one-line" << std::endl;
                                   });
#endif

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
                                     beginGameover();
                                   });

    // ドッスンに踏まれた
    connections_ += event_.connect("pressed-pickable",
                                   [this](const Connection&, EventParam& param) {
                                     DOUT << "pressed-pickable" << std::endl;
                                     beginGameover();
                                   });

#if 0
    connections_ += event_.connect("fall-all-pickable",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "fall-all-pickable" << std::endl;
                                     entity_.cancelPickPickableCubes();
                                     view_.enableTouchInput(false);
                                     entity_.gameover();
                                   });
#endif

    connections_ += event_.connect("pickable-start-idle",
                                   [this](const Connection&, EventParam& param) {
                                     u_int id = boost::any_cast<u_int>(param["id"]);
                                     entity_.startIdlePickableCube(id);
                                   });

    
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

                                     // 中断、全クリの時はタイトル起動
                                     bool all_cleard = boost::any_cast<bool>(param.at("all_cleard"));
                                     bool aborted = boost::any_cast<bool>(param.at("game_aborted"));
                                     if (all_cleard || aborted) {
                                       event_.signal("begin-title", EventParam());
                                     }
                                   });

    connections_ += event.connect("title-started",
                                  [this](const Connection&, EventParam& param) {
                                    view_.enableTouchInput();
                                  });
                                  

    connections_ += event_.connect("pause-start",
                                   [this](const Connection&, EventParam& param) {
                                     entity_.cancelPickPickableCubes();
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
                                     view_.enableFollowCamera(false);
                                     paused_ = false;
                                     entity_.abortGame();
                                   });

    connections_ += event_.connect("pickuped-item",
                                   [this](const Connection& connection, EventParam& param) {
                                     DOUT << "pickuped-item" << std::endl;
                                     entity_.pickupedItemCube();
                                   });

    
    connections_ += event_.connect("field-input-stop",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.cancelPickPickableCubes();
                                     entity_.enablePickableCubeMovedEvent(false);
                                     view_.enableTouchInput(false);
                                   });

    connections_ += event_.connect("field-input-start",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.enablePickableCubeMovedEvent();
                                     view_.enableTouchInput();
                                   });

    connections_ += event_.connect("stage-color",
                                   [this](const Connection& connection, EventParam& param) {
                                     const auto& color = boost::any_cast<const ci::Color&>(param["bg_color"]);
                                     view_.setStageBgColor(color);

                                     const auto& light_tween = boost::any_cast<const std::string&>(param["light_tween"]);
                                     view_.setStageLightTween(light_tween);
                                   });

    connections_ += event_.connect("falling-down",
                                   [this](const Connection& connection, EventParam& param) {
                                     float duration = boost::any_cast<float>(param["duration"]);
                                     view_.startQuake(duration);
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

    connections_ += event_.connect("entry-pickable",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.entryPickableCube();
                                   });
#endif

    setup();
  }

  ~FieldController() {
    DOUT << "~FieldController()" << std::endl;

    // 再生途中のものもあるので、手動で取り除く
    timeline_->removeSelf();
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

  void draw(FontHolder& fonts, ModelHolder& models) override {
    const auto field = entity_.fieldData();
    view_.draw(field, models);
  }

  
  void setup() {
    connections_ += event_.connect("pickable-moved",
                                   [this](const Connection& connection, EventParam& param) {
                                     entity_.startStageBuild();
                                     timeline_->add([this]() {
                                         event_.signal("begin-progress", EventParam());
                                       },
                                       timeline_->getCurrentTime() + 1.0f);
                                       
                                     connection.disconnect();
                                   });
    view_.enableTouchInput(false);
    view_.resetCamera();
    view_.enableFollowCamera();
    entity_.setupStartStage();
  }

  void startNextStage() {
    view_.enableTouchInput();
    entity_.startStageBuild();

    stage_cleard_     = false;
    stageclear_agree_ = false;
  }

  void beginGameover() {
    entity_.cancelPickPickableCubes();
    view_.enableTouchInput(false);
    view_.enableFollowCamera(false);
    entity_.gameover();
  }
  
};

}
