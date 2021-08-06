#ifndef VKOVR_DEMO_ENGINE_UBO_LIGHT_UBO_H_
#define VKOVR_DEMO_ENGINE_UBO_LIGHT_UBO_H_

#include <glm/glm.hpp>

namespace demo
{
namespace engine
{
struct LightUbo
{
  struct Light
  {
    alignas(16) glm::vec4 position{ 0.f };
    alignas(16) glm::vec4 ambient{ 0.f };
    alignas(16) glm::vec4 diffuse{ 0.f };
    alignas(16) glm::vec4 specular{ 0.f };
  };

  static constexpr int MAX_NUM_LIGHTS = 8;
  Light lights[MAX_NUM_LIGHTS];
};
}
}

#endif //VKOVR_DEMO_ENGINE_UBO_LIGHT_UBO_H_
