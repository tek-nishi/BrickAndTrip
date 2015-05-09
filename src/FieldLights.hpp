#pragma once

//
// Fieldの光源
//

#include "cinder/gl/Light.h"


namespace ngs {

class FieldLights {
  struct Light {
    int type;
    ci::gl::Light l;

    const ci::JsonTree* const tween_params;

    ci::Anim<ci::Vec3f> vector;

    ci::Anim<float> constant_attenuation;
    ci::Anim<float> linear_attenuation;
    ci::Anim<float> quadratic_attenuation;

    ci::Anim<ci::Color> diffuse;
    ci::Anim<ci::Color> ambient;
    ci::Anim<ci::Color> specular;
  };
  
  std::vector<Light> lights_;

  ci::TimelineRef animation_timeline_;


public:
  FieldLights(ci::JsonTree& params,
              ci::TimelineRef timeline) :
    animation_timeline_(ci::Timeline::create())
  {
    auto current_time = timeline->getCurrentTime();
    animation_timeline_->setStartTime(current_time);
    timeline->apply(animation_timeline_);
    
    int id = 0;

    static std::map<std::string, int> light_type = {
      { "point",       ci::gl::Light::POINT },
      { "directional", ci::gl::Light::DIRECTIONAL },
    };
    
    for (const auto& param : params["game_view.lights"]) {
      const auto& type = light_type.at(param["type"].getValue<std::string>());
      const ci::JsonTree* const tween_params = param.hasChild("tween") ? &param["tween"] : nullptr;
      
      Light light = {
        type,
        { type, id },
        tween_params,
        Json::getVec3<float>(param["pos"]),
      };

      switch (type) {
      case ci::gl::Light::POINT:
        {
          light.l.setPosition(light.vector);

          light.constant_attenuation  = param["constant_attenuation"].getValue<float>();
          light.linear_attenuation    = param["linear_attenuation"].getValue<float>();
          light.quadratic_attenuation = param["quadratic_attenuation"].getValue<float>();

          light.l.setAttenuation(light.constant_attenuation(),
                                 light.linear_attenuation(),
                                 light.quadratic_attenuation());
        }
        break;

      case ci::gl::Light::DIRECTIONAL:
        {
          light.l.setDirection(light.vector);
        }
        break;
      }

      light.diffuse  = Json::getColor<float>(param["diffuse"]);
      light.ambient  = Json::getColor<float>(param["ambient"]);
      light.specular = Json::getColor<float>(param["specular"]);
      
      light.l.setDiffuse(light.diffuse());
      light.l.setAmbient(light.ambient());
      light.l.setSpecular(light.specular());

      lights_.push_back(std::move(light));
      
      ++id;
    }
  }

  ~FieldLights() {
    // 再生途中のものもあるので、手動で取り除く
    animation_timeline_->removeSelf();
  }


  void enableLights() {
    for (auto& light : lights_) {
      light.l.enable();
    }
  }

  void disableLights() {
    for (auto& light : lights_) {
      light.l.disable();
    }
  }

  
  void updateLights(const ci::Vec3f& target_position) {
    for (auto& light : lights_) {
      switch (light.type) {
      case ci::gl::Light::POINT:
        {
          ci::Vec3f pos = light.vector;

          // 注視点のzだけ拝借
          pos.z += target_position.z;
          light.l.setPosition(pos);
        }
        break;
      }
    }
  }

  void startLightTween(const std::string& tween_name) {
    for (auto& light : lights_) {
      if (!light.tween_params) continue;
      if (!light.tween_params->hasChild(tween_name)) continue;

      startTween(light, light.tween_params->getChild(tween_name));
    }
  }

  
private:
  void startTween(Light& light, const ci::JsonTree& tween_params) {
    std::set<std::string> applyed_targets;
    for (const auto& params : tween_params) {
      std::map<std::string,
               std::function<void (Light&, const ci::JsonTree&, const bool)> > tween_setup = {
        { "position",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, light.vector, params, is_first);
          }
        },
        { "direction",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setVec3Tween(*animation_timeline_, light.vector, params, is_first);
          }
        },
        
        { "constant_attenuation",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setFloatTween(*animation_timeline_, light.constant_attenuation, params, is_first);
          }
        },
        { "linear_attenuation",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setFloatTween(*animation_timeline_, light.linear_attenuation, params, is_first);
          }
        },
        { "quadratic_attenuation",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setFloatTween(*animation_timeline_, light.quadratic_attenuation, params, is_first);
          }
        },

        { "diffuse",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setColorTween(*animation_timeline_, light.diffuse, params, is_first);
          }
        },
        { "ambient",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setColorTween(*animation_timeline_, light.ambient, params, is_first);
          }
        },
        { "specular",
          [this](Light& light, const ci::JsonTree& params, const bool is_first) {
            setColorTween(*animation_timeline_, light.specular, params, is_first);
          }
        },
      };
      
      const auto& target = params["target"].getValue<std::string>();
      tween_setup[target](light, params, isFirstApply(target, applyed_targets));
    }
  }  

  static bool isFirstApply(const std::string& type, std::set<std::string>& apply) {
    auto result = apply.insert(type);
    return result.second;
  }

};

}
