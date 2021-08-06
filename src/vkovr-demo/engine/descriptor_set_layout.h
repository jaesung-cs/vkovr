#ifndef VKOVR_DEMO_DESCRIPTOR_SET_LAYOUT_H_
#define VKOVR_DEMO_DESCRIPTOR_SET_LAYOUT_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class DescriptorSetLayout;
class DescriptorSetLayoutCreateInfo;

DescriptorSetLayout createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo);

class DescriptorSetLayout
{
  friend DescriptorSetLayout createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo);

public:
  DescriptorSetLayout();
  ~DescriptorSetLayout();

  void destroy();

private:
  vk::Device device_;

  vk::DescriptorSetLayout descriptorSetLayout_;
};

class DescriptorSetLayoutCreateInfo
{
public:
  vk::Device device;
  std::vector<vk::DescriptorSetLayoutBinding> bindings;
};
}
}

#endif // VKOVR_DEMO_DESCRIPTOR_SET_LAYOUT_H_
