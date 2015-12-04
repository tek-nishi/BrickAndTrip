#pragma once

//
// BGM処理
//

#include <boost/noncopyable.hpp>
#include <cinder/audio/Context.h>
#include <cinder/audio/SamplePlayerNode.h>


namespace ngs {

class Sound : private boost::noncopyable {
  // 各音源情報
  struct Object : private boost::noncopyable {
    std::string type;
    std::string category;
    
    std::vector<std::string> poly_category;
    size_t category_index;

    bool loop;

    // TIPS:std::map::emplaceを使って構築したいので
    //      コンストラクタを定義している
    Object(std::string type_,
           std::string category_,
           std::vector<std::string> poly_category_,
           const size_t category_index_,
           const bool loop_) noexcept :
      type(std::move(type_)),
      category(std::move(category_)),
      poly_category(std::move(poly_category_)),
      category_index(category_index_),
      loop(loop_)
    {}
  };

  std::map<std::string, Object> objects_;

  
  // 効果音用の定義
  std::map<std::string, ci::audio::BufferRef> buffer_;
  std::map<std::string, ci::audio::BufferPlayerNodeRef> buffer_node_;

  // ストリーミング再生用の定義
  std::map<std::string, ci::audio::SourceFileRef> source_;
  std::map<std::string, ci::audio::FilePlayerNodeRef> file_node_;

  
  // 停止用
  std::map<std::string, ci::audio::SamplePlayerNodeRef> category_node_;

  bool buffer_silent_;
  bool file_silent_;
  
  
public:
  Sound(const ci::JsonTree& params) noexcept :
    buffer_silent_(false),
    file_silent_(false)
  {
    auto* ctx = ci::audio::Context::master();
    ctx->getOutput()->enableClipDetection(false);
    ctx->enable();
    
    // TIPS:文字列による処理の分岐をstd::mapとラムダ式で実装
    std::map<std::string,
             std::function<void (ci::audio::Context*, const ci::JsonTree&)> > creator = {
      { "file", 
        [this](ci::audio::Context* ctx, const ci::JsonTree& param) {
          auto path = param["path"].getValue<std::string>();
          auto source = ci::audio::load(ci::app::loadAsset(path));
          DOUT << "source:" << path << " ch:" << source->getNumChannels() << std::endl;

          // TIPS:初期値より増やしておかないと、処理負荷で音が切れる
          source->setMaxFramesPerRead(8192);
          
          source_.insert({ param["name"].getValue<std::string>(), source });

          const auto& category = param["category"].getValue<std::string>();
          if (!file_node_.count(category)) {
            // TIPS:SPECIFIEDにしないと、STEREOの音源を直接MONO出力できない
            ci::audio::Node::Format format;
            format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

            auto node = ctx->makeNode(new ci::audio::FilePlayerNode(format));
            file_node_.insert({ category, node });
            category_node_.insert({ category, node });
          }
        }
      },

      { "buffer",
        [this](ci::audio::Context* ctx, const ci::JsonTree& param) {
          auto path = param["path"].getValue<std::string>();
          auto source = ci::audio::load(ci::app::loadAsset(path));
          DOUT << "source:" << path << " ch:" << source->getNumChannels() << std::endl;

          buffer_.insert({ param["name"].getValue<std::string>(), source->loadBuffer() });

          if (param.hasChild("poly-category")) {
            auto categories = Json::getArray<std::string>(param["poly-category"]);
            for (const auto& category : categories) {
              makeBufferNode(ctx, category);
            }
          }
          else {
            const auto& category = param["category"].getValue<std::string>();
            makeBufferNode(ctx, category);
          }
        }
      }
    };

    for (const auto& it : params) {
      std::string category;
      std::vector<std::string> poly_category;
      
      if (it.hasChild("poly-category")) {
        poly_category = Json::getArray<std::string>(it["poly-category"]);
      }
      else {
        category = it["category"].getValue<std::string>();
      }

      const auto& name = it["name"].getValue<std::string>();
      const auto& type = it["type"].getValue<std::string>();
      objects_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(name),
                       std::forward_as_tuple(type,
                                             category,
                                             poly_category,
                                             0,
                                             it["loop"].getValue<bool>()));

      creator[type](ctx, it);
    }
  }

  ~Sound() {
    auto* ctx = ci::audio::Context::master();
    ctx->disable();
    ctx->disconnectAllNodes();
  }
  

  void play(const std::string& name) noexcept {
    disconnectInactiveNode();
    
    // TIPS:文字列による分岐をstd::mapとラムダ式で実装
    std::map<std::string,
             std::function<void (const std::string&, Object&)> > assign = {
      { "file",
        [this](const std::string& name, Object& object) {
          if (file_silent_) return;
          
          auto& source = source_.at(name);
          auto& node   = file_node_.at(object.category);
          
          if (node->isEnabled()) {
            node->stop();
          }
          auto* ctx = ci::audio::Context::master();

          node->setSourceFile(source);
          node->setLoopEnabled(object.loop);

          node >> ctx->getOutput();
          node->start();
        }
      },

      { "buffer",
        [this](const std::string& name, Object& object) {
          if (buffer_silent_) return;

          auto& buffer = buffer_.at(name);
          auto& node   = buffer_node_.at(getCategory(object));

          if (node->isEnabled()) {
            node->stop();
          }
          
          auto* ctx = ci::audio::Context::master();

          node->setBuffer(buffer);
          node->setLoopEnabled(object.loop);
          
          node >> ctx->getOutput();
          node->start();
        }
      }
    };

    auto& object = objects_.at(name);
    assign[object.type](name, object);
  }

  void stop(const std::string& category) noexcept {
    if (category_node_.count(category)) {
      auto node = category_node_.at(category);
      if (node->isEnabled()) {
        node->stop();
      }
    }
    
    disconnectInactiveNode();
  }

  void stopAll() noexcept {
    for (auto& it : category_node_) {
      if (it.second->isEnabled()) {
        it.second->stop();
      }
    }

    disconnectInactiveNode();
  }

  
  void setBufferSilent(const bool value) noexcept {
    buffer_silent_ = value;

    // 無音モードになった瞬間から音を止める
    if (value) {
      for (auto& it : buffer_node_) {
        if (it.second->isEnabled()) {
          it.second->stop();
        }
      }

      disconnectInactiveNode();
    }
  }

  void setFileSilent(const bool value) noexcept {
    file_silent_ = value;

    // 無音モードになった瞬間から音を止める
    if (value) {
      for (auto& it : file_node_) {
        if (it.second->isEnabled()) {
          it.second->stop();
        }
      }

      disconnectInactiveNode();
    }
  }
  

private:
  void makeBufferNode(ci::audio::Context* ctx, const std::string& category) noexcept {
    if (!buffer_node_.count(category)) {
      // TIPS:SPECIFIEDにしないと、STEREOの音源を直接MONO出力できない
      ci::audio::Node::Format format;
      format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

      auto node = ctx->makeNode(new ci::audio::BufferPlayerNode(format));
      buffer_node_.insert({ category, node });
      category_node_.insert({ category, node });
    }
  }

  
  const std::string& getCategory(Object& object) noexcept {
    if (object.poly_category.empty()) {
      return object.category;
    }
    else {
      object.category_index = (object.category_index + 1) % object.poly_category.size();
      return object.poly_category[object.category_index];
    }
  }
  
  // FIXME:iOSではNodeをOutputにたくさん繋げると、音量が小さくなる
  void disconnectInactiveNode() noexcept {
    auto output = ci::audio::Context::master()->getOutput();
    // DOUT << "active nodes:" << output->getNumConnectedInputs() << std::endl;

    std::set<ci::audio::NodeRef> nodes = output->getInputs();
    for (auto node : nodes) {
      if (!node->isEnabled()) node->disconnect(output);
    }
  }

};

}
