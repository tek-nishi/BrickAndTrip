#pragma once

//
// EventのConnectionを安全に切断するUtility
//

#include <vector>
#include "Event.hpp"
#include <boost/noncopyable.hpp>


namespace ngs {

class ConnectionHolder : private boost::noncopyable {
  std::vector<Connection> connections_;


public:
  ConnectionHolder() = default;

  ~ConnectionHolder() {
    for (auto& connection : connections_) {
      connection.disconnect();
    }
  }


  void clear() {
    for (auto& connection : connections_) {
      connection.disconnect();
    }
    
    connections_.clear();
  }

  
  void operator += (Connection& connection) {
    connections_.push_back(connection);
  }

  void operator += (Connection&& connection) {
    connections_.push_back(connection);
  }
  
};

}
