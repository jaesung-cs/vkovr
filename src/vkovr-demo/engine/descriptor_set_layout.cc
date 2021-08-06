#include <vkovr-demo/engine/descriptor_set_layout.h>

namespace demo
{
namespace engine
{
DescriptorSetLayout createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto& bindings = createInfo.bindings;

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
  descriptorSetLayoutCreateInfo
    .setBindings(bindings);
  const auto descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

  DescriptorSetLayout result;
  result.device_ = device;
  result.descriptorSetLayout_ = descriptorSetLayout;
  return result;
}

DescriptorSetLayout::DescriptorSetLayout()
{
}

DescriptorSetLayout::~DescriptorSetLayout()
{
}

void DescriptorSetLayout::destroy()
{
  device_.destroyDescriptorSetLayout(descriptorSetLayout_);
}
}
}
