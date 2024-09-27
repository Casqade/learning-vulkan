#pragma once

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <map>
#include <unordered_map>


namespace cqdeVk
{

class Allocator
{
  struct AllocatedBlock
  {
    size_t size {};
    size_t alignment {};
    VkSystemAllocationScope scope {};

    AllocatedBlock() = default;
  };

  struct ScopeAllocation
  {
    size_t size {};
    size_t count {};

    ScopeAllocation() = default;
  };

  using AllocationScopeMap =
    std::map <VkSystemAllocationScope, ScopeAllocation>;

  using AllocatedBlocks =
    std::unordered_map <void*, AllocatedBlock>;

  AllocatedBlocks mAllocatedBlocks {};
  AllocationScopeMap mOccupiedMemory {};


public:
  Allocator() = default;


  void printMemoryUsage() const;

  void* allocate(
    const size_t size,
    const size_t alignment,
    const VkSystemAllocationScope );

  void* reallocate(
    void* data,
    const size_t size,
    const size_t alignment,
    const VkSystemAllocationScope );

  void deallocate( void* data );


  void allocate_internal(
    const size_t size,
    const VkInternalAllocationType,
    const VkSystemAllocationScope );

  void deallocate_internal(
    const size_t size,
    const VkInternalAllocationType,
    const VkSystemAllocationScope );
};


void*
allocate(
  void* pAllocator,
  size_t size,
  size_t alignment,
  VkSystemAllocationScope );

void* reallocate(
  void* pAllocator,
  void* data,
  size_t size,
  size_t alignment,
  VkSystemAllocationScope );

void free(
  void* pAllocator,
  void* data );

void internalAllocate(
  void* pAllocator,
  size_t size,
  VkInternalAllocationType,
  VkSystemAllocationScope );

void internalFree(
  void* pAllocator,
  size_t size,
  VkInternalAllocationType,
  VkSystemAllocationScope );

} // namespace cqdeVk
