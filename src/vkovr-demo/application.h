#ifndef VKOVR_DEMO_APPLICATION_H_
#define VKOVR_DEMO_APPLICATION_H_

#include <cstdint>

#include <vkovr/session.h>

#include <vkovr-demo/engine/engine.h>

struct GLFWwindow;

namespace demo
{
namespace scene
{
class Light;
class Camera;
class CameraControl;
}

class Application
{
public:
  Application();
  ~Application();

  void run();

  void mouseButton(int button, int action, int mods, double x, double y);
  void key(int key, int scancode, int action, int mods);
  void cursorPos(double x, double y);
  void scroll(double scroll);
  void resize(int width, int height);

private:
  void updateKeyboard(double dt);
  void updateCamera();
  void updateLight();

  uint32_t width_ = 800;
  uint32_t height_ = 450;
  GLFWwindow* window_ = nullptr;

  std::unique_ptr<engine::Engine> engine_;
  std::unique_ptr<scene::CameraControl> cameraControl_;

  // Light
  std::vector<std::shared_ptr<scene::Light>> lights_;

  // Events
  std::array<bool, 2> mouseButtons_{};
  double mouseLastX_ = 0.;
  double mouseLastY_ = 0.;
  std::array<bool, 512> keyPressed_{};

  // Animation
  bool isAnimated_ = false;
};
}

#endif // VKOVR_DEMO_APPLICATION_H_
