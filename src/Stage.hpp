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
  // ステージ全体
  std::deque<std::vector<StageCube> > cubes_;

  // 現在表示中のCube
  std::deque<std::vector<StageCube> > active_cubes_;

  // 崩れ中のCube
  std::deque<std::vector<StageCube> > collapse_cubes_;

  float cube_size_;
  int top_z_;

  float build_speed_;
  float collapse_speed_;

  std::string build_ease_;
  float       build_duration_;
  ci::Vec2f   build_y_;

  std::string collapse_ease_;
  float       collapse_duration_;
  ci::Vec2f   collapse_y_;

  bool finished_build_;
  bool finished_collapse_;
  
  ci::TimelineRef event_timeline_;
  ci::TimelineRef animation_timeline_;
  

public:
  Stage(const ci::JsonTree& params) :
    cube_size_(params["game.cube_size"].getValue<float>()),
    top_z_(0),
    build_speed_(params["game.stage.build_speed"].getValue<float>()),
    collapse_speed_(params["game.stage.collapse_speed"].getValue<float>()),
    build_ease_(params["game.stage.build_ease"].getValue<std::string>()),
    build_duration_(params["game.stage.build_duration"].getValue<float>()),
    build_y_(Json::getVec2<float>(params["game.stage.build_y"])),
    collapse_ease_(params["game.stage.collapse_ease"].getValue<std::string>()),
    collapse_duration_(params["game.stage.collapse_duration"].getValue<float>()),
    collapse_y_(Json::getVec2<float>(params["game.stage.collapse_y"])),
    finished_build_(false),
    finished_collapse_(false),
    event_timeline_(ci::Timeline::create()),
    animation_timeline_(ci::Timeline::create())
  {
    auto current_time = ci::app::timeline().getCurrentTime();
    event_timeline_->setStartTime(current_time);
    animation_timeline_->setStartTime(current_time);

    ci::app::timeline().apply(event_timeline_);
    ci::app::timeline().apply(animation_timeline_);
  }

  ~Stage() {
    // 再生途中のものもあるので、手動で取り除く
    event_timeline_->removeSelf();
    animation_timeline_->removeSelf();
  }

  
  bool canBuild() const {
    return !cubes_.empty();
  }

  bool canCollapse() const {
    return !active_cubes_.empty();
  }


  void buildStage(const float speed_rate = 1.0f) {
    if (!canBuild()) {
      finished_build_ = true;
      return;
    }
    
    event_timeline_->add([this, speed_rate]() {
        buildOneLine();

        // 落下演出
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

  void collapseStage(const float speed_rate = 1.0f) {
    if (!canCollapse()) {
      finished_collapse_ = true;
      return;
    }

    // 落下開始
    collapseStartOneLine();
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

    event_timeline_->add(std::bind(&Stage::collapseStage, this, speed_rate),
                          event_timeline_->getCurrentTime() + collapse_speed_ * speed_rate);
  }

  
  void addCubes(const ci::JsonTree& stage_data,
                const std::vector<ci::Color>& cube_color,
                const ci::Color& line_color) {
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
  }

  
  void buildOneLine() {
    auto row = cubes_.front();
    cubes_.pop_front();
    active_cubes_.push_back(std::move(row));
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


  // 「この場所にはCubeが無い」も結果に含めるので、std::pairを利用
  std::pair<bool, int> getStageHeight(const ci::Vec3i& block_pos) {
    if (active_cubes_.empty()) {
      return std::pair<bool, int>(false, 0);
    }

    int top_z    = top_z_ - 1;
    int bottom_z = top_z_ - active_cubes_.size();
    
    if ((block_pos.z < bottom_z) || (block_pos.z > top_z)) {
      return std::pair<bool, int>(false, 0);
    }

    // TIPS:z値とコンテナの添え字が一致するようになっているので、
    //      コンテナからz値を検索する必要がない
    int iz = block_pos.z - bottom_z;
    for (const auto& cube : active_cubes_[iz]) {
      if (cube.block_position.x == block_pos.x) {
        return std::pair<bool, int>(cube.can_ride, cube.block_position.y);
      }
    }
    return std::pair<bool, int>(false, 0);
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
  
};

}
