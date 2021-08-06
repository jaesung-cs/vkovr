#include <vkovr-demo/scene/camera_control.h>

#include <algorithm>

#include <vkovr-demo/scene/camera.h>

namespace demo
{
namespace scene
{
CameraControl::CameraControl(std::shared_ptr<scene::Camera> camera)
  : camera_(camera)
{
  update();
}

void CameraControl::update()
{
  const auto cosTheta = std::cos(theta_);
  const auto sinTheta = std::sin(theta_);
  const auto cosPhi = std::cos(phi_);
  const auto sinPhi = std::sin(phi_);

  const glm::vec3 eye = center_ + radius_ * glm::vec3(cosTheta * cosPhi, sinTheta * cosPhi, sinPhi);

  camera_->lookAt(eye, center_, up_);
}

void CameraControl::translateByPixels(int dx, int dy)
{
  const auto cosTheta = std::cos(theta_);
  const auto sinTheta = std::sin(theta_);
  const auto cosPhi = std::cos(phi_);
  const auto sinPhi = std::sin(phi_);

  const glm::vec3 x = radius_ * glm::vec3(-sinTheta, cosTheta, 0.f);
  const glm::vec3 y = radius_ * glm::vec3(cosTheta * -sinPhi, sinTheta * -sinPhi, cosPhi);

  center_ += translationSensitivity_ * (-x * static_cast<float>(dx) + y * static_cast<float>(dy));
}

void CameraControl::rotateByPixels(int dx, int dy)
{
  constexpr float epsilon = 1e-3f;
  constexpr auto phiLimit = glm::pi<float>() / 2.f - epsilon;

  theta_ -= rotationSensitivity_ * dx;
  phi_ = std::clamp(phi_ + rotationSensitivity_ * dy, -phiLimit, phiLimit);
}

void CameraControl::zoomByPixels(int dx, int dy)
{
  radius_ *= std::pow(1.f + zoomMultiplier_, zoomSensitivity_ * dy);
}

void CameraControl::zoomByWheel(int scroll)
{
  radius_ /= std::pow(1.f + zoomMultiplier_, zoomWheelSensitivity_ * scroll);
}

void CameraControl::moveForward(float dt)
{
  const auto cosTheta = std::cos(theta_);
  const auto sinTheta = std::sin(theta_);
  const auto cosPhi = std::cos(phi_);
  const auto sinPhi = std::sin(phi_);

  const auto forward = radius_ * -glm::vec3(cosTheta * cosPhi, sinTheta * cosPhi, sinPhi);

  center_ += moveSpeed_ * forward * dt;
}

void CameraControl::moveRight(float dt)
{
  const auto cosTheta = std::cos(theta_);
  const auto sinTheta = std::sin(theta_);

  const glm::vec3 x = radius_ * glm::vec3(-sinTheta, cosTheta, 0.f);

  center_ += moveSpeed_ * x * dt;
}

void CameraControl::moveUp(float dt)
{
  center_ += moveSpeed_ * (radius_ * up_) * dt;
}
}
}
