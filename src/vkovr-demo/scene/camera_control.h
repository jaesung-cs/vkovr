#ifndef VKOVR_DEMO_SCENE_CAMERA_CONTROL_H_
#define VKOVR_DEMO_SCENE_CAMERA_CONTROL_H_

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace demo
{
namespace scene
{
class Camera;

class CameraControl
{
public:
  CameraControl() = delete;
  explicit CameraControl(std::shared_ptr<Camera> camera);
  ~CameraControl() = default;

  auto camera() const { return camera_; }

  void update();

  void translateByPixels(int dx, int dy);
  void rotateByPixels(int dx, int dy);
  void zoomByPixels(int dx, int dy);
  void zoomByWheel(int scroll);

  void moveForward(float dt);
  void moveRight(float dt);
  void moveUp(float dt);

private:
  std::shared_ptr<scene::Camera> camera_;

  // pos = center + radius * (cos theta cos phi, sin theta cos phi, sin phi)
  glm::vec3 center_{ 0.f, 0.f, 0.f };
  const glm::vec3 up_{ 0.f, 0.f, 1.f };
  float radius_ = 2.f;
  float theta_ = glm::pi<float>() * 1.5f;
  float phi_ = glm::pi<float>() / 2.f - 1e-2f;

  float translationSensitivity_ = 0.003f;
  float rotationSensitivity_ = 0.003f;
  float zoomSensitivity_ = 0.1f;
  float zoomWheelSensitivity_ = 5.f;
  float zoomMultiplier_ = 0.01f;
  float moveSpeed_ = 1.f;
};
}
}

#endif // VKOVR_DEMO_SCENE_CAMERA_CONTROL_H_
