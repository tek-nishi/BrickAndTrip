//
// コロパレ
// 

#include "Defines.hpp"
#include "cinder/app/AppNative.h"
#include "cinder/Json.h"
#include "cinder/gl/gl.h"
#include "JsonUtil.hpp"
#include "Touch.hpp"
#include "Event.hpp"
#include "FontHolder.hpp"
#include "RootController.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class ColoColoParadeApp : public AppNative {
  JsonTree params_;

  double elapsed_seconds_;

  Vec2f mouse_pos_;
  Vec2f mouse_prev_pos_;

  Event<std::vector<ngs::Touch> > touch_event_;

  FontHolder fonts_;

  Color background_;
  
  std::unique_ptr<ControllerBase> controller_;

  
  void prepareSettings(Settings* settings) {
    // アプリ起動時の設定はここで処理する
    params_ = JsonTree(loadAsset("params.json"));

    auto size = Json::getVec2<int>(params_["app.size"]);
    settings->setWindowSize(size);

    background_ = Json::getColor<float>(params_["app.background"]);

    settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
    
    settings->enableMultiTouch();
  }
  
	void setup() {
    // Windowが表示された後の設定はここで処理
    // OpenGLのコンテキストも使える
#if defined(CINDER_MAC)
    // OSXでタイトルバーにアプリ名を表示するworkaround
    getWindow()->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
#endif

#if defined(CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([](){ return InterfaceOrientation::All; });
#endif

    setupFonts();
    
    controller_ = std::unique_ptr<ControllerBase>(new RootController(params_, touch_event_));

    // 以下OpenGL設定
    gl::enableDepthRead();
    gl::enableDepthWrite();

    gl::enableAlphaBlending();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl::enable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    gl::enable(GL_LIGHTING);
    gl::enable(GL_NORMALIZE);

    // TIPS: ci::gl::colorで色を決める
    //       ci::gl::Materialを使わない
    glEnable(GL_COLOR_MATERIAL);
#if !(CINDER_COCOA_TOUCH)
    // OpenGL ESは未対応
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#endif
    
    elapsed_seconds_ = getElapsedSeconds();
  }

  
  // FIXME:Windowsではtouchイベントとmouseイベントが同時に呼ばれる
	void mouseDown(MouseEvent event) {
    if (!event.isLeft()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-began", touches);
  }
  
	void mouseDrag(MouseEvent event) {
    if (!event.isLeftDown()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-moved", touches);
  }
  
	void mouseUp(MouseEvent event) {
    if (!event.isLeft()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-ended", touches);
  }


  void touchesBegan(TouchEvent event) {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-began", touches);
  }
  
  void touchesMoved(TouchEvent event) {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-moved", touches);
  }
  
  void touchesEnded(TouchEvent event) {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-ended", touches);
  }


  void keyDown(KeyEvent event) {
  }


  void resize() {
    controller_->resize();
  }
  
  
	void update() {
    double elapsed_seconds     = getElapsedSeconds();
    double progressing_seconds = elapsed_seconds - elapsed_seconds_;

    controller_->update(progressing_seconds);
    
    elapsed_seconds_ = elapsed_seconds;
  }
  
	void draw() {
    gl::clear(background_);

    controller_->draw();
  }


  void setupFonts() {
    const auto& fonts_params = params_["app.fonts"];

    for(const auto& p : fonts_params) {
      const auto& name = p["name"].getValue<std::string>();
      const auto& path = p["path"].getValue<std::string>();
      float size = p["size"].getValue<float>();
      fonts_.addFont(name, path, size);

      if (Json::getValue(p, "default", false)) {
        fonts_.setDefaultFont(name);
      }
    }
  }

  
  std::vector<ngs::Touch> createTouchInfo(const MouseEvent& event) {
    // TouchEvent同様、直前の位置も取れるように
    mouse_prev_pos_ = mouse_pos_;
    mouse_pos_ = event.getPos();

    std::vector<ngs::Touch> t = {
      { false,
        getElapsedSeconds(),
        ngs::Touch::MOUSE_EVENT_ID,
        mouse_pos_, mouse_prev_pos_ }
    };

    return std::move(t);
  }
  
  static std::vector<ngs::Touch> createTouchInfo(const TouchEvent& event) {
    std::vector<ngs::Touch> touches;
    
    const auto& event_touches = event.getTouches();
    for (const auto& touch : event_touches) {
      ngs::Touch t = {
        false,
        touch.getTime(),
        touch.getId(),
        touch.getPos(), touch.getPrevPos()
      };
      touches.push_back(std::move(t));
    }
    
    return std::move(touches);
  }
  
};

}

CINDER_APP_NATIVE( ngs::ColoColoParadeApp, RendererGl )
