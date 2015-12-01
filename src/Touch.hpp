#pragma once

//
// タッチ情報
//

namespace ngs {

struct Touch {
  // Mouse入力をタッチイベントとして扱うための識別ID
  enum { MOUSE_EVENT_ID = 0xffffffff };
  
  bool handled;
  
  double timestamp;
  
  u_int id;

  ci::Vec2f pos;
  ci::Vec2f prev_pos;

  Touch(const bool handled_, const double timestamp_, const u_int id_,
        const ci::Vec2f& pos_, const ci::Vec2f& prev_pos_) noexcept :
    handled(handled_),
    timestamp(timestamp_),
    id(id_),
    pos(pos_),
    prev_pos(prev_pos_)
  { }
  

  // std::findを利用するための定義
  bool operator== (const Touch& rhs) const noexcept {
    return id == rhs.id;
  }

  bool operator== (const u_int rhs_id) const noexcept {
    return id == rhs_id;
  }
  
};

}
