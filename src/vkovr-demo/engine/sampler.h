#ifndef VKOVR_DEMO_ENGINE_SAMPLER_H_
#define VKOVR_DEMO_ENGINE_SAMPLER_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class Sampler;
class SamplerCreateInfo;

Sampler createSampler(const SamplerCreateInfo& createInfo);

class Sampler
{
  friend Sampler createSampler(const SamplerCreateInfo& createInfo);

public:
  Sampler();
  ~Sampler();

  operator vk::Sampler() const { return sampler_; }

  void destroy();

private:
  vk::Device device_;
  vk::Sampler sampler_;
  uint32_t mipLevel_;
};

class SamplerCreateInfo
{
public:
  vk::Device device;
  uint32_t mipLevel = 1;
};
}
}

#endif // VKOVR_DEMO_ENGINE_SAMPLER_H_
