#ifndef VKOVR_DEMO_SCENE_CAMERA_H_
#define VKOVR_DEMO_SCENE_CAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace demo
{
namespace scene
{
class Camera
{
private:
  enum class Type
  {
    PERSPECTIVE,
    ORTHO,
  };

public:
  Camera() = default;
  ~Camera() = default;

  void setPerspective();
  void setOrtho();

  void setFovy(float fovy);
  void setZoom(float zoom);
  void setNearFar(float near, float far);
  void setScreenSize(int width, int height);

  void lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

  glm::mat4 projectionMatrix() const;
  glm::mat4 viewMatrix() const;

  const auto& eye() const { return eye_; }
  const auto& center() const { return center_; }
  const auto& up() const { return up_; }

private:
  Type type_ = Type::PERSPECTIVE;

  int width_ = 1;
  int height_ = 1;

  float near_ = 0.01f;
  float far_ = 100.f;

  float fovy_ = 60.f / 180.f * glm::pi<float>();

  float zoom_ = 1.f;

  glm::vec3 eye_{};
  glm::vec3 center_{};
  glm::vec3 up_{};
};
}
}

#endif // VKOVR_DEMO_SCENE_CAMERA_H_
