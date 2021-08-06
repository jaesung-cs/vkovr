#include <vkovr-demo/engine/sampler.h>

namespace demo
{
namespace engine
{

Sampler createSampler(const SamplerCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto mipLevel = createInfo.mipLevel;

  vk::SamplerCreateInfo samplerCreateInfo;
  samplerCreateInfo
    .setMagFilter(vk::Filter::eLinear)
    .setMinFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
    .setMipLodBias(0.f)
    .setAnisotropyEnable(true)
    .setMaxAnisotropy(16.f)
    .setCompareEnable(false)
    .setMinLod(0.f)
    .setMaxLod(mipLevel)
    .setBorderColor(vk::BorderColor::eFloatTransparentBlack)
    .setUnnormalizedCoordinates(false);
  const auto sampler = device.createSampler(samplerCreateInfo);

  Sampler result;
  result.device_ = device;
  result.sampler_ = sampler;
  result.mipLevel_ = mipLevel;
  return result;
}

Sampler::Sampler() = default;

Sampler::~Sampler() = default;

void Sampler::destroy()
{
  device_.destroySampler(sampler_);
}
}
}
