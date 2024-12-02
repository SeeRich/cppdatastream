#ifndef CPPDS_ALIGNED_ALLOCATOR_H
#define CPPDS_ALIGNED_ALLOCATOR_H

#pragma once

#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

/**
 * Returns aligned pointers when allocations are requested. Default alignment
 * is 64B = 512b, sufficient for AVX-512 and most cache line sizes.
 *
 * @tparam ALIGNMENT_IN_BYTES Must be a positive power of 2.
 */
template <typename ElementType, std::size_t ALIGNMENT_IN_BYTES = 64>
class AlignedAllocator
{
private:
    static_assert(ALIGNMENT_IN_BYTES >= alignof(ElementType),
                  "Beware that types like int have minimum alignment requirements "
                  "or access will result in crashes.");

public:
    constexpr AlignedAllocator() noexcept {}

    constexpr AlignedAllocator([[maybe_unused]] const AlignedAllocator& other) noexcept {}

    // SEE:
    // https://stackoverflow.com/questions/72238649/getting-weird-compiler-errors-while-compiling-my-own-allocator-with-msvc-202
    // We could disable MSCVs iterator debugging structures with #define _ITERATOR_DEBUG_LEVEL 0 but these are nice to
    // have around if possible
    template <typename T2>
    constexpr AlignedAllocator(const AlignedAllocator<T2>&) noexcept
    {}

    using value_type = ElementType;
    static std::align_val_t constexpr ALIGNMENT{ALIGNMENT_IN_BYTES};

    /**
     * This is only necessary because AlignedAllocator has a second template
     * argument for the alignment that will make the default
     * std::allocator_traits implementation fail during compilation.
     * @see https://stackoverflow.com/a/48062758/2191065
     */
    template <class OtherElementType>
    struct rebind
    {
        using other = AlignedAllocator<OtherElementType, ALIGNMENT_IN_BYTES>;
    };

    [[nodiscard]] ElementType* allocate(std::size_t nElementsToAllocate)
    {
        if(nElementsToAllocate > std::numeric_limits<std::size_t>::max() / sizeof(ElementType)) {
            throw std::bad_array_new_length();
        }

        auto const nBytesToAllocate = nElementsToAllocate * sizeof(ElementType);
        return reinterpret_cast<ElementType*>(::operator new[](nBytesToAllocate, ALIGNMENT));
    }

    void deallocate(ElementType* allocatedPointer, [[maybe_unused]] std::size_t nBytesAllocated)
    {
        /* According to the C++20 draft n4868 § 17.6.3.3, the delete operator
         * must be called with the same alignment argument as the new expression.
         * The size argument can be omitted but if present must also be equal to
         * the one used in new. */
        ::operator delete[](allocatedPointer, ALIGNMENT);
    }
};

// Define aligned vector type
template <typename T, std::size_t ALIGNMENT_IN_BYTES = 64>
using AlignedVector = std::vector<T, AlignedAllocator<T, ALIGNMENT_IN_BYTES>>;

#endif