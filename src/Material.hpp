#pragma once

//
// モデルのマテリアル定義
// TODO:copy禁止
//

#include <boost/noncopyable.hpp>
#include "cinder/gl/Material.h"
#include "JsonUtil.hpp"


namespace ngs {

class Material : private boost::noncopyable {
  ci::gl::Material material_;


public:
  Material(const ci::JsonTree& params) :
    material_(Json::getColorA<float>(params["ambient"]),
              Json::getColorA<float>(params["diffuse"]),
              Json::getColorA<float>(params["specular"]),
              params["shininess"].getValue<float>(),
              Json::getColorA<float>(params["emission"]))
  { }

  
  void apply() const { material_.apply(); }

  
  void setAmbient(const ci::ColorA& color) {
    material_.setAmbient(color);
  }

  void setDiffuse(const ci::ColorA& color) {
    material_.setDiffuse(color);
  }

  void setSpecular(const ci::ColorA& color) {
    material_.setSpecular(color);
  }
 
  void setShininess(const float shininess) {
    material_.setShininess(shininess);
  }

  void setEmission(const ci::ColorA& color) {
    material_.setEmission(color);
  }

  
private:

  
};

}
