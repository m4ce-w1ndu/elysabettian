#pragma once

#include <ely/vm/heap_object.hpp>
#include <cassert>
#include <cstddef>
#include <utility>
#include <variant>
#include <vector>

namespace ely::vm {

/**
 * Manages dynamically allocated runtime objects for the virtual machine.
 *
 * Encapsulates object lifecycle, provides type-safe accessors via `std::variant`,
 * and assigns unique 48-bit heap identifiers compatible with IEEE-754 NaN-boxing.
 */
class Heap {
public:
    /**
     * Default constructor creating an empty object heap.
     */
    Heap() = default;

    /**
     * Allocates a heap object and returns its unique identifier index.
     *
     * @param object The HeapObject variant to store.
     * @return 0-based identifier of the allocated object.
     */
    size_t allocate(HeapObject object) {
        m_objects.push_back(std::move(object));
        return m_objects.size() - 1;
    }

    /**
     * Constructs a heap object in-place and returns its identifier index.
     *
     * @tparam T Concrete object type (e.g., ObjFunction, ObjClosure, std::string).
     * @tparam Args Argument types forwarded to T's constructor.
     * @param args Arguments forwarded to construct T.
     * @return 0-based identifier of the emplaced object.
     */
    template <typename T, typename... Args>
    size_t emplace(Args&&... args) {
        m_objects.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
        return m_objects.size() - 1;
    }

    /**
     * Retrieves a mutable reference to the heap object with the given identifier.
     *
     * @param id Identifier index of the object.
     * @return Mutable reference to the HeapObject variant.
     */
    [[nodiscard]] HeapObject& get(size_t id) {
        assert(id < m_objects.size() && "Heap object index out of bounds.");
        return m_objects[id];
    }

    /**
     * Retrieves a const reference to the heap object with the given identifier.
     *
     * @param id Identifier index of the object.
     * @return Read-only reference to the HeapObject variant.
     */
    [[nodiscard]] const HeapObject& get(size_t id) const {
        assert(id < m_objects.size() && "Heap object index out of bounds.");
        return m_objects[id];
    }

    /**
     * Attempts to access a heap object as a specific concrete type.
     *
     * @tparam T Concrete type to retrieve (e.g. ObjClosure, std::string).
     * @param id Identifier index of the object.
     * @return Pointer to T if the ID is valid and the variant holds T, nullptr otherwise.
     */
    template <typename T>
    [[nodiscard]] T* get_if(size_t id) noexcept {
        if (id >= m_objects.size()) return nullptr;
        return std::get_if<T>(&m_objects[id]);
    }

    /**
     * Attempts to access a heap object as a specific concrete type (read-only).
     *
     * @tparam T Concrete type to retrieve (e.g. ObjClosure, std::string).
     * @param id Identifier index of the object.
     * @return Const pointer to T if the ID is valid and matches T, nullptr otherwise.
     */
    template <typename T>
    [[nodiscard]] const T* get_if(size_t id) const noexcept {
        if (id >= m_objects.size()) return nullptr;
        return std::get_if<T>(&m_objects[id]);
    }

    /**
     * Returns the total count of currently allocated heap objects.
     *
     * @return Number of objects stored on the heap.
     */
    [[nodiscard]] size_t size() const noexcept { return m_objects.size(); }

    /**
     * Clears all objects from the heap, freeing their resources.
     */
    void clear() noexcept { m_objects.clear(); }

private:
    /// Storage container for all heap-allocated object variants.
    std::vector<HeapObject> m_objects{};
};

} // namespace ely::vm

