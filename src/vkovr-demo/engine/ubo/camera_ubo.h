#ifndef VKOVR_DEMO_ENGINE_UBO_CAMERA_UBO_H_
#define VKOVR_DEMO_ENGINE_UBO_CAMERA_UBO_H_

#include <glm/glm.hpp>

namespace demo
{
namespace engine
{
struct CameraUbo
{
  glm::mat4 projection{ 0.f };
  glm::mat4 view{ 0.f };
  glm::vec3 eye{ 0.f };
};
}
}

#endif // VKOVR_DEMO_ENGINE_UBO_CAMERA_UBO_H_
