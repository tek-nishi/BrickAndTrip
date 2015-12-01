#pragma once

//
// 立方体で構成された移動場所
//

#include <vector>
#include <deque>
#include <limits>
#include <boost/noncopyable.hpp>
#include "StageCube.hpp"
#include "EasingUtil.hpp"


namespace ngs {

class Stage : private boost::noncopyable {
  Event<EventParam>& event_;
  
  // ステージ全体
  std::deque<std::vector<StageCube> > cubes_;

  // 現在表示中のCube
  std::deque<std::vector<StageCube> > active_cubes_;

  // 崩れ中のCube
  std::deque<std::vector<StageCube> > collapse_cubes_;
  
  int top_z_;
  int active_top_z_;
  int finish_line_z_;
  
  float build_speed_;
  float collapse_speed_;
  float auto_collapse_;

  ci::Anim<float> build_speed_rate_;
  float collapse_speed_rate_;
  
  std::string build_ease_;
  float       build_duration_;
  ci::Vec2f   build_y_;

  std::string collapse_ease_;
  float       collapse_duration_;
  ci::Vec2f   collapse_y_;

  std::string open_ease_;
  float       open_duration_;
  float       open_delay_;

  std::string move_ease_;
  float       move_duration_;
  float       move_delay_;

  std::string build_start_ease_;
  float       build_start_duration_;
  float       build_start_rate_;
  
  bool started_collapse_;
  bool finished_build_;
  bool finished_collapse_;

  ci::Vec2i stage_width_;
  
  ci::TimelineRef event_timeline_;
  ci::TimelineRef animation_timeline_;
  

public:
  Stage(const ci::JsonTree& params,
        ci::TimelineRef timeline,
        Event<EventParam>& event) noexcept :
    event_(event),
    top_z_(0),
    active_top_z_(0),
    finish_line_z_(-1),
    build_speed_(params["game.stage.build_speed"].getValue<float>()),
    collapse_speed_(params["game.stage.collapse_speed"].getValue<float>()),
    auto_collapse_(params["game.stage.auto_collapse"].getValue<float>()),
    build_speed_rate_(1.0f),
    collapse_speed_rate_(1.0f),
    build_ease_(params["game.stage.build_ease"].getValue<std::string>()),
    build_duration_(params["game.stage.build_duration"].getValue<float>()),
    build_y_(Json::getVec2<float>(params["game.stage.build_y"])),
    collapse_ease_(params["game.stage.collapse_ease"].getValue<std::string>()),
    collapse_duration_(params["game.stage.collapse_duration"].getValue<float>()),
    collapse_y_(Json::getVec2<float>(params["game.stage.collapse_y"])),
    open_ease_(params["game.stage.open_ease"].getValue<std::string>()),
    open_duration_(params["game.stage.open_duration"].getValue<float>()),
    open_delay_(params["game.stage.open_delay"].getValue<float>()),
    move_ease_(params["game.stage.move_ease"].getValue<std::string>()),
    move_duration_(params["game.stage.move_duration"].getValue<float>()),
    move_delay_(params["game.stage.move_delay"].getValue<float>()),
    build_start_ease_(params["game.stage.build_start_ease"].getValue<std::string>()),
    build_start_duration_(params["game.stage.build_start_duration"].getValue<float>()),
    build_start_rate_(params["game.stage.build_start_rate"].getValue<float>()),
    started_collapse_(false),
    finished_build_(false),
    finished_collapse_(false),
    stage_width_(ci::Vec2i::zero()),
    event_timeline_(ci::Timeline::create()),
    animation_timeline_(ci::Timeline::create())
  {
    auto current_time = timeline->getCurrentTime();
    event_timeline_->setStartTime(current_time);
    animation_timeline_->setStartTime(current_time);

    timeline->apply(event_timeline_);
    timeline->apply(animation_timeline_);
  }

  ~Stage() {
    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
    animation_timeline_->removeSelf();
  }


  // 生成開始
  void startBuildStage(const float speed_rate, const bool start_speedup) noexcept {
    if (start_speedup) {
      animation_timeline_->apply(&build_speed_rate_,
                                 speed_rate * build_start_rate_, speed_rate,
                                 build_start_duration_,
                                 getEaseFunc(build_start_ease_));
    }
    else {
      build_speed_rate_.stop();
      build_speed_rate_ = speed_rate;
    }
    
    buildStage();
  }

  // 生成速度の変更
  void setBuildSpeedRate(const float speed_rate) noexcept {
    build_speed_rate_ = speed_rate;
  }

  // 崩壊開始
  void startCollapseStage(const int stop_z, const float speed_rate = 1.0f) noexcept {
    collapse_speed_rate_ = speed_rate;
    collapseStage(stop_z);
  }

  
  // 生成 & 崩壊を止める
  void stopBuildAndCollapse() noexcept {
    event_timeline_->clear();
    build_speed_rate_.stop();
  }

  // BuildとCollapseが完了しているか判定
  bool isFinishedBuildAndCollapse() noexcept {
    if (!finished_build_ || !finished_collapse_) return false;

    // Build演出の完了も調べる
    for (const auto& cube : topLine()) {
      if (!cube.position.isComplete()) return false;
    }
    
    return true;
  }

  // 崩壊完了()
  bool isFinishedCollapse() noexcept {
    return active_cubes_.empty();
  }

  void restart(const int restart_z) noexcept {
    stopBuildAndCollapse();

    top_z_         = restart_z;
    active_top_z_  = top_z_;
    finish_line_z_ = -1;

    started_collapse_  = false;
    finished_build_    = false;
    finished_collapse_ = false;

    cubes_.clear();
  }
  

  // StartLineを下げる
  void openStartLine() noexcept {
    size_t iz = active_cubes_.size() - 1;

    for (auto& cube : active_cubes_[iz]) {
      ci::Vec3f end_value(cube.position() + ci::Vec3f(0.0f, -1.0f, 0.0f));
      auto option = animation_timeline_->apply(&cube.position, end_value,
                                               open_duration_, getEaseFunc(open_ease_));

      option.delay(open_delay_);
    }

    event_timeline_->add([this]() noexcept {
        event_.signal("startline-will-open", EventParam());
      }, event_timeline_->getCurrentTime() + open_delay_);
    
    event_timeline_->add([this, iz]() noexcept {
        // コンテナへの参照が無効になっている場合があるので、関数経由で取得
        for (auto& cube : active_cubes_[iz]) {
          cube.block_position.y -= 1;
        }
        event_.signal("startline-opened", EventParam());
      },
      event_timeline_->getCurrentTime() + open_delay_ + open_duration_);
  }

  // stageの自動崩壊を仕掛ける
  void setupAutoCollapse(const int stop_z, const float speed_rate = 1.0f) noexcept {
    started_collapse_ = false;

    event_timeline_->add([this, stop_z, speed_rate]() noexcept {
        if (!started_collapse_) {
          DOUT << "auto collapse:" << auto_collapse_ << std::endl;
          startCollapseStage(stop_z, speed_rate);
        }
      },
      event_timeline_->getCurrentTime() + auto_collapse_);
  }

  bool isStartedCollapse() const noexcept { return started_collapse_; }

  void setFinishLine(const int z) noexcept {
    finish_line_z_ = z;
  }
  

  int getTopZ() const noexcept { return top_z_; }
  int getActiveTopZ() const noexcept { return active_top_z_; }
  int getActiveBottomZ() const noexcept { return active_top_z_ - int(active_cubes_.size()); }

  const ci::Vec2i& getStageWidth() const noexcept { return stage_width_; }
  
  int addCubes(const ci::JsonTree& stage_data,
               const int x_offset,
               const std::vector<ci::Color>& cube_color,
               const ci::Color& line_color) noexcept {
    // finishlineにはデータが入っていてはいけない
    build_speed_    = Json::getValue<float>(stage_data, "build_speed", build_speed_);
    collapse_speed_ = Json::getValue<float>(stage_data, "collapse_speed", collapse_speed_);
    auto_collapse_  = Json::getValue<float>(stage_data, "auto_collapse", auto_collapse_);

    const auto stage_color = Json::getColor<float>(stage_data["color"]);

    stage_width_.x = x_offset;
    // yにはstageの最大値を入れたいので、初期値として最小値を入れておく
    stage_width_.y = std::numeric_limits<int>::min();
    
    
    const auto& body = stage_data["body"];
    int last_iz = int(body.getNumChildren()) - 1;
    int iz = 0;
    for (const auto& row : body) {
      std::vector<StageCube> cube_row;
      cube_row.reserve(16);
      
      int x = x_offset;
      int z = iz + top_z_;

      const auto& base_color = (iz == last_iz) ? line_color
                                               : stage_color;
      
      for (const auto& p : row) {
        int y = p.getValue<int>();
        
        if (y >= 0) {
          ci::Vec3i block_pos(x, y, z);
          ci::Vec3f pos = ci::Vec3f(x, y, z);

          StageCube cube = {
            pos,
            ci::Quatf::identity(),
            block_pos,
            block_pos,
            base_color * cube_color[(x + z) & 1],
            true, true
          };

          cube_row.push_back(std::move(cube));
        }
        x += 1;
      }
      cubes_.push_back(std::move(cube_row));

      // stageのxの最大値を保持
      if (x > stage_width_.y) stage_width_.y = x;
      
      iz += 1;
    }

    top_z_ += body.getNumChildren();
    
    return top_z_; 
  }

  
  // 「この場所にはCubeが無い」も結果に含めるので、std::pairを利用
  std::pair<bool, int> getStageHeight(const ci::Vec3i& block_pos) const noexcept {
    const auto* const cube = getStageCube(block_pos);
    if (!cube) return std::make_pair(false, 0);
    
    return std::make_pair(cube->can_ride, cube->block_position.y);
  }

  void moveStageCube(const ci::Vec3i& block_pos) noexcept {
    auto* const target = getStageCube(block_pos);
    if (!target) return;

    moveStageCube(*target);
  }

  void cleanup() noexcept {
    event_timeline_->clear();
  }


  float buildSpeed() const noexcept { return build_speed_; }
  
  
  const std::deque<std::vector<StageCube> >& activeCubes() const noexcept {
    return active_cubes_;
  }

  const std::deque<std::vector<StageCube> >& collapseCubes() const noexcept {
    return collapse_cubes_;
  }

  
private:
  // 生成(再帰)
  void buildStage() noexcept {
    if (!canBuild()) {
      finished_build_ = true;
      return;
    }
    finished_build_ = false;
    
    event_timeline_->add([this]() noexcept {
        buildOneLine();

        {
          // active_top_z_には次のzが入っている
          EventParam params = {
            { "active_top_z", active_top_z_ - 1 }
          };
          event_.signal("build-one-line", params);
        }

        // active_top_z_には次のzが入っている
        if ((active_top_z_ - 1) == finish_line_z_) {
          event_.signal("build-finish-line", EventParam());
        }

        // 生成演出
        auto& cubes = topLine();
        for (auto& cube : cubes) {
          cube.can_ride = false;
          
          float y = ci::randFloat(build_y_.x, build_y_.y);
          ci::Vec3f start_value(cube.position() + ci::Vec3f(0, y, 0));
          ci::Vec3f end_value = cube.position();
          auto options = animation_timeline_->apply(&cube.position,
                                                    start_value, end_value,
                                                    build_duration_, getEaseFunc(build_ease_));

          // lambda内でcubeを書き換えるので、mutable指定
          options.finishFn([&cube]() mutable {
              cube.can_ride = true;
            });

          cube.position = start_value;
        }
        
        buildStage();
      },
      event_timeline_->getCurrentTime() + build_speed_ * build_speed_rate_());
  }

  // 崩壊(再帰)
  void collapseStage(const int stop_z) noexcept {
    int bottom_z = active_top_z_ - int(active_cubes_.size()) - 1;
    
    if (!canCollapse() || (bottom_z == stop_z)) {
      finished_collapse_ = true;
      return;
    }
    started_collapse_  = true;
    finished_collapse_ = false;

    // 落下開始
    collapseStartOneLine();
    event_.signal("collapse-one-line", EventParam());
    auto& cubes = collapseLine();
    for (auto& cube : cubes) {
      cube.can_ride = false;

      float y = ci::randFloat(collapse_y_.x, collapse_y_.y);
      ci::Vec3f end_value(cube.position() + ci::Vec3f(0, y, 0));
      animation_timeline_->apply(&cube.position, end_value,
                                 collapse_duration_, getEaseFunc(collapse_ease_));
    }

    animation_timeline_->add([this]() noexcept {
        collapseFinishOneLine();
      },
      animation_timeline_->getCurrentTime() + collapse_duration_);

    event_timeline_->add(std::bind(&Stage::collapseStage, this, stop_z),
                          event_timeline_->getCurrentTime() + collapse_speed_ * collapse_speed_rate_);
  }
  
  const StageCube* const getStageCube(const ci::Vec3i& block_pos) const noexcept {
    int top_z    = active_top_z_ - 1;
    int bottom_z = active_top_z_ - int(active_cubes_.size());

    if ((block_pos.z < bottom_z) || (block_pos.z > top_z)) return nullptr;
    
    int iz = block_pos.z - bottom_z;
    for (auto& cube : active_cubes_[iz]) {
      if (cube.block_position.x == block_pos.x) {
        return &cube;
      }
    }
    return nullptr;
  }

  StageCube* const getStageCube(const ci::Vec3i& block_pos) noexcept {
    // TIPS:constなし版のための策
    return const_cast<StageCube*>(static_cast<const Stage*>(this)->getStageCube(block_pos));
  }
  

  
  bool canBuild() const {
    return !cubes_.empty();
  }

  bool canCollapse() const {
    return !active_cubes_.empty();
  }
  
  void buildOneLine() {
    auto row = cubes_.front();
    cubes_.pop_front();
    active_cubes_.push_back(std::move(row));
    active_top_z_ += 1;
  }

  void collapseStartOneLine() {
    collapse_cubes_.push_back(active_cubes_.front());
    active_cubes_.pop_front();
  }

  void collapseFinishOneLine() {
    collapse_cubes_.pop_front();
  }

  std::vector<StageCube>& topLine() {
    return active_cubes_.back();
  }
  
  std::vector<StageCube>& bottomLine() {
    return active_cubes_.front();
  }

  std::vector<StageCube>& collapseLine() {
    return collapse_cubes_.back();
  }


  void moveStageCube(StageCube& cube) {
    cube.block_position_new.y -= 1;
    
    auto end_value = ci::Vec3f(cube.block_position_new);
    auto option = animation_timeline_->appendTo(&cube.position, end_value,
                                                move_duration_, getEaseFunc(move_ease_));

    option.delay(move_delay_);

    ci::Vec3f block_position = cube.block_position;
    option.finishFn([this, block_position]() noexcept {
        auto* cube = getStageCube(block_position);
        if (cube) {
          cube->block_position.y -= 1;
          DOUT << cube->block_position << std::endl;
        }
      });
  }
  
};

}
