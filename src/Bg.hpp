#pragma once

//
// イケてる背景(主観)
//

#include <boost/noncopyable.hpp>


namespace ngs {

class Bg : private boost::noncopyable {
public:
  struct Cube {
    ci::Vec3f position;
    ci::Vec3f size;
    ci::Color color;

    ci::Vec3f speed;
  };


private:
  Event<EventParam>& event_;

  ci::TimelineRef animation_timeline_;

  float cube_size_;

  std::vector<Cube> cubes_;

  ci::Vec3f bbox_min_orig_;
  ci::Vec3f bbox_max_orig_;

  ci::Vec3f bbox_min_;
  ci::Vec3f bbox_max_;
  
  
public:
  Bg(const ci::JsonTree& params,
     ci::TimelineRef timeline,
     Event<EventParam>& event) :
    event_(event),
    animation_timeline_(ci::Timeline::create()),
    cube_size_(params["game.cube_size"].getValue<float>()),
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
      cube.position += cube.speed * progressing_seconds;

      // FIXME:コピペ甚だしい
      if (cube.position.x > bbox_max_.x) {
        cube.position.x = bbox_min_.x + (cube.position.x - bbox_max_.x);
      }
      else if (cube.position.x < bbox_min_.x) {
        cube.position.x = bbox_max_.x + (cube.position.x - bbox_min_.x);
      }
      
      if (cube.position.y > bbox_max_.y) {
        cube.position.y = bbox_min_.y + (cube.position.y - bbox_max_.y);
      }
      else if (cube.position.y < bbox_min_.y) {
        cube.position.y = bbox_max_.y + (cube.position.y - bbox_min_.y);
      }
      
      if (cube.position.z > bbox_max_.z) {
        cube.position.z = bbox_min_.z + (cube.position.z - bbox_max_.z);
      }
      else if (cube.position.z < bbox_min_.z) {
        cube.position.z = bbox_max_.z + (cube.position.z - bbox_min_.z);
      }
    }
  }


  const std::vector<Cube>& cubes() const { return cubes_; }

  std::pair<ci::Vec3f, ci::Vec3f> getBbox() const {
    return std::make_pair(bbox_min_, bbox_max_);
  }
  

private:

  

  
};

}
