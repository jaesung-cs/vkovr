#ifndef VKOVR_DEMO_ENGINE_MEMORY_POOL_H_
#define VKOVR_DEMO_ENGINE_MEMORY_POOL_H_

#include <array>

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class MemoryPool;
class MemoryPoolCreateInfo;

MemoryPool createMemoryPool(const MemoryPoolCreateInfo& createInfo);

class MemoryPool
{
  friend MemoryPool createMemoryPool(const MemoryPoolCreateInfo& createInfo);

private:
  struct Memory
  {
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;
  };

  struct MappedMemory
  {
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    uint8_t* map;
  };

public:
  MemoryPool();
  ~MemoryPool();

  MappedMemory allocatePersistentlyMappedMemory(vk::Buffer buffer);
  MappedMemory allocatePersistentlyMappedMemory(vk::Image image);
  Memory allocateDeviceMemory(vk::Buffer buffer);
  Memory allocateDeviceMemory(vk::Image image);
  Memory allocateHostMemory(vk::Buffer buffer);
  Memory allocateHostMemory(vk::Image image);

  void destroy();

private:
  MappedMemory allocatePersistentlyMappedMemory(const vk::MemoryRequirements& requirements);
  Memory allocateMemory(int memoryIndex, const vk::MemoryRequirements& requirements);

  vk::Device device_;
  uint32_t hostIndex_ = 0;
  std::array<vk::DeviceMemory, 2> memories_;
  std::array<vk::DeviceSize, 2> offsets_ = { 0ull, 0ull };

  std::vector<MappedMemory> mappedMemories_;
};

class MemoryPoolCreateInfo
{
public:
  vk::Device device;
  vk::PhysicalDevice physicalDevice;
  vk::DeviceSize deviceMemorySize;
  vk::DeviceSize hostMemorySize;
};
}
}

#endif // VKOVR_DEMO_ENGINE_MEMORY_POOL_H_
