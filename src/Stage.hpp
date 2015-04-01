#pragma once

//
// 立方体で構成された移動場所
//

#include <vector>
#include <deque>
#include "StageCube.hpp"
#include "EasingUtil.hpp"


namespace ngs {

class Stage {
  Event<EventParam>& event_;
  
  // ステージ全体
  std::deque<std::vector<StageCube> > cubes_;

  // 現在表示中のCube
  std::deque<std::vector<StageCube> > active_cubes_;

  // 崩れ中のCube
  std::deque<std::vector<StageCube> > collapse_cubes_;

  float cube_size_;
  int top_z_;
  int active_top_z_;
  int finish_line_z_;
  
  float build_speed_;
  float collapse_speed_;

  std::string build_ease_;
  float       build_duration_;
  ci::Vec2f   build_y_;

  std::string collapse_ease_;
  float       collapse_duration_;
  ci::Vec2f   collapse_y_;

  std::string open_ease_;
  float       open_duration_;
  float       open_delay_;
  
  bool finished_build_;
  bool finished_collapse_;
  
  ci::TimelineRef event_timeline_;
  ci::TimelineRef animation_timeline_;
  

public:
  Stage(const ci::JsonTree& params,
        ci::TimelineRef timeline,
        Event<EventParam>& event) :
    event_(event),
    cube_size_(params["game.cube_size"].getValue<float>()),
    top_z_(0),
    active_top_z_(0),
    finish_line_z_(-1),
    build_speed_(params["game.stage.build_speed"].getValue<float>()),
    collapse_speed_(params["game.stage.collapse_speed"].getValue<float>()),
    build_ease_(params["game.stage.build_ease"].getValue<std::string>()),
    build_duration_(params["game.stage.build_duration"].getValue<float>()),
    build_y_(Json::getVec2<float>(params["game.stage.build_y"])),
    collapse_ease_(params["game.stage.collapse_ease"].getValue<std::string>()),
    collapse_duration_(params["game.stage.collapse_duration"].getValue<float>()),
    collapse_y_(Json::getVec2<float>(params["game.stage.collapse_y"])),
    open_ease_(params["game.stage.open_ease"].getValue<std::string>()),
    open_duration_(params["game.stage.open_duration"].getValue<float>()),
    open_delay_(params["game.stage.open_delay"].getValue<float>()),
    finished_build_(false),
    finished_collapse_(false),
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

  


  void buildStage(const float speed_rate = 1.0f) {
    if (!canBuild()) {
      finished_build_ = true;
      return;
    }
    finished_build_ = false;
    
    event_timeline_->add([this, speed_rate]() {
        buildOneLine();
        event_.signal("build-one-line", EventParam());

        // active_top_z_には次のzが入っている
        if ((active_top_z_ - 1) == finish_line_z_) {
          event_.signal("build-finish-line", EventParam());
        }

        // 生成演出
        auto& cubes = topLine();
        for (auto& cube : cubes) {
          cube.can_ride = false;
          
          float y = ci::randFloat(build_y_.x, build_y_.y) * cube_size_;
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
        
        buildStage(speed_rate);
      },
      event_timeline_->getCurrentTime() + build_speed_ * speed_rate);
  }

  void collapseStage(const int stop_z, const float speed_rate = 1.0f) {
    int bottom_z = active_top_z_ - active_cubes_.size() - 1;
    
    if (!canCollapse() || (bottom_z == stop_z)) {
      finished_collapse_ = true;
      return;
    }
    finished_collapse_ = false;

    // 落下開始
    collapseStartOneLine();
    event_.signal("collapse-one-line", EventParam());
    auto& cubes = collapseLine();
    for (auto& cube : cubes) {
      cube.can_ride = false;

      float y = ci::randFloat(collapse_y_.x, collapse_y_.y) * cube_size_;
      ci::Vec3f end_value(cube.position() + ci::Vec3f(0, y, 0));
      animation_timeline_->apply(&cube.position, end_value,
                                 collapse_duration_, getEaseFunc(collapse_ease_));
    }

    animation_timeline_->add([this]() {
        collapseFinishOneLine();
      },
      animation_timeline_->getCurrentTime() + collapse_duration_);

    event_timeline_->add(std::bind(&Stage::collapseStage, this, stop_z, speed_rate),
                          event_timeline_->getCurrentTime() + collapse_speed_ * speed_rate);
  }

  // 生成 & 崩壊を止める
  void stopBuildAndCollapse() {
    event_timeline_->clear();
  }

  // BuildとCollapseが完了しているか判定
  bool isFinishedBuildAndCollapse() {
    if (!finished_build_ || !finished_collapse_) return false;

    // Build演出の完了も調べる
    for (const auto& cube : topLine()) {
      if (!cube.position.isComplete()) return false;
    }
    
    return true;
  }

  // 崩壊完了()
  bool isFinishedCollapse() {
    return active_cubes_.empty();
  }

  void restart() {
    stopBuildAndCollapse();

    top_z_         = 0;
    active_top_z_  = 0;
    finish_line_z_ = -1;
    
    finished_build_    = false;
    finished_collapse_ = false;

    cubes_.clear();
  }
  

  // StartLineを下げる
  void openStartLine() {
    size_t iz = active_cubes_.size() - 1;

    for (auto& cube : active_cubes_[iz]) {
      ci::Vec3f end_value(cube.position() + ci::Vec3f(0, -cube_size_, 0));
      auto option = animation_timeline_->apply(&cube.position, end_value,
                                               open_duration_, getEaseFunc(open_ease_));

      option.delay(open_delay_);
    }

    event_timeline_->add([this, iz]() {
        // コンテナへの参照が無効になっている場合があるので、関数経由で取得
        for (auto& cube : active_cubes_[iz]) {
          cube.block_position.y -= 1;
        }
        event_.signal("startline-opened", EventParam());
      },
      event_timeline_->getCurrentTime() + open_delay_ + open_duration_);
  }

  void setFinishLine(const int z) {
    finish_line_z_ = z;
  }
  
  
  int addCubes(const ci::JsonTree& stage_data,
                const std::vector<ci::Color>& cube_color,
                const ci::Color& line_color) {
    build_speed_    = Json::getValue<float>(stage_data, "build_speed", build_speed_);
    collapse_speed_ = Json::getValue<float>(stage_data, "collapse_speed", collapse_speed_);
    
    const auto& body = stage_data["body"];
    int last_iz = body.getNumChildren() - 1;
    int iz = 0;
    for (const auto& row : body) {
      std::vector<StageCube> cube_row;
      int x = 0;
      int z = iz + top_z_;
      for (const auto& p : row) {
        int y = p.getValue<int>();
        ci::Vec3i block_pos(x, y, z);
        ci::Vec3f pos = ci::Vec3f(x, y, z) * cube_size_;

        auto color = cube_color[(x + z) & 1];
        if (iz == last_iz) {
          color *= line_color;
        }
        
        StageCube cube = {
          pos,
          ci::Quatf::identity(),
          cube_size_,
          block_pos,
          color,
          true, true
        };

        cube_row.push_back(std::move(cube));

        x += 1;
      }
      cubes_.push_back(std::move(cube_row));
      
      iz += 1;
    }

    top_z_ += body.getNumChildren();
    
    return top_z_; 
  }

  
  // 「この場所にはCubeが無い」も結果に含めるので、std::pairを利用
  std::pair<bool, int> getStageHeight(const ci::Vec3i& block_pos) {
    if (active_cubes_.empty()) {
      return std::make_pair(false, 0);
    }

    int top_z    = active_top_z_ - 1;
    int bottom_z = active_top_z_ - active_cubes_.size();
    
    if ((block_pos.z < bottom_z) || (block_pos.z > top_z)) {
      return std::make_pair(false, 0);
    }

    // TIPS:z値とコンテナの添え字が一致するようになっているので、
    //      コンテナからz値を検索する必要がない
    int iz = block_pos.z - bottom_z;
    for (const auto& cube : active_cubes_[iz]) {
      if (cube.block_position.x == block_pos.x) {
        return std::make_pair(cube.can_ride, cube.block_position.y);
      }
    }
    return std::make_pair(false, 0);
  }

  
  const std::deque<std::vector<StageCube> >& activeCubes() const {
    return active_cubes_;
  }

  const std::deque<std::vector<StageCube> >& collapseCubes() const {
    return collapse_cubes_;
  }


private:
  // TIPS:コピー不可
  Stage(const Stage&) = delete;
  Stage& operator=(const Stage&) = delete;


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
  
};

}
