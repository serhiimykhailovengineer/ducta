/**
 * @file NodeStorage.hpp
 * @brief Where NodeManager keeps the nodes it creates
 *
 * A storage policy is a class with
 *     void* allocate(size_t size, size_t align) noexcept;  // nullptr if it does not fit
 *     void deallocate(void* memory, size_t align) noexcept; // undo the last allocate()
 *     void adopt(void* object, size_t align, NodeDestroyFn destroy);
 *     void clear() noexcept;                               // destroys nodes in reverse order
 *     size_t size() const noexcept;                        // number of nodes
 * adopt() takes ownership of an object constructed in memory returned by the last allocate().
 * Provided policies: StaticNodeStorage (fixed buffer, no heap) and HeapNodeStorage
 * (std::unique_ptr per node, standard backend only).
 */

#ifndef DUCTA_PIPELINE_NODE_STORAGE_HPP
#define DUCTA_PIPELINE_NODE_STORAGE_HPP

#include "ducta/Core/Config.hpp"

#include <cstddef>
#include <new>

#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Utility.hpp"

#include "ducta/Pipeline/NodeRef.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <memory>
#include <vector>
#endif

namespace ducta {
namespace Pipeline {

/* Destroys a node in place (runs its destructor, does not free its memory) */
using NodeDestroyFn = void (*)(void* node);

namespace Private {

template <class T>
void destroy_node(void* node)
{
    static_cast<T*>(node)->~T();
}

} // namespace Private

/*
 * @brief Type-erased access to the storage of a NodeManager, given to CreateNodeFn
 */
class NodeStorage
{
public:
    /*
     * @brief Construct T(args...) in the storage and hand it over to its owner
     * @return The node, or nullptr if the storage has no room for it (nothing is constructed)
     */
    template <class T, class... Args>
    T* construct(Args&&... args)
    {
        void* memory = allocate(sizeof(T), alignof(T));
        if (memory == nullptr)
        {
            return nullptr;
        }
        T* node = nullptr;
#if defined(__cpp_exceptions)
        try
        {
            node = ::new (memory) T(::ducta::forward<Args>(args)...);
        }
        catch (...)
        {
            deallocate(memory, alignof(T));
            throw;
        }
#else
        node = ::new (memory) T(::ducta::forward<Args>(args)...);
#endif
        adopt(NodeRef{*node}, node, alignof(T), &Private::destroy_node<T>);
        return node;
    }

protected:
    ~NodeStorage() = default;

    virtual void* allocate(size_t size, size_t align) noexcept = 0;
    /* Give back the memory of the last allocate() when the node could not be constructed */
    virtual void deallocate(void* memory, size_t align) noexcept = 0;
    virtual void adopt(NodeRef node, void* object, size_t align, NodeDestroyFn destroy) = 0;
};

/*
 * @brief Nodes are placed one after another in a buffer inside the storage object
 * @tparam Bytes Size of the buffer (node sizes plus alignment padding)
 * @tparam MaxNodes Maximum number of nodes
 */
template <size_t Bytes, size_t MaxNodes>
class StaticNodeStorage
{
public:
    StaticNodeStorage() = default;
    StaticNodeStorage(StaticNodeStorage const&) = delete;
    StaticNodeStorage& operator=(StaticNodeStorage const&) = delete;

    ~StaticNodeStorage()
    {
        clear();
    }

    void* allocate(size_t size, size_t align) noexcept
    {
        if (m_count == MaxNodes)
        {
            return nullptr;
        }
        uintptr_t const base = reinterpret_cast<uintptr_t>(m_buffer);
        size_t const offset = static_cast<size_t>((base + m_used + align - 1) / align * align - base);
        if (offset > Bytes || size > Bytes - offset)
        {
            return nullptr;
        }
        m_used_before_last = m_used;
        m_used = offset + size;
        return m_buffer + offset;
    }

    void deallocate(void* /*memory*/, size_t /*align*/) noexcept
    {
        m_used = m_used_before_last;
    }

    void adopt(void* object, size_t /*align*/, NodeDestroyFn destroy)
    {
        m_entries[m_count++] = Entry{object, destroy};
    }

    void clear() noexcept
    {
        while (m_count > 0)
        {
            Entry const& entry = m_entries[--m_count];
            entry.destroy(entry.object);
        }
        m_used = 0;
        m_used_before_last = 0;
    }

    size_t size() const noexcept
    {
        return m_count;
    }

    /* Bytes of the buffer in use (including alignment padding) */
    size_t used() const noexcept
    {
        return m_used;
    }

private:
    struct Entry
    {
        void* object;
        NodeDestroyFn destroy;
    };

    alignas(alignof(std::max_align_t)) unsigned char m_buffer[Bytes];
    Entry m_entries[MaxNodes]{};
    size_t m_count{0};
    size_t m_used{0};
    size_t m_used_before_last{0};
};

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
/*
 * @brief Every node is allocated on the heap and owned by a std::unique_ptr
 */
class HeapNodeStorage
{
public:
    HeapNodeStorage() = default;
    HeapNodeStorage(HeapNodeStorage const&) = delete;
    HeapNodeStorage& operator=(HeapNodeStorage const&) = delete;

    ~HeapNodeStorage()
    {
        clear();
    }

    void* allocate(size_t size, size_t align) noexcept
    {
        return ::operator new(size, std::align_val_t{align}, std::nothrow);
    }

    void deallocate(void* memory, size_t align) noexcept
    {
        ::operator delete(memory, std::align_val_t{align});
    }

    void adopt(void* object, size_t align, NodeDestroyFn destroy)
    {
        Deleter deleter{destroy, align};
#if defined(__cpp_exceptions)
        try
        {
            m_nodes.emplace_back(object, deleter);
        }
        catch (...)
        {
            // Not owned yet (e.g. bad_alloc of the vector): destroy and free here
            deleter(object);
            throw;
        }
#else
        m_nodes.emplace_back(object, deleter);
#endif
    }

    void clear() noexcept
    {
        while (!m_nodes.empty())
        {
            m_nodes.pop_back();
        }
    }

    size_t size() const noexcept
    {
        return m_nodes.size();
    }

private:
    struct Deleter
    {
        NodeDestroyFn destroy;
        size_t align;

        void operator()(void* object) const noexcept
        {
            destroy(object);
            ::operator delete(object, std::align_val_t{align});
        }
    };

    std::vector<std::unique_ptr<void, Deleter>> m_nodes;
};
#endif

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_STORAGE_HPP
