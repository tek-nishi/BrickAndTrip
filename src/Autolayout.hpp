#pragma once

//
// 解像度に依存しないCameraでのAutolayout
//   カメラはまっすぐZ軸方向を向いている事
//

#include <utility>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/noncopyable.hpp>


namespace ngs {

using ViewRect = std::pair<ci::Ray, ci::Ray>;

ViewRect createViewRect(ci::Camera& camera) noexcept {
  float aspect_ratio = camera.getAspectRatio();

  auto top_left     = camera.generateRay(0.0f, 1.0f, aspect_ratio);
  auto bottom_right = camera.generateRay(1.0f, 0.0f, aspect_ratio);

  return std::make_pair(std::move(top_left), std::move(bottom_right));
}


using WorldRect = std::pair<ci::Vec3f, ci::Vec3f>;

WorldRect createWorldRect(const ViewRect& view_rect, const float z) noexcept {
  // origin + t * direction = z
  // から、tを求める
  const auto& origin    = view_rect.first.getOrigin();
  const auto& direction = view_rect.first.getDirection();
  float t = (z - origin.z) / direction.z;

  auto top_left     = view_rect.first.calcPosition(t);
  auto bottom_right = view_rect.second.calcPosition(t);

  return std::make_pair(std::move(top_left), std::move(bottom_right));
}


class Autolayout : private boost::noncopyable {
public:
  class Widget {
  public:
    enum Type {
      TOP_LEFT,
      TOP_CENTER,
      TOP_RIGHT,

      CENTER_LEFT,
      CENTER,
      CENTER_RIGHT,

      BOTTOM_LEFT,
      BOTTOM_CENTER,
      BOTTOM_RIGHT,

      TOP_MID_LEFT,
      TOP_MID_CENTER,
      TOP_MID_RIGHT,
    };

    
  private:
    Type origin_;
    Type layout_;

    ci::Vec3f pos_;
    ci::Vec3f size_;

    ci::Vec3f layout_pos_;
    ci::Vec3f layouted_pos_;

    
  public:
    explicit Widget(const Type origin,
                    const Type layout, const
                    ci::Vec3f& pos, const ci::Vec3f& size) noexcept :
      origin_(origin),
      layout_(layout),
      pos_(pos),
      size_(size),
      layout_pos_(pos_),
      layouted_pos_(pos_)
    { }


    const ci::Vec3f& getPos() const noexcept { return layouted_pos_; }

    void doLayout(const ViewRect& view_rect) noexcept {
      auto world_rect = createWorldRect(view_rect, pos_.z);

      struct Layout {
        ci::Vec3f first;
        ci::Vec3f second;
      };
      static const Layout layout_tbl[] = {
        { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },    // Type::TOP_LEFT
        { { 0.5f, 1.0f, 0.0f }, { 0.5f, 0.0f, 0.0f } },    // Type::TOP_CENTER
        { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },    // Type::TOP_RIGHT
        
        { { 1.0f, 0.5f, 0.0f }, { 0.0f, 0.5f, 0.0f } },    // Type::CENTER_LEFT
        { { 0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f } },    // Type::CENTER
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.5f, 0.0f } },    // Type::CENTER_RIGHT

        { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },    // Type::BOTTOM_LEFT
        { { 0.5f, 0.0f, 0.0f }, { 0.5f, 1.0f, 0.0f } },    // Type::BOTTOM_CENTER
        { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },    // Type::BOTTOM_RIGHT

        { { 1.0f, 0.75f, 0.0f }, { 0.0f, 0.25f, 0.0f } },  // Type::TOP_MID_LEFT
        { { 0.5f, 0.75f, 0.0f }, { 0.5f, 0.25f, 0.0f } },  // Type::TOP_MID_CENTER
        { { 0.0f, 0.75f, 0.0f }, { 1.0f, 0.25f, 0.0f } },  // Type::TOP_MID_RIGHT
      };

      const auto& layout = layout_tbl[layout_];
      ci::Vec3f pos = pos_ + world_rect.first  * layout.first
                           + world_rect.second * layout.second;
      layout_pos_ = pos;

      static const ci::Vec3f origin_tbl[] = {
        {  0.0f, -1.0f, 0.0f },    // Type::TOP_LEFT
        { -0.5f, -1.0f, 0.0f },    // Type::TOP_CENTER
        { -1.0f, -1.0f, 0.0f },    // Type::TOP_RIGHT
        
        {  0.0f, -0.5f, 0.0f },    // Type::CENTER_LEFT
        { -0.5f, -0.5f, 0.0f },    // Type::CENTER
        { -1.0f, -0.5f, 0.0f },    // Type::CENTER_RIGHT

        {  0.0f, 0.0f, 0.0f },     // Type::BOTTOM_LEFT
        { -0.5f, 0.0f, 0.0f },     // Type::BOTTOM_CENTER
        { -1.0f, 0.0f, 0.0f },     // Type::BOTTOM_RIGHT

        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_LEFT
        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_CENTER
        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_RIGHT
      };
      
      pos += size_ * origin_tbl[origin_];
      layouted_pos_ = pos;
    }

    void resizeWidget(const ci::Vec3f& size) noexcept {
      size_ = size;
      auto pos = layout_pos_;

      static const ci::Vec3f origin_tbl[] = {
        {  0.0f, -1.0f, 0.0f },    // Type::TOP_LEFT
        { -0.5f, -1.0f, 0.0f },    // Type::TOP_CENTER
        { -1.0f, -1.0f, 0.0f },    // Type::TOP_RIGHT
        
        {  0.0f, -0.5f, 0.0f },    // Type::CENTER_LEFT
        { -0.5f, -0.5f, 0.0f },    // Type::CENTER
        { -1.0f, -0.5f, 0.0f },    // Type::CENTER_RIGHT

        {  0.0f, 0.0f, 0.0f },     // Type::BOTTOM_LEFT
        { -0.5f, 0.0f, 0.0f },     // Type::BOTTOM_CENTER
        { -1.0f, 0.0f, 0.0f },     // Type::BOTTOM_RIGHT

        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_LEFT
        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_CENTER
        { 0.0f, 0.0f, 0.0f },      // Type::TOP_MID_RIGHT
      };

      pos += size_ * origin_tbl[origin_];
      layouted_pos_ = pos;
    }
  };

  
private:
  ViewRect view_rect_;
  std::vector<std::weak_ptr<Widget> > widgets_;

  
public:
  using WidgetRef = std::shared_ptr<Widget>;


  Autolayout() = default;
  
  explicit Autolayout(ci::Camera& camera) noexcept :
    view_rect_(createViewRect(camera))
  { }


  // Widget生成
  // 画面がresizeされた時にすべてのWidgetを計算し直すので、
  // このメソッドを使って生成する事
  WidgetRef makeWidget(const ci::Vec3f& pos, const ci::Vec3f& size,
                       const Widget::Type origin = Widget::Type::CENTER,
                       const Widget::Type layout = Widget::Type::CENTER) noexcept {

    auto widget = std::make_shared<Widget>(origin, layout, pos, size);
    widget->doLayout(view_rect_);

    // resizeの時に一気に書き換えるので、weak_ptrで保持
    widgets_.push_back(widget);

    return widget;
  }

  // 保持しているWidgetの位置を再計算する
  void resize(ci::Camera& camera) noexcept {
    view_rect_ = createViewRect(camera);

    // doLayoutのついでに無効なオブジェクトを削除している
    boost::remove_erase_if(widgets_,
                           [this](std::weak_ptr<Widget> widget) {
                             if (auto ptr = widget.lock()) {
                               ptr->doLayout(view_rect_);
                               return false;
                             }
                             return true;
                           });
  }

  // 無効なオブジェクトを取り除く
  void eraseInvalid() noexcept {
    boost::remove_erase_if(widgets_,
                           [this](std::weak_ptr<Widget> widget) {
                             return widget.expired();
                           });
  }


  // 文字列からWidget::Typeを取得
  static Widget::Type getType(const std::string& name) {
    static std::map<std::string, Widget::Type> layout = {
      {"top-left",   Widget::Type::TOP_LEFT },
      {"top-center", Widget::Type::TOP_CENTER },
      {"top-right",  Widget::Type::TOP_RIGHT },

      {"center-left",  Widget::Type::CENTER_LEFT },
      {"center",       Widget::Type::CENTER },
      {"center-right", Widget::Type::CENTER_RIGHT },

      {"bottom-left",   Widget::Type::BOTTOM_LEFT },
      {"bottom-center", Widget::Type::BOTTOM_CENTER },
      {"bottom-right",  Widget::Type::BOTTOM_RIGHT },
      
      {"top-mid-left",   Widget::Type::TOP_MID_LEFT },
      {"top-mid-center", Widget::Type::TOP_MID_CENTER },
      {"top-mid-right",  Widget::Type::TOP_MID_RIGHT },
    };

    return layout[name];
  }
  
};

}
