#include "allocator.hpp"
#include "logger.hpp"



namespace cqdeVk
{

std::string
AllocationScopeToString( const VkSystemAllocationScope scope )
{
  switch (scope)
  {
    case VkSystemAllocationScope::VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
      return "Command";

    case VkSystemAllocationScope::VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
      return "Object";

    case VkSystemAllocationScope::VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
      return "Cache";

    case VkSystemAllocationScope::VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
      return "Device";

    case VkSystemAllocationScope::VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
      return "Instance";

    default:
      assert(false);
  }
}


void
Allocator::printMemoryUsage() const
{
  size_t totalAllocatedBytes {};
  size_t totalAllocatedBlocks {};

  for ( auto&& [scope, allocations] : mOccupiedMemory )
  {
    LOG_INFO("Allocator: {} scope allocated {} bytes in {} memory blocks",
      AllocationScopeToString(scope),
      allocations.size, allocations.count );

    totalAllocatedBytes += allocations.size;
    totalAllocatedBlocks += allocations.count;
  }

  const auto internalAllocationsCount =
    totalAllocatedBlocks - mAllocatedBlocks.size();

  LOG_INFO("Allocator: In total, all scopes allocated {} bytes in {} memory blocks",
    totalAllocatedBytes, totalAllocatedBlocks );

  LOG_INFO("Allocator: In total, there are {} internal allocations",
    internalAllocationsCount );
}

void*
Allocator::allocate(
  const size_t size,
  const size_t alignment,
  const VkSystemAllocationScope scope )
{
  const auto data = std::aligned_alloc(
    alignment, size );

  if ( data == nullptr )
  {
    LOG_ERROR("Allocator: Failed to allocate {} bytes with {}-byte alignment",
      size, alignment );

    return nullptr;
  }

  mAllocatedBlocks[data] = {size, alignment, scope};

  auto& scopeAllocation = mOccupiedMemory[scope];
  scopeAllocation.size += size;
  scopeAllocation.count++;

  return data;
}

void*
Allocator::reallocate(
  void* data,
  const size_t size,
  const size_t alignment,
  const VkSystemAllocationScope scope )
{
  assert(data != nullptr);
  assert(size != 0);

  if ( mAllocatedBlocks.count(data) == 0 )
  {
    LOG_ERROR("Allocator: Failed to reallocate unknown memory block {}", data);

    return nullptr;
  }

  const auto& block = mAllocatedBlocks[data];

  assert(alignment == block.alignment);

  const auto newData = allocate(
    size, alignment, scope );

  if ( newData == nullptr )
  {
    LOG_ERROR("Allocator: Failed to reallocate {} bytes with {}-byte alignment to {} bytes with {}-byte alignment",
      block.size, block.alignment, size, alignment );

    return nullptr;
  }

  std::memcpy(
    newData, data,
    std::min(size, block.size) );

  deallocate(data);

  return newData;
}

void
Allocator::deallocate(
  void* data )
{
  if ( mAllocatedBlocks.count(data) == 0 )
    return;

  const auto& block = mAllocatedBlocks[data];

  LOG_TRACE("Allocator: Deallocating {} bytes from {} scope",
    block.size, AllocationScopeToString(block.scope) );

  auto& scopeAllocation = mOccupiedMemory[block.scope];
  scopeAllocation.size -= block.size;
  scopeAllocation.count--;

  mAllocatedBlocks.erase(data);
}

void
Allocator::allocate_internal(
  const size_t size,
  const VkInternalAllocationType type,
  const VkSystemAllocationScope scope )
{
  auto& scopeAllocation = mOccupiedMemory[scope];

  scopeAllocation.size += size;
  scopeAllocation.count++;
}

void
Allocator::deallocate_internal(
  const size_t size,
  const VkInternalAllocationType type,
  const VkSystemAllocationScope scope )
{
  auto& scopeAllocation = mOccupiedMemory[scope];

  scopeAllocation.size -= size;
  scopeAllocation.count--;
}


void*
allocate(
  void* pAllocator,
  size_t size,
  size_t alignment,
  VkSystemAllocationScope scope )
{
  const auto allocator =
    static_cast <Allocator*> (pAllocator);

  assert(allocator != nullptr);

  LOG_TRACE("Vulkan: allocating {} bytes within {} scope",
    size, AllocationScopeToString(scope) );


  return allocator->allocate(
    size, alignment, scope );
}

void*
reallocate(
  void* pAllocator,
  void* data,
  size_t size,
  size_t alignment,
  VkSystemAllocationScope scope )
{
  const auto allocator =
    static_cast <Allocator*> (pAllocator);

  assert(allocator != nullptr);

  LOG_TRACE("Vulkan: reallocating {} bytes within {} scope",
    size, AllocationScopeToString(scope) );

  if ( size == 0 )
  {
    allocator->deallocate(data);
    return nullptr;
  }


  void* newData {};

  if ( data != nullptr )
    newData = allocator->reallocate(
      data, size, alignment, scope );
  else
    newData = allocator->allocate(
      size, alignment, scope );


  if ( newData == nullptr )
    LOG_ERROR("Vulkan: Failed to reallocate memory block {} to {} bytes with {}-byte alignment",
      data, size, alignment );

  return newData;
}

void
free(
  void* pAllocator,
  void* data )
{
  const auto allocator =
    static_cast <Allocator*> (pAllocator);

  assert(allocator != nullptr);

  LOG_TRACE("Vulkan: deallocating memory block {}", data);


  allocator->deallocate(data);
}

void
internalAllocate(
  void* pAllocator,
  size_t size,
  VkInternalAllocationType type,
  VkSystemAllocationScope scope )
{
  const auto allocator =
    static_cast <Allocator*> (pAllocator);

  assert(allocator != nullptr);

  LOG_TRACE("Vulkan: internally allocating {} bytes within {} scope",
    size, AllocationScopeToString(scope) );


  allocator->allocate_internal(
    size, type, scope );
}

void
internalFree(
  void* pAllocator,
  size_t size,
  VkInternalAllocationType type,
  VkSystemAllocationScope scope )
{
  const auto allocator =
    static_cast <Allocator*> (pAllocator);

  assert(allocator != nullptr);

  LOG_TRACE("Vulkan: internally deallocating {} bytes from {} scope",
    size, AllocationScopeToString(scope) );


  allocator->deallocate_internal(
    size, type, scope );
}

} // namespace cqdeVk
