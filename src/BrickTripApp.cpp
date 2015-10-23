//
// BRICK TRIP
// 

#include "Defines.hpp"
#include <boost/noncopyable.hpp>
#include "cinder/app/AppNative.h"
#include "cinder/Json.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/System.h"
#include "DrawVboMesh.hpp"
#include "AntiAliasingType.hpp"
#include "Autolayout.hpp"
#include "JsonUtil.hpp"
#include "Touch.hpp"
#include "Event.hpp"
#include "FontHolder.hpp"
#include "RootController.hpp"
#include "Model.hpp"
#include "Params.hpp"
#include "AudioSession.h"
#include "GameCenter.h"


using namespace ci;
using namespace ci::app;


namespace ngs {

class BrickTripApp : public AppNative,
                     private boost::noncopyable {
  JsonTree params_;

  double fast_speed_;
  double slow_speed_;
  
  TimelineRef timeline_;

  double elapsed_seconds_;
  double max_elapsed_seconds_;

  double forward_speed_;
  bool forward_speed_change_;
  bool pause_;

  bool active_touch_;

  // FIXME:setupが呼ばれるまではCinderの機能を使ってはならない
  std::unique_ptr<FontHolder> fonts_;
  std::unique_ptr<ModelHolder> models_;
  
  Vec2f mouse_pos_;
  Vec2f mouse_prev_pos_;
  Event<std::vector<ngs::Touch> > touch_event_;
  
  std::unique_ptr<ControllerBase> controller_;

  
  void prepareSettings(Settings* settings) noexcept override {
    // アプリ起動時の設定はここで処理する
#if defined (OBFUSCATION_PARAMS) && defined (DEBUG)
    Params::convert("params.json");
#endif
    params_ = Params::load("params.json");

    // 低性能環境を調べて設定に追加
    params_["app.low_efficiency_device"] = ci::JsonTree("low_efficiency_device",
                                                        ngs::LowEfficiencyDevice::determine());
#ifdef DEBUG
    // 強制的に低性能環境
    if (params_["app.force_low_efficiency"].getValue<bool>()) {
      params_["app.low_efficiency_device"] = ci::JsonTree("low_efficiency_device", true);
    }
#endif

    auto size = Json::getVec2<int>(params_["app.size"]);
    settings->setWindowSize(size);

    settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
    if (!params_["app.framerate_limit"].getValue<bool>() && gl::isVerticalSyncEnabled()) {
      settings->disableFrameRate();
    }
    
#if !defined(CINDER_MAC)
    // FIXME:Macでだとマルチタッチが邪魔
    active_touch_ = ci::System::hasMultiTouch();
    if (active_touch_) {
      settings->enableMultiTouch();
    }
#endif
    
    // Retina Display対応
    settings->enableHighDensityDisplay();

    fast_speed_ = params_["app.fast_speed"].getValue<double>();
    slow_speed_ = params_["app.slow_speed"].getValue<double>();

    setEasingParams(params_);
  }
  

	void setup() noexcept override {
    beginAudioSession();
    GameCenter::authenticateLocalPlayer();
    
    
    Rand::randomize();
    
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

    timeline_ = Timeline::create();
    forward_speed_        = 1.0;
    forward_speed_change_ = false;
    pause_ = false;
    
    setupFonts();
    setupModels();
    
    controller_ = std::unique_ptr<ControllerBase>(new RootController(params_, timeline_, touch_event_));

    // 以下OpenGL設定
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::disableAlphaTest();

    gl::enable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    gl::enable(GL_NORMALIZE);

#if !defined(CINDER_COCOA_TOUCH)
    // OpenGL ESは未対応
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#endif
    // TIPS: ambientとdiffuseをci::gl::colorで決める
    gl::enable(GL_COLOR_MATERIAL);

    elapsed_seconds_ = getElapsedSeconds();
    
    // 起動後安定するまでは経過時間の許容を少なく
    auto v = Json::getVec2<double>(params_["app.max_elapsed_seconds"]);
    max_elapsed_seconds_ = 1.0 / v.x;

    timeline().add([this, v]() {
        max_elapsed_seconds_ = 1.0 / v.y;
      },
      timeline().getCurrentTime() + params_["app.startup_frame"].getValue<float>());
  }
  
  void shutdown() noexcept override {
    DOUT << "shutdown()" << std::endl;
    
    timeline_->clear();
    endAudioSession();
  }

  
  // FIXME:Windowsではtouchイベントとmouseイベントが同時に呼ばれる
	void mouseDown(MouseEvent event) noexcept override {
    if (!event.isLeft()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-began", touches);
  }
  
	void mouseDrag(MouseEvent event) noexcept override {
    if (!event.isLeftDown()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-moved", touches);
  }
  
	void mouseUp(MouseEvent event) noexcept override {
    if (!event.isLeft()) return;

    mouse_pos_ = event.getPos();

    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-ended", touches);
  }


  void touchesBegan(TouchEvent event) noexcept override {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-began", touches);
  }
  
  void touchesMoved(TouchEvent event) noexcept override {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-moved", touches);
  }
  
  void touchesEnded(TouchEvent event) noexcept override {
    auto touches = createTouchInfo(event);
    touch_event_.signal("touches-ended", touches);
  }


#ifdef DEBUG
  void keyDown(KeyEvent event) noexcept override {
    char chara = event.getChar();
    int  code  = event.getCode();

    if (chara == 'R') {
      // Soft Reset
      // TIPS:先にresetを実行。Controllerが二重に確保されるのを避ける
      controller_.reset();
      timeline_->clear();

#if defined (OBFUSCATION_PARAMS) && defined (DEBUG)
      Params::convert("params.json");
#endif
      params_ = Params::load("params.json");

      controller_ = std::unique_ptr<ControllerBase>(new RootController(params_, timeline_, touch_event_));
      // すぐさまresizeを呼んでCameraの調整
      resize();

      return;
    }
    
    if (code == KeyEvent::KEY_LCTRL) {
      // 倍速モード
      forward_speed_change_ = true;
      forward_speed_        = fast_speed_;
    }
    else if (code == KeyEvent::KEY_LALT) {
      // 低速モード
      forward_speed_change_ = true;
      forward_speed_        = slow_speed_;
    }
    else if (code == KeyEvent::KEY_ESCAPE) {
      // 強制PAUSE
      pause_ = !pause_;
    }
    else if (event.isMetaDown()) {
      int fixed_size = static_cast<int>(params_["app.fixed_size"].getNumChildren());
      int index = chara - '1';
      if ((index >= 0) && (index < fixed_size)) {
        auto size = Json::getVec2<int>(params_["app.fixed_size"][index]);
        setWindowSize(size);

        // 位置は画面中央に
        DisplayRef display = Display::getMainDisplay();
        auto display_size = display->getSize();
        setWindowPos((display_size - size) / 2);
      }
    }
    else if ((chara < '0') || (chara > '9')) {
      // JsonTree::hasChild では数字の場合に自動的にインデックス値
      // で判断してくれるので、数字キーは除外してから処理

      // paramsに書かれたsignalを発生
      // TIPS:大文字小文字の区別をしている
      auto debug_signal = std::string(1, chara);
      if (params_["app.debug"].hasChild(debug_signal, true)) {
        auto message = params_.getChild("app.debug." + debug_signal, true).getValue<std::string>();
        
        DOUT << "debug-signal: "
             << debug_signal
             << " "
             << message << std::endl;
      
        controller_->event().signal(message, EventParam());
      }
    }
  }
  
  void keyUp(KeyEvent event) noexcept override {
    char chara = event.getChar();
    int  code  = event.getCode();

    if ((code == KeyEvent::KEY_LCTRL) || (code == KeyEvent::KEY_LALT)) {
      // 低・倍速モード解除
      forward_speed_change_ = false;
    }
  }
#endif


  void resize() noexcept override {
    controller_->resize();
  }


  void updateApp() {
    double elapsed_seconds = getElapsedSeconds();

    // 経過時間が大きな値になりすぎないよう調整
    double progressing_seconds = std::min(elapsed_seconds - elapsed_seconds_,
                                          max_elapsed_seconds_);

#ifdef DEBUG
    if (pause_) progressing_seconds = 0.0;
    if (forward_speed_change_) {
      progressing_seconds *= forward_speed_;
    }
#endif
    
    timeline_->step(progressing_seconds);
    controller_->update(progressing_seconds);
    
    elapsed_seconds_ = elapsed_seconds;
  }

  
#if !defined(CINDER_MSW)
  // FIXME:更新処理をここでおこなうとWindowsでフレームレートが安定しない
	void update() noexcept override {
    updateApp();
  }
#endif
  
	void draw() noexcept override {
#if defined(CINDER_MSW)
    updateApp();
#endif
    controller_->draw(*fonts_, *models_);
  }


  void setupFonts() noexcept {
    fonts_ = std::unique_ptr<FontHolder>(new FontHolder);

    const auto& fonts_params = params_["app.fonts"];

    bool do_mipmap = true;
    if (params_["app.low_efficiency_device"].getValue<bool>()) {
      // 低性能の実行環境ではFontのmipmapを強制OFF
      do_mipmap = false;

      DOUT << "Font:no mipmap." << std::endl;
    }
    
    for(const auto& p : fonts_params) {
      const auto& name = p["name"].getValue<std::string>();
      const auto& path = p["path"].getValue<std::string>();
      int size = p["size"].getValue<int>();
      ci::Vec3f scale  = Json::getVec3<float>(p["scale"]);
      ci::Vec3f offset = Json::getVec3<float>(p["offset"]);
      bool mipmap = p["mipmap"].getValue<bool>() && do_mipmap;
      
      auto& font = fonts_->addFont(name, path, size, scale, offset, mipmap);

      if (Json::getValue(p, "default", false)) {
        fonts_->setDefaultFont(name);
      }

      // アプリ起動時に事前に文字をレンダリングしてキャッシュに入れておく
      // iOS:処理の引っ掛かりを減らす
      if (Json::getValue(p, "pre_render", false)) {
        auto text = p["pre_render_text"].getValue<std::string>();
        auto text_length = strlen(text);
        for (size_t it = 0; it < text_length; ++it) {
          font.getTextureFromString(substr(text, 1));
        }
      }
    }
  }

  void setupModels() noexcept {
    models_ = std::unique_ptr<ModelHolder>(new ModelHolder);

    // 低性能環境はポリゴン数の少ないモデルを使う
    auto model_path = params_["app.low_efficiency_device"].getValue<bool>() ? "path_low"
                                                                            : "path";
    
    for(const auto& p : params_["app.models"]) {
      const auto& name = p["name"].getValue<std::string>();
      const auto& path = p.hasChild(model_path) ? p[model_path].getValue<std::string>()
                                                : p["path"].getValue<std::string>();
      bool has_normals = p["normals"].getValue<bool>();
      bool has_uvs = p["uvs"].getValue<bool>();
      bool has_indices = p["indices"].getValue<bool>();
      
      models_->add(name, path, has_normals, has_uvs, has_indices);
    }
  }

  
  std::vector<ngs::Touch> createTouchInfo(const MouseEvent& event) noexcept {
    // TouchEvent同様、直前の位置も取れるように
    mouse_prev_pos_ = mouse_pos_;
    mouse_pos_ = event.getPos();

    std::vector<ngs::Touch> t = {
      { false,
        getElapsedSeconds(),
        ngs::Touch::MOUSE_EVENT_ID,
        mouse_pos_, mouse_prev_pos_ }
    };

    return t;
  }
  
  static std::vector<ngs::Touch> createTouchInfo(const TouchEvent& event) noexcept {
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
    
    return touches;
  }

  static void setEasingParams(const JsonTree& params) noexcept {
    ease_in_elastic_a = params["easing.ease_in_elastic_a"].getValue<float>();
    ease_in_elastic_b = params["easing.ease_in_elastic_b"].getValue<float>();

    ease_out_elastic_a = params["easing.ease_out_elastic_a"].getValue<float>();
    ease_out_elastic_b = params["easing.ease_out_elastic_b"].getValue<float>();

    ease_inout_elastic_a = params["easing.ease_inout_elastic_a"].getValue<float>();
    ease_inout_elastic_b = params["easing.ease_inout_elastic_b"].getValue<float>();

    ease_outin_elastic_a = params["easing.ease_outin_elastic_a"].getValue<float>();
    ease_outin_elastic_b = params["easing.ease_outin_elastic_b"].getValue<float>();
  }
  
};

}

CINDER_APP_NATIVE(ngs::BrickTripApp, RendererGl(ngs::AntiAliasingType::get()))
