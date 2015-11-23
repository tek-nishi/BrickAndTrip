#pragma once

//
// モデルのマテリアル定義
//

#include <boost/noncopyable.hpp>
#include "cinder/gl/Material.h"
#include "JsonUtil.hpp"


namespace ngs {

class Material : private boost::noncopyable {
  ci::gl::Material material_;


public:
  Material(const ci::JsonTree& params) noexcept :
    material_(Json::getColorA<float>(params["ambient"]),
              Json::getColorA<float>(params["diffuse"]),
              Json::getColorA<float>(params["specular"]),
              params["shininess"].getValue<float>(),
              Json::getColorA<float>(params["emission"]),
              GL_FRONT_AND_BACK)
  { }

  
  void apply() const noexcept { material_.apply(); }

  
  void setAmbient(const ci::ColorA& color) noexcept {
    material_.setAmbient(color);
  }

  void setDiffuse(const ci::ColorA& color) noexcept {
    material_.setDiffuse(color);
  }

  void setSpecular(const ci::ColorA& color) noexcept {
    material_.setSpecular(color);
  }
 
  void setShininess(const float shininess) noexcept {
    material_.setShininess(shininess);
  }

  void setEmission(const ci::ColorA& color) noexcept {
    material_.setEmission(color);
  }

  
private:

  
};

}
