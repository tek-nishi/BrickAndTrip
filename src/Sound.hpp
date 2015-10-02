#pragma once

//
// BGM処理
// TODO:fade in/out
// TODO:同じ音を複数同時再生
// TODO:Pan
//

#include "cinder/audio/Context.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/NodeEffects.h"
#include <boost/noncopyable.hpp>


namespace ngs {

class Sound : private boost::noncopyable {
  // 各音源情報
  struct Object {
    std::string type;
    std::string category;
    
    std::vector<std::string> poly_category;
    size_t category_index;

    bool loop;
    float gain;
  };

  std::map<std::string, Object> objects_;

  
  // 効果音用の定義
  struct BufferNode {
    ci::audio::BufferPlayerNodeRef node;
    ci::audio::GainNodeRef gain;
  };

  std::map<std::string, ci::audio::BufferRef> buffer_;
  std::map<std::string, BufferNode> buffer_node_;

  // ストリーミング再生用の定義
  struct FileNode {
    ci::audio::FilePlayerNodeRef node;
    ci::audio::GainNodeRef gain;
  };
  
  std::map<std::string, ci::audio::SourceFileRef> source_;
  std::map<std::string, FileNode> file_node_;

  
  // 停止用
  std::map<std::string, ci::audio::SamplePlayerNodeRef> category_node_;

  bool buffer_silent_;
  bool file_silent_;
  
  
public:
  Sound(const ci::JsonTree& params) :
    buffer_silent_(false),
    file_silent_(false)
  {
    auto* ctx = ci::audio::Context::master();

#if 0
    {
      // フレームあたりの処理数を増やす(標準は512)
      auto device = ci::audio::Device::getDefaultOutput();
      size_t frames_per_block = device->getFramesPerBlock();
      ci::audio::Device::Format format;
      format.framesPerBlock(frames_per_block * 2);
      device->updateFormat(format);
    }
#endif

    ctx->enable();
    
    // TIPS:文字列による処理の分岐をstd::mapとラムダ式で実装
    std::map<std::string,
             std::function<void (ci::audio::Context*, const ci::JsonTree&)> > creator = {
      { "file", 
        [this](ci::audio::Context* ctx, const ci::JsonTree& param) {
          auto path = param["path"].getValue<std::string>();
          auto source = ci::audio::load(ci::app::loadAsset(path));
          DOUT << "source:" << path << " ch:" << source->getNumChannels() << std::endl;
          
          source_.insert({ param["name"].getValue<std::string>(), source });

          const auto& category = param["category"].getValue<std::string>();
          if (file_node_.find(category) == file_node_.end()) {
            // TIPS:SPECIFIEDにしないと、STEREOの音源を直接MONO出力できない
            ci::audio::Node::Format format;
            format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);
            
            FileNode node = {
              ctx->makeNode(new ci::audio::FilePlayerNode(format)),
              ctx->makeNode(new ci::audio::GainNode(1.0f)),
            };
            
            file_node_.insert({ category, node });
            category_node_.insert({ category, node.node });
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
      
      Object object = {
        it["type"].getValue<std::string>(),
        category,
        poly_category,
        0,
        it["loop"].getValue<bool>(),
        it["gain"].getValue<float>()
      };

      const auto& name = it["name"].getValue<std::string>();
      objects_.insert({ name, object });

      creator[object.type](ctx, it);
    }
  }

  ~Sound() {
    auto* ctx = ci::audio::Context::master();
    ctx->disable();
    ctx->disconnectAllNodes();
  }
  

  void play(const std::string& name, const float gain = 1.0f) noexcept {
    disconnectInactiveNode();
    
    // TIPS:文字列による分岐をstd::mapとラムダ式で実装
    std::map<std::string,
             std::function<void (const std::string&, Object&, const float)> > assign = {
      { "file",
        [this](const std::string& name, Object& object, const float gain) {
          if (file_silent_) return;
          
          auto& source = source_.at(name);
          auto& node   = file_node_.at(object.category);
          
          if (node.node->isEnabled()) {
            node.node->stop();
          }
          auto* ctx = ci::audio::Context::master();
          // node.node->disconnect(ctx->getOutput());

          node.node->setSourceFile(source);
          node.node->setLoopEnabled(object.loop);
          // node.gain->setValue(object.gain * gain);

          node.node >> ctx->getOutput();
          node.node->start();
        }
      },

      { "buffer",
        [this](const std::string& name, Object& object, const float gain) {
          if (buffer_silent_) return;

          auto& buffer = buffer_.at(name);
          auto& node   = buffer_node_.at(getCategory(object));

          if (node.node->isEnabled()) {
            node.node->stop();
          }
          
          auto* ctx = ci::audio::Context::master();
          // node.node->disconnect(ctx->getOutput());

          node.node->setBuffer(buffer);
          node.node->setLoopEnabled(object.loop);
          // node.gain->setValue(object.gain * gain);
          
          node.node >> ctx->getOutput();
          node.node->start();
        }
      }
    };

    auto& object = objects_[name];
    assign[object.type](name, object, gain);
  }

  void stop(const std::string& category) noexcept {
    if (category_node_.find(category) != category_node_.end()) {
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
        if (it.second.node->isEnabled()) {
          it.second.node->stop();
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
        if (it.second.node->isEnabled()) {
          it.second.node->stop();
        }
      }

      disconnectInactiveNode();
    }
  }

private:
  void makeBufferNode(ci::audio::Context* ctx, const std::string& category) noexcept {
    if (buffer_node_.find(category) == buffer_node_.end()) {
      // TIPS:SPECIFIEDにしないと、STEREOの音源を直接MONO出力できない
      ci::audio::Node::Format format;
      format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

      BufferNode node = {
        ctx->makeNode(new ci::audio::BufferPlayerNode(format)),
        ctx->makeNode(new ci::audio::GainNode(1.0f)),
      };
            
      buffer_node_.insert({ category, node });
      category_node_.insert({ category, node.node });
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
    DOUT << "active nodes:" << output->getNumConnectedInputs() << std::endl;

    std::set<ci::audio::NodeRef> nodes = output->getInputs();
    for (auto node : nodes) {
      if (!node->isEnabled()) node->disconnect(output);
    }
  }

};

}
