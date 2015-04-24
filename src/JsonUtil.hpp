#pragma once

//
// Json utility
// 配列から色々生成する
//

#include "cinder/Vector.h"
#include <sstream>


namespace ngs {
namespace Json {

template<typename T>
std::vector<T> getArray(const ci::JsonTree& json) {
  size_t num = json.getNumChildren();

  std::vector<T> array(num);
  for (size_t i = 0; i < num; ++i) {
    array[i] = json[i].getValue<T>();
  }

  return array;
}


template<typename T>
ci::Vec2<T> getVec2(const ci::JsonTree& json) {
  return ci::Vec2<T>(json[0].getValue<T>(), json[1].getValue<T>());
}

template<typename T>
ci::Vec3<T> getVec3(const ci::JsonTree& json) {
  return ci::Vec3<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>());
}

template<typename T>
ci::Vec4<T> getVec4(const ci::JsonTree& json) {
  return ci::Vec4<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>(), json[3].getValue<T>());
}

template<typename T>
ci::Quaternion<T> getQuaternion(const ci::JsonTree& json) {
  return ci::Quaternion<T>(getVec3<T>(json[0]), ci::toRadians(json[1].getValue<T>()));
}

template<typename T>
ci::ColorT<T> getColor(const ci::JsonTree& json) {
  return ci::ColorT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>());
}

ci::Vec3f getHsvColor(const ci::JsonTree& json) {
  return ci::Vec3f(json[0].getValue<float>() / 360.0f, json[1].getValue<float>(), json[2].getValue<float>());
}

template<typename T>
ci::ColorAT<T> getColorA(const ci::JsonTree& json) {
  return ci::ColorAT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>(), json[3].getValue<T>());
}


template<typename T>
T getValue(const ci::JsonTree& json, const std::string& name, const T& default_value) {
  return (json.hasChild(name)) ? json[name].getValue<T>()
                               : default_value;
}


ci::JsonTree readParams(const std::string& path) {
#if defined (DEBUG) && defined (CINDER_MAC)
  // DEBUG時、OSXはプロジェクトの場所からfileを読み込む
  std::ostringstream full_path;
  full_path << PREPRO_TO_STR(SRCROOT) << "../assets/" << path;
  return ci::JsonTree(ci::DataSourcePath::create(full_path.str()));
#else
  return ci::JsonTree(ci::app::loadAsset(path));
#endif
}

}
}
