#ifndef VKOVR_DEMO_SCENE_MESH_H_
#define VKOVR_DEMO_SCENE_MESH_H_

#include <vector>

#include <glm/glm.hpp>

namespace demo
{
namespace scene
{
class Mesh
{
public:
  struct Vertex
  {
    glm::vec3 position{ 0.f };
    glm::vec3 normal{ 0.f };
    glm::vec2 tex_coord{ 0.f };
  };

public:
  Mesh();
  ~Mesh();

  Mesh& addVertex(const Vertex& vertex);
  Mesh& addVertex(Vertex&& vertex);

  Mesh& addFace(const glm::uvec3& face);
  Mesh& addFace(glm::uvec3&& face);

  void setHasNormal() { hasNormal_ = true; }
  void setHasTexture() { hasTexture_ = true; }

  const auto& vertices() const { return vertices_; }
  const auto& faces() const { return faces_; }
  auto hasNormal() const { return hasNormal_; }
  auto hasTexture() const { return hasTexture_; }

private:
  bool hasNormal_ = false;
  bool hasTexture_ = false;

  std::vector<Vertex> vertices_;
  std::vector<glm::uvec3> faces_;
};
}
}

#endif // VKOVR_DEMO_SCENE_MESH_H_
