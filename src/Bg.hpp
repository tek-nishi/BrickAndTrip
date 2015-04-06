#pragma once

//
// イケてる背景(主観)
//


namespace ngs {

class Bg {
  Event<EventParam>& event_;

  ci::TimelineRef animation_timeline_;

  float cube_size_;


public:
  Bg(const ci::JsonTree& params,
     ci::TimelineRef timeline,
     Event<EventParam>& event) :
    event_(event),
    animation_timeline_(ci::Timeline::create()),
    cube_size_(params["game.cube_size"].getValue<float>())
  {
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);
  }

  ~Bg() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }
  

private:

  

  
};

}
