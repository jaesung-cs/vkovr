#include <vkovr-demo/engine/pipeline_layout.h>

namespace demo
{
namespace engine
{
PipelineLayout createPipelineLayout(const PipelineLayoutCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto count = createInfo.count;
  const auto descriptorSetLayout = createInfo.descriptorSetLayout;
  const auto pushConstantRanges = createInfo.pushConstantRanges;

  std::vector<vk::DescriptorSetLayout> setLayouts(count, descriptorSetLayout);
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  pipelineLayoutCreateInfo
    .setSetLayouts(setLayouts)
    .setPushConstantRanges(pushConstantRanges);
  const auto pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

  PipelineLayout result;
  result.device_ = device;
  result.pipelineLayout_ = pipelineLayout;
  return result;
}

PipelineLayout::PipelineLayout()
{
}

PipelineLayout::~PipelineLayout()
{
}

void PipelineLayout::destroy()
{
  device_.destroyPipelineLayout(pipelineLayout_);
}
}
}
