﻿#pragma once

//
// Tweenセットアップ
//


namespace ngs {

template <typename T>
void setTweenOption(typename ci::Tween<T>::Options& option, const ci::JsonTree& param) noexcept {
  if (param.hasChild("loop")) {
    option.loop(param["loop"].getValue<bool>());
  }
  if (param.hasChild("ping-pong")) {
    option.pingPong(param["ping-pong"].getValue<bool>());
  }
  if (param.hasChild("delay")) {
    option.delay(param["delay"].getValue<float>());
  }
}


void setFloatTween(ci::Timeline& timeline,
                   ci::Anim<float>& value, const ci::JsonTree& param,
                   const bool is_first) noexcept {
  auto start = param.hasChild("start") ? param["start"].getValue<float>()
                                       : value();
  auto end = param.hasChild("end") ? param["end"].getValue<float>()
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<float>(option, param);
}


void setVec3Tween(ci::Timeline& timeline,
                  ci::Anim<ci::Vec3f>& value, const ci::JsonTree& param,
                  const bool is_first) noexcept {
  auto start = param.hasChild("start") ? Json::getVec3<float>(param["start"])
                                       : value();
  auto end = param.hasChild("end") ? Json::getVec3<float>(param["end"])
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<ci::Vec3f>(option, param);
}

void setQuatTween(ci::Timeline& timeline,
                  ci::Anim<ci::Quatf>& value, const ci::JsonTree& param,
                  const bool is_first) noexcept {
  auto start = param.hasChild("start") ? Json::getQuaternion<float>(param["start"])
                                       : value();
  auto end = param.hasChild("end") ? Json::getQuaternion<float>(param["end"])
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<ci::Quatf>(option, param);
}
  
void setColorTween(ci::Timeline& timeline,
                   ci::Anim<ci::Color>& value, const ci::JsonTree& param,
                   const bool is_first) noexcept {
  auto start = param.hasChild("start") ? Json::getColor<float>(param["start"])
                                       : value();
  auto end = param.hasChild("end") ? Json::getColor<float>(param["end"])
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<ci::Color>(option, param);
}


void setVec3Tween(ci::Timeline& timeline,
                  ci::Anim<ci::Vec3f>& value, const ci::JsonTree& param,
                  const ci::Vec3f& offset,
                  const bool is_first) noexcept {
  auto start = param.hasChild("start") ? Json::getVec3<float>(param["start"]) + offset
                                       : value();
  auto end = param.hasChild("end") ? Json::getVec3<float>(param["end"]) + offset
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<ci::Vec3f>(option, param);
}

void setHsvTween(ci::Timeline& timeline,
                 ci::Anim<ci::Vec3f>& value, const ci::JsonTree& param,
                 const bool is_first) noexcept {
  auto start = param.hasChild("start") ? Json::getHsvColor(param["start"])
                                       : value();
  auto end = param.hasChild("end") ? Json::getHsvColor(param["end"])
                                   : value();

  auto option = is_first ? timeline.apply(&value,
                                          start, end,
                                          param["duration"].getValue<float>(),
                                          getEaseFunc(param["type"].getValue<std::string>()))
                         : timeline.appendTo(&value,
                                             end,
                                             param["duration"].getValue<float>(),
                                             getEaseFunc(param["type"].getValue<std::string>()));

  if (is_first) value = start;
  setTweenOption<ci::Vec3f>(option, param);
}


bool isFirstApply(const std::string& type, std::set<std::string>& apply) noexcept {
  auto result = apply.insert(type);
  return result.second;
}

}
