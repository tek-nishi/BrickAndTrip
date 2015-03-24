#pragma once

//
// EventのConnectionを安全に切断するUtility
//

#include <vector>
#include "Event.hpp"


namespace ngs {

class ConnectionHolder {
  std::vector<Connection> connections_;


public:
  ConnectionHolder() = default;

  ~ConnectionHolder() {
    for (auto& connection : connections_) {
      connection.disconnect();
    }
  }

  
  void operator += (Connection& connection) {
    connections_.push_back(connection);
  }

  void operator += (Connection&& connection) {
    connections_.push_back(connection);
  }


private:
  // コピー禁止
  // 同じハンドルで何度もdisconnectしてしまうのを防ぐ
  ConnectionHolder(const ConnectionHolder&) = delete;
  ConnectionHolder& operator=(const ConnectionHolder&) = delete;
  
};

}
