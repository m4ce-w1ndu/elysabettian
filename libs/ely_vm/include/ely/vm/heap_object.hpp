#pragma once

#include <ely/core/chunk.hpp>
#include <ely/core/value.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace ely::vm {

/**
 * Represents a method bound to a receiver object instance.
 */
struct ObjBoundMethod {
    /// The target instance acting as the `this`/receiver value.
    core::Value receiver;
    /// Heap identifier of the compiled method or closure.
    size_t method_id{0};

    /**
     * Constructs a bound method wrapper.
     *
     * @param recv Receiver instance Value.
     * @param method_id Heap index of the target method.
     */
    explicit ObjBoundMethod(core::Value recv, size_t method_id) noexcept
        : receiver(recv), method_id(method_id) {}
};

/**
 * Represents a class blueprint with a table of method references.
 */
struct ObjClass {
    /// Display name of the class.
    std::string name;
    /// Table mapping method name hash to function/closure heap ID.
    std::unordered_map<uint32_t, size_t> methods{};

    /**
     * Constructs a class definition with the given name.
     *
     * @param name Name of the class.
     */
    explicit ObjClass(std::string name)
        : name(std::move(name)) {}
};

/**
 * Represents an instantiated class object with dynamic instance fields.
 */
struct ObjInstance {
    /// Heap identifier of the class definition this instance belongs to.
    size_t class_id{0};
    /// Dynamic field storage mapping property name hashes to Values.
    std::unordered_map<uint32_t, core::Value> fields{};

    /**
     * Constructs a new instance of the specified class.
     *
     * @param class_id Identifier of the originating ObjClass.
     */
    explicit ObjInstance(size_t class_id) noexcept
        : class_id(class_id) {}
};

/**
 * Represents a compiled function object tracked on the heap.
 *
 * Stores executable bytecode chunk, arity, upvalue counts, and profiling
 * metrics for JIT tier-up compilation.
 */
struct ObjFunction {
    /// Expected number of parameters.
    size_t arity{0};
    /// Number of upvalues captured by closures wrapping this function.
    size_t upvalue_count{0};
    /// Shared reference to the bytecode Chunk executing this function.
    std::shared_ptr<core::Chunk> chunk;
    /// Optional debug name of the function.
    std::optional<std::string> name{std::nullopt};
    /// Execution counter used by the tier-up policy to trigger JIT compilation.
    size_t invocation_count{0};
    /// Entry pointer to native code compiled by the AsmJit backend (nullptr if interpreted).
    const uint8_t* jit_entry{nullptr};

    /**
     * Constructs a function object.
     *
     * @param arity Parameter count.
     * @param name Optional function identifier.
     */
    explicit ObjFunction(size_t arity = 0, std::optional<std::string> name = std::nullopt)
        : arity(arity), chunk(std::make_shared<core::Chunk>()), name(std::move(name)) {}
};

/**
 * Tracks a variable captured by a closure across execution scopes.
 *
 * An upvalue begins in an "open" state pointing to a stack slot. When the
 * enclosing stack frame exits, the variable is "closed" by moving the Value
 * into the `closed` member.
 */
struct ObjUpvalue {
    /// Stack slot offset where the local variable resides while open.
    size_t location{0};
    /// Stored value once the upvalue is closed and no longer on the stack.
    std::optional<core::Value> closed{std::nullopt};

    /**
     * Constructs an open upvalue pointing to a VM stack location.
     *
     * @param location Stack slot offset of the captured variable.
     */
    explicit ObjUpvalue(size_t location) noexcept
        : location(location) {}

    /**
     * Checks if this upvalue is still pointing to the active VM stack.
     *
     * @return True if open (on the stack), false if closed.
     */
    [[nodiscard]] bool is_open() const noexcept {
        return !closed.has_value();
    }
};

/**
 * Represents a first-class closure pairing a function with its captured upvalues.
 */
struct ObjClosure {
    /// Heap identifier of the underlying ObjFunction.
    size_t function_id{0};
    /// Heap identifiers of the ObjUpvalues captured by this closure instance.
    std::vector<size_t> upvalues{};

    /**
     * Constructs a closure wrapping the given function.
     *
     * @param func_id Heap identifier of the function.
     */
    explicit ObjClosure(size_t func_id) noexcept : function_id(func_id) {}
};

/**
 * Sum type representing any dynamically allocated object managed by the VM heap.
 *
 * Each variant is addressable via a 48-bit index encoded inside a NaN-boxed Value.
 */
using HeapObject = std::variant<
    std::string,               // String literal or dynamic string
    std::vector<core::Value>,  // Dynamically-sized array of Values
    ObjFunction,               // Compiled function prototype
    ObjClosure,                // First-class closure with upvalues
    ObjUpvalue,                // Captured local variable
    ObjClass,                  // Class blueprint
    ObjInstance,               // Class instance with properties
    ObjBoundMethod             // Method bound to a receiver
>;

} // namespace ely::vm

