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


  // std::findを利用するための定義
  bool operator== (const Touch& rhs) const noexcept {
    return id == rhs.id;
  }

  bool operator== (const u_int rhs_id) const noexcept {
    return id == rhs_id;
  }
  
};

}
