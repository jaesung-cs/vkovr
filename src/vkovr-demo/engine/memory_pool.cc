#include <vkovr-demo/engine/memory_pool.h>

namespace demo
{
namespace engine
{
namespace
{
vk::DeviceSize align(vk::DeviceSize offset, vk::DeviceSize alignment)
{
  return (offset + alignment - 1) & ~(alignment - 1);
}
}

MemoryPool createMemoryPool(const MemoryPoolCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto physicalDevice = createInfo.physicalDevice;
  const auto deviceMemorySize = createInfo.deviceMemorySize;
  const auto hostMemorySize = createInfo.hostMemorySize;

  uint32_t hostIndex = 0;
  uint32_t deviceIndex = 0;

  // Find memroy type index
  uint64_t deviceAvailableSize = 0;
  uint64_t hostAvailableSize = 0;
  const auto memoryProperties = physicalDevice.getMemoryProperties();
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    const auto properties = memoryProperties.memoryTypes[i].propertyFlags;
    const auto heap_index = memoryProperties.memoryTypes[i].heapIndex;
    const auto heap = memoryProperties.memoryHeaps[heap_index];

    if ((properties & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal)
    {
      if (heap.size > deviceAvailableSize)
      {
        deviceIndex = i;
        deviceAvailableSize = heap.size;
      }
    }

    if ((properties & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
      == (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
    {
      if (heap.size > hostAvailableSize)
      {
        hostIndex = i;
        hostAvailableSize = heap.size;
      }
    }
  }

  vk::MemoryAllocateInfo memoryAllocateInfo;
  memoryAllocateInfo
    .setAllocationSize(deviceMemorySize)
    .setMemoryTypeIndex(deviceIndex);
  auto deviceMemory = device.allocateMemory(memoryAllocateInfo);

  memoryAllocateInfo
    .setAllocationSize(hostMemorySize)
    .setMemoryTypeIndex(hostIndex);
  auto hostMemory = device.allocateMemory(memoryAllocateInfo);

  MemoryPool memoryPool;
  memoryPool.device_ = device;
  memoryPool.hostIndex_ = hostIndex;
  memoryPool.memories_[0] = deviceMemory;
  memoryPool.memories_[1] = hostMemory;
  return memoryPool;
}

MemoryPool::MemoryPool()
{
  for (int i = 0; i < 2; i++)
    mutexes_.emplace_back(std::make_shared<std::mutex>());

  mappedMutex_ = std::make_shared<std::mutex>();
}

MemoryPool::~MemoryPool()
{
}

MemoryPool::MappedMemory MemoryPool::allocatePersistentlyMappedMemory(vk::Buffer buffer)
{
  return allocatePersistentlyMappedMemory(device_.getBufferMemoryRequirements(buffer));
}

MemoryPool::MappedMemory MemoryPool::allocatePersistentlyMappedMemory(vk::Image image)
{
  return allocatePersistentlyMappedMemory(device_.getImageMemoryRequirements(image));
}

MemoryPool::MappedMemory MemoryPool::allocatePersistentlyMappedMemory(const vk::MemoryRequirements& requirements)
{
  vk::MemoryAllocateInfo memoryAllocateInfo;
  memoryAllocateInfo
    .setAllocationSize(requirements.size)
    .setMemoryTypeIndex(hostIndex_);
  auto memory = device_.allocateMemory(memoryAllocateInfo);

  MappedMemory mappedMemory;
  mappedMemory.memory = memory;
  mappedMemory.offset = 0;
  mappedMemory.size = requirements.size;
  mappedMemory.map = reinterpret_cast<uint8_t*>(device_.mapMemory(memory, 0, requirements.size));

  {
    std::lock_guard<std::mutex> guard{ *mappedMutex_ };
    mappedMemories_.push_back(mappedMemory);
  }

  return mappedMemory;
}

MemoryPool::Memory MemoryPool::allocateDeviceMemory(vk::Buffer buffer)
{
  return allocateMemory(0, device_.getBufferMemoryRequirements(buffer));
}

MemoryPool::Memory MemoryPool::allocateDeviceMemory(vk::Image image)
{
  return allocateMemory(0, device_.getImageMemoryRequirements(image));
}

MemoryPool::Memory MemoryPool::allocateHostMemory(vk::Buffer buffer)
{
  return allocateMemory(1, device_.getBufferMemoryRequirements(buffer));
}

MemoryPool::Memory MemoryPool::allocateHostMemory(vk::Image image)
{
  return allocateMemory(1, device_.getImageMemoryRequirements(image));
}

MemoryPool::Memory MemoryPool::allocateMemory(int memoryIndex, const vk::MemoryRequirements& requirements)
{
  std::lock_guard<std::mutex> lock_guard{ *mutexes_[memoryIndex] };

  Memory memory;
  memory.memory = memories_[memoryIndex];
  memory.offset = align(offsets_[memoryIndex], requirements.alignment);
  memory.size = requirements.size;

  offsets_[memoryIndex] = memory.offset + memory.size;

  return memory;
}

void MemoryPool::destroy()
{
  for (const auto& memory : memories_)
    device_.freeMemory(memory);

  for (const auto& mappedMemory : mappedMemories_)
    device_.freeMemory(mappedMemory.memory);
  mappedMemories_.clear();
}
}
}
