#include <vkovr-demo/scene/mesh.h>

namespace demo
{
namespace scene
{
Mesh::Mesh() = default;

Mesh::~Mesh() = default;

Mesh& Mesh::addVertex(const Vertex& vertex)
{
  vertices_.push_back(vertex);
  return *this;
}

Mesh& Mesh::addVertex(Vertex&& vertex)
{
  vertices_.emplace_back(std::move(vertex));
  return *this;
}

Mesh& Mesh::addFace(const glm::uvec3& face)
{
  faces_.push_back(face);
  return *this;
}

Mesh& Mesh::addFace(glm::uvec3&& face)
{
  faces_.emplace_back(std::move(face));
  return *this;
}
}
}
