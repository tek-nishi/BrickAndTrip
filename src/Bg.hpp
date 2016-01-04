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

    Cube(const ci::Vec3f& position_,
         const ci::Anim<ci::Vec3f>& size_,
         const ci::Color& color_,
         const ci::Vec3f& speed_,
         const ci::Vec3f& revised_pos_,
         const bool is_tween_) noexcept :
      position(position_),
      size(size_),
      color(color_),
      speed(speed_),
      revised_pos(revised_pos_),
      is_tween(is_tween_)
    {}
  };


private:
  const ci::JsonTree& params_;
  Event<EventParam>& event_;

  ci::TimelineRef animation_timeline_;

  float revise_duration_;
  
  std::vector<Cube> cubes_;

  ci::Vec3f bbox_min_orig_;
  ci::Vec3f bbox_max_orig_;

  ci::Vec3f bbox_min_;
  ci::Vec3f bbox_max_;
  
  
public:
  Bg(ci::JsonTree& params,
     ci::TimelineRef timeline,
     Event<EventParam>& event) noexcept :
    params_(params),
    event_(event),
    animation_timeline_(ci::Timeline::create()),
    revise_duration_(params["game.bg.revise_duration"].getValue<float>()),
    bbox_min_orig_(Json::getVec3<float>(params["game.bg.bbox_min"])),
    bbox_max_orig_(Json::getVec3<float>(params["game.bg.bbox_max"])),
    bbox_min_(bbox_min_orig_),
    bbox_max_(bbox_max_orig_)
  {
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);

    auto cube_speed = Json::getVec2<float>(params["game.bg.cube_speed"]);
    auto cube_density = params["app.low_efficiency_device"].getValue<bool>() ? params["game.bg.cube_density_low"].getValue<float>()
                                                                             : params["game.bg.cube_density"].getValue<float>();

    
    auto color_range = Json::getVec2<float>(params["game.bg.color_range"]);

    int max_y = bbox_max_.y;
    for (int iy = bbox_min_.y; iy < max_y; ++iy) {
      if (ci::randInt(100) < 50) {
        // X方向
        int max_x = bbox_max_.x;
        for (int ix = bbox_min_.x; ix < max_x; ++ix) {
          if (ci::randFloat() > cube_density) continue;

          float speed = ci::randFloat(cube_speed.x, cube_speed.y);
          // 確率1/2で向きを逆に
          if (ci::randInt(100) < 50) speed = -speed;
          float v = ci::randFloat(color_range.x, color_range.y);

          cubes_.emplace_back(ci::Vec3f(ix, iy, ci::randInt(bbox_min_.z, bbox_max_.z)),
                              ci::Vec3f::one(),
                              ci::Color(v, v, v),
                              ci::Vec3f(0, 0, speed),
                              ci::Vec3f::zero(),
                              false);
        }
      }
      else {
        // Z方向
        int max_z = bbox_max_.z;
        for (int iz = bbox_min_.z; iz < max_z; ++iz) {
          if (ci::randFloat() > cube_density) continue;

          float speed = ci::randFloat(cube_speed.x, cube_speed.y);
          if (ci::randInt(100) < 50) speed = -speed;
          float v = ci::randFloat(color_range.x, color_range.y);

          cubes_.emplace_back(ci::Vec3f(ci::randInt(bbox_min_.x, bbox_max_.x), iy, iz),
                              ci::Vec3f::one(),
                              ci::Color(v, v, v),
                              ci::Vec3f(speed, 0, 0),
                              ci::Vec3f::zero(),
                              false);
        }
      }
    }

    DOUT << "bg num:" << cubes_.size() << std::endl;
  }

  ~Bg() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }
  

  void setCenterPosition(const ci::Vec3i& pos) noexcept {
    
    bbox_min_ = bbox_min_orig_ + ci::Vec3f(pos);
    bbox_max_ = bbox_max_orig_ + ci::Vec3f(pos);
  }

  void update(const double progressing_seconds) noexcept {
    for (auto& cube : cubes_) {
      if (cube.is_tween) continue;

      cube.position += cube.speed * progressing_seconds;
      bool revised = !checkInBbox(cube.position, bbox_min_, bbox_max_);
      if (revised) {
        cube.revised_pos.x = repeatValue(cube.position.x, bbox_min_.x, bbox_max_.x);
        cube.revised_pos.y = repeatValue(cube.position.y, bbox_min_.y, bbox_max_.y);
        cube.revised_pos.z = repeatValue(cube.position.z, bbox_min_.z, bbox_max_.z);
        
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


  const std::vector<Cube>& cubes() const noexcept { return cubes_; }

  std::pair<ci::Vec3f, ci::Vec3f> getBbox() const noexcept {
    return std::make_pair(bbox_min_, bbox_max_);
  }
  

private:
  void startTween(const std::string& name, Cube& cube) noexcept {
    auto tween_params = params_["game.bg.tween." + name];

    std::set<std::string> applyed_targets;
    for (const auto& params : tween_params) {
      std::map<std::string,
               std::function<void (Cube&, const ci::JsonTree&, const bool)> > tween_setup = {
        { "scale",
          [this](Cube& cube, const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, cube.size, params, ci::Vec3f::zero(), is_first);
          }
        },
      };

      const auto& target = params["target"].getValue<std::string>();
      tween_setup[target](cube, params, isFirstApply(target, applyed_targets));
    }
  }

  static bool checkInBbox(const ci::Vec3f& pos,
                          const ci::Vec3f& bbox_min, const ci::Vec3f& bbox_max) noexcept {
    return pos.x >= bbox_min.x
        && pos.x <= bbox_max.x
        && pos.y >= bbox_min.y
        && pos.y <= bbox_max.y
        && pos.z >= bbox_min.z
        && pos.z <= bbox_max.z;
  }
  
  static float repeatValue(const float value,
                           const float min_value, const float max_value) noexcept {
    float d = value - min_value;
    float between_value = max_value - min_value;
    return (d >= 0.0f) ? std::fmod(d, between_value) + min_value
                       : d + max_value;
  }
  
};

}
