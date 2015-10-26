#pragma once

//
// 達成項目のキャッシュ
//

#include <map>
#include "GameCenter.h"


namespace ngs {

class Achievement {
  struct Item {
    bool send;
    double value;
  };
  std::map<std::string, Item> items_;


public:
  Achievement() = default;

  
  void entry(const std::string& id, const double value) noexcept {
    // TODO:キャッシュと値が同じ場合は送信しない
    GameCenter::submitAchievement(id, value,
                                  [this, id, value]() {
                                    Item item = { false, value };
                                    items_[id] = item;
                                  },
                                  [this, id, value]() {
                                    Item item = { true, value };
                                    items_[id] = item;
                                  });
  }

  
  void load(const std::string& path) noexcept {
    auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(full_path)) return;

#if defined(OBFUSCATION_ACHIEVEMENT)
    ci::JsonTree achievements(TextCodec::load(full_path.string()));
#else
    ci::JsonTree achievements = ci::JsonTree(ci::loadFile(full_path));
#endif

    for (u_int i = 0; i < achievements.getNumChildren(); ++i) {
      auto& json = achievements[i];
      
      std::string id = json["id"].getValue<std::string>();

      Item item = {
        json["send"].getValue<bool>(),
        json["value"].getValue<double>(),
      };
      
      items_[id] = item;
    }

    DOUT << "Achievement: load success." << std::endl;
  }

  void write(const std::string& path) noexcept {
    // キャッシュが空の場合は何もしない
    if (items_.empty()) return;
    
    ci::JsonTree achievements = ci::JsonTree::makeArray("achievements");
    for (const auto& item : items_) {
      ci::JsonTree achievement;
      achievement.addChild(ci::JsonTree("id", item.first))
        .addChild(ci::JsonTree("send", item.second.send))
        .addChild(ci::JsonTree("value", item.second.value));

      achievements.pushBack(achievement);
    }
    
    auto full_path = getDocumentPath() / path;
#if defined(OBFUSCATION_ACHIEVEMENT)
    TextCodec::write(full_path.string(), achievements.serialize());
#else
    achievements.write(full_path);
#endif
    
    DOUT << "Achievement: write success." << std::endl;
  }

  
#ifdef DEBUG
  void reset() noexcept {
    // キャッシュは項目の内容を初期化して、即座に書き出す
    for (auto& item : items_) {
      item.second.send  = true;
      item.second.value = 0.0;
    }
    
    GameCenter::resetAchievement();
    
    DOUT << "Achievement::reset" << std::endl;
  }
#endif
  
};

}
