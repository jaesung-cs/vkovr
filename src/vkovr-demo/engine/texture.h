#ifndef VKOVR_DEMO_ENGINE_TEXTURE_H_
#define VKOVR_DEMO_ENGINE_TEXTURE_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class MemoryPool;

class Texture;
class TextureCreateInfo;

Texture createTexture(const TextureCreateInfo& createInfo);

class Texture
{
  friend Texture createTexture(const TextureCreateInfo& createInfo);

public:
  Texture();
  ~Texture();

  auto getImageView() const { return imageView_; }

  void destroy();

private:
  vk::Device device_;

  uint32_t mipLevel_;
  vk::Image image_;
  vk::ImageView imageView_;
};

class TextureCreateInfo
{
public:
  vk::Device device;
  uint32_t width = 0;
  uint32_t height = 0;
  vk::Buffer srcBuffer;
  vk::DeviceSize srcOffset = 0;
  uint32_t mipLevel = 1;
  vk::CommandBuffer commandBuffer;
  MemoryPool* pMemoryPool;
};
}
}

#endif // VKOVR_DEMO_ENGINE_TEXTURE_H_
