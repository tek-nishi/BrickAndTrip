#pragma once

//
// イケてる背景(主観)
//

#include <boost/noncopyable.hpp>
#include "TweenUtil.hpp"


namespace ngs {

class Bg : private boost::noncopyable {
public:
  struct Cube {
    ci::Vec3f position;
    ci::Anim<ci::Vec3f> size;
    ci::Color color;

    ci::Vec3f speed;
    ci::Vec3f revised_pos;
    bool is_tween;
  };


private:
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  ci::TimelineRef animation_timeline_;

  float cube_size_;
  float revise_duration_;
  
  std::vector<Cube> cubes_;

  ci::Vec3f bbox_min_orig_;
  ci::Vec3f bbox_max_orig_;

  ci::Vec3f bbox_min_;
  ci::Vec3f bbox_max_;
  
  
public:
  Bg(ci::JsonTree& params,
     ci::TimelineRef timeline,
     Event<EventParam>& event) :
    params_(params),
    event_(event),
    animation_timeline_(ci::Timeline::create()),
    cube_size_(params["game.cube_size"].getValue<float>()),
    revise_duration_(params["game.bg.revise_duration"].getValue<float>()),
    bbox_min_orig_(Json::getVec3<float>(params["game.bg.bbox_min"]) * cube_size_),
    bbox_max_orig_(Json::getVec3<float>(params["game.bg.bbox_max"]) * cube_size_),
    bbox_min_(bbox_min_orig_),
    bbox_max_(bbox_max_orig_)
  {
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    auto cube_speed = Json::getVec2<float>(params["game.bg.cube_speed"]);
    auto cube_density = params["game.bg.cube_density"].getValue<float>();
    int max_y = bbox_max_.y;
    for (int iy = bbox_min_.y; iy < max_y; ++iy) {
      if (ci::randInt(100) < 50) {
        // X方向
        int max_x = bbox_max_.x;
        for (int ix = bbox_min_.x; ix < max_x; ++ix) {
          if (ci::randFloat() > cube_density) continue;

          float speed = ci::randFloat(cube_speed.x, cube_speed.y);
          if (ci::randInt(100) < 50) speed = -speed;
          
          Cube cube = {
            ci::Vec3f(ix, iy, ci::randInt(bbox_min_.z, bbox_max_.z)),
            ci::Vec3f(cube_size_, cube_size_, cube_size_),
            ci::hsvToRGB(ci::Vec3f(0, 0, ci::randFloat(0, 1))),
            ci::Vec3f(0, 0, speed),
            ci::Vec3f::zero(),
            false,
          };
          cubes_.push_back(cube);
        }
      }
      else {
        // Z方向
        int max_z = bbox_max_.z;
        for (int iz = bbox_min_.z; iz < max_z; ++iz) {
          if (ci::randFloat() > cube_density) continue;

          float speed = ci::randFloat(cube_speed.x, cube_speed.y);
          if (ci::randInt(100) < 50) speed = -speed;

          Cube cube = {
            ci::Vec3f(ci::randInt(bbox_min_.x, bbox_max_.x), iy, iz),
            ci::Vec3f(cube_size_, cube_size_, cube_size_),
            ci::hsvToRGB(ci::Vec3f(0, 0, ci::randFloat(0, 1))),
            ci::Vec3f(speed, 0, 0),
            ci::Vec3f::zero(),
            false,
          };
          cubes_.push_back(cube);
        }
      }
    }

    DOUT << "bg num:" << cubes_.size() << std::endl;
  }

  ~Bg() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }
  

  void setCenterPosition(const ci::Vec3i& pos) {
    
    bbox_min_ = bbox_min_orig_ + ci::Vec3f(pos) * cube_size_;
    bbox_max_ = bbox_max_orig_ + ci::Vec3f(pos) * cube_size_;
  }


  void update(const double progressing_seconds) {
    for (auto& cube : cubes_) {
      if (cube.is_tween) continue;

      bool revised = false;
      cube.position += cube.speed * progressing_seconds;

      // FIXME:コピペ甚だしい
      auto pos = cube.position;
      if (pos.x > bbox_max_.x) {
        pos.x = bbox_min_.x + (pos.x - bbox_max_.x);
        revised = true;
      }
      else if (pos.x < bbox_min_.x) {
        pos.x = bbox_max_.x + (pos.x - bbox_min_.x);
        revised = true;
      }
      
      if (pos.y > bbox_max_.y) {
        pos.y = bbox_min_.y + (pos.y - bbox_max_.y);
        revised = true;
      }
      else if (pos.y < bbox_min_.y) {
        pos.y = bbox_max_.y + (pos.y - bbox_min_.y);
        revised = true;
      }
      
      if (pos.z > bbox_max_.z) {
        pos.z = bbox_min_.z + (pos.z - bbox_max_.z);
        revised = true;
      }
      else if (pos.z < bbox_min_.z) {
        pos.z = bbox_max_.z + (pos.z - bbox_min_.z);
        revised = true;
      }

      if (revised) {
        cube.revised_pos = pos;
        startTween("out_box", cube);
        cube.is_tween = true;

        animation_timeline_->add([this, &cube]() mutable {
            cube.position = cube.revised_pos;
            startTween("in_box", cube);
          },
          animation_timeline_->getCurrentTime() + revise_duration_);

        animation_timeline_->add([&cube]() mutable {
            cube.is_tween = false;
          },
          animation_timeline_->getCurrentTime() + revise_duration_ * 2);
      }
    }
  }


  const std::vector<Cube>& cubes() const { return cubes_; }

  std::pair<ci::Vec3f, ci::Vec3f> getBbox() const {
    return std::make_pair(bbox_min_, bbox_max_);
  }
  

private:
  void startTween(const std::string& name, Cube& cube) {
    auto tween_params = params_["game.bg.tween." + name];

    std::set<std::string> applyed_targets;
    for (const auto& params : tween_params) {
      std::map<std::string,
               std::function<void (Cube&, const ci::JsonTree&, const bool)> > tween_setup = {
        { "scale",
          [this](Cube& cube, const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, cube.size, params, ci::Vec3f::zero(), cube_size_, is_first);
          }
        },
      };

      const auto& target = params["target"].getValue<std::string>();
      tween_setup[target](cube, params, isFirstApply(target, applyed_targets));
    }
  }


  static bool isFirstApply(const std::string& type, std::set<std::string>& apply) {
    auto result = apply.insert(type);
    return result.second;
  }
  

  
};

}
