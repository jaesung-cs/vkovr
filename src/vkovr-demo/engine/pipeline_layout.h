#ifndef VKOVR_DEMO_PIPELINE_LAYOUT_H_
#define VKOVR_DEMO_PIPELINE_LAYOUT_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class PipelineLayout;
class PipelineLayoutCreateInfo;

PipelineLayout createPipelineLayout(const PipelineLayoutCreateInfo& createInfo);

class PipelineLayout
{
  friend PipelineLayout createPipelineLayout(const PipelineLayoutCreateInfo& createInfo);

public:
  PipelineLayout();
  ~PipelineLayout();

  void destroy();

private:
  vk::Device device_;

  vk::PipelineLayout pipelineLayout_;
};

class PipelineLayoutCreateInfo
{
public:
  vk::Device device;
  vk::DescriptorSetLayout descriptorSetLayout;
  int count;
  std::vector<vk::PushConstantRange> pushConstantRanges;
};
}
}

#endif // VKOVR_DEMO_PIPELINE_LAYOUT_H_
