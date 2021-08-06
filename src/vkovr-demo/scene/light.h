#ifndef VKOVR_DEMO_SCENE_LIGHT_H_
#define VKOVR_DEMO_SCENE_LIGHT_H_

#include <glm/glm.hpp>

namespace demo
{
namespace scene
{
class Light
{
private:
  enum class Type
  {
    DIRECTIONAL,
    POINT,
  };

public:
  Light() = default;
  ~Light() = default;

  void setDirectionalLight() { type_ = Type::DIRECTIONAL; }
  void setPointLight() { type_ = Type::POINT; }
  void setPosition(const glm::vec3& position) { position_ = position; }
  void setAmbient(const glm::vec3& ambient) { ambient_ = ambient; }
  void setDiffuse(const glm::vec3& diffuse) { diffuse_ = diffuse; }
  void setSpecular(const glm::vec3& specular) { specular_ = specular; }

  bool isDirectionalLight() const { return type_ == Type::DIRECTIONAL; }
  bool isPointLight() const { return type_ == Type::POINT; }
  const glm::vec3& position() const { return position_; }
  const glm::vec3& ambient() const { return ambient_; }
  const glm::vec3& diffuse() const { return diffuse_; }
  const glm::vec3& specular() const { return specular_; }

private:
  Type type_;
  glm::vec3 position_{ 0.f, 0.f, 1.f };
  glm::vec3 ambient_{ 0.f, 0.f, 0.f };
  glm::vec3 diffuse_{ 0.f, 0.f, 0.f };
  glm::vec3 specular_{ 0.f, 0.f, 0.f };
};
}
}

#endif // VKOVR_DEMO_SCENE_LIGHT_H_
