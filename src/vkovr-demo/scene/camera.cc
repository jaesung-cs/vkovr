#include <vkovr-demo/scene/camera.h>

namespace demo
{
namespace scene
{
void Camera::setPerspective()
{
  type_ = Type::PERSPECTIVE;
}

void Camera::setOrtho()
{
  type_ = Type::ORTHO;
}

void Camera::setFovy(float fovy)
{
  fovy_ = fovy;
}

void Camera::setZoom(float zoom)
{
  zoom_ = zoom;
}

void Camera::setNearFar(float near, float far)
{
  near_ = near;
  far_ = far;
}

void Camera::setScreenSize(int width, int height)
{
  width_ = width;
  height_ = height;
}

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
  eye_ = eye;
  center_ = center;
  up_ = up;
}

glm::mat4 Camera::projectionMatrix() const
{
  const auto aspect = static_cast<float>(width_) / height_;

  switch (type_)
  {
  case Type::PERSPECTIVE:
    return glm::perspective(fovy_, aspect, near_, far_);
  case Type::ORTHO:
    return glm::ortho(-aspect * zoom_, aspect * zoom_, -zoom_, zoom_, near_, far_);
  default:
    return glm::mat4(1.f);
  }
}

glm::mat4 Camera::viewMatrix() const
{
  return glm::lookAt(eye_, center_, up_);
}
}
}
