#pragma once

#include <ely/core/chunk.hpp>
#include <ely/core/value.hpp>
#include <ely/stdlib/native.hpp>
#include "call_frame.hpp"
#include "heap.hpp"
#include "heap_object.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ely::vm {

/**
 * Expresses the result of an execution: the execution could stop at
 * compile time (syntax errors, etc..) or at runtime (logical error
 * that generates an exception, etc...).
 */
enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

/**
 * Virtual machine responsible for executing compiled Elysabettian bytecode chunks.
 *
 * Implements a stack-based runtime with call frame management, managed object heap,
 * lexical closure and upvalue resolution, and dynamic native standard library integration.
 */
class VM {
public:
    /**
     * Initializes a new virtual machine instance pre-allocating evaluation stack
     * and call frame storage.
     */
    VM() {
        m_stack.reserve(kStackInitialCapacity);
        m_frames.reserve(kMaxFrames);
    }

    /**
     * Interprets a compiled bytecode chunk by setting up the top-level script frame.
     *
     * @param chunk Compiled bytecode chunk to execute.
     * @return Execution outcome (Ok, CompileError, or RuntimeError).
     */
    [[nodiscard]] InterpretResult interpret(const core::Chunk& chunk);

    /**
     * Compiles and executes an Elysabettian source code string.
     *
     * @param source Raw source script text.
     * @return Execution outcome.
     */
    [[nodiscard]] InterpretResult interpret(std::string_view source);

    /**
     * Pushes a NaN-boxed Value onto the evaluation stack.
     *
     * @param value The value to push.
     */
    void push(core::Value value);

    /**
     * Pops and returns the top value from the evaluation stack.
     *
     * @return The popped Value.
     */
    [[nodiscard]] core::Value pop();

    /**
     * Peeks at a value on the stack at a given offset from the top without removing it.
     *
     * @param distance 0-based distance from top (0 = top element).
     * @return The peeked Value.
     */
    [[nodiscard]] core::Value peek(size_t distance) const;

    /**
     * Clears the evaluation stack, call frames, and active upvalues.
     */
    void reset_stack() noexcept;

    /**
     * Allocates a heap object and returns its NaN-boxed object Value handle.
     *
     * @param obj HeapObject variant to emplace into the managed heap.
     * @return NaN-boxed object Value pointing to the new heap entity.
     */
    [[nodiscard]] core::Value allocate_object(HeapObject obj);

    /**
     * Retrieves a read-only reference to a heap object referenced by a Value handle.
     *
     * @param value NaN-boxed Value referencing a heap object.
     * @return Read-only reference to the managed HeapObject.
     */
    [[nodiscard]] const HeapObject& get_object(core::Value value) const;

    /**
     * Retrieves a mutable reference to a heap object referenced by a Value handle.
     *
     * @param value NaN-boxed Value referencing a heap object.
     * @return Mutable reference to the managed HeapObject.
     */
    [[nodiscard]] HeapObject& get_object_mut(core::Value value);

    /**
     * Binds a native callable function to a global identifier.
     *
     * @param name_id Identifier index of the native symbol.
     * @param native Callable native wrapper.
     */
    void define_native(uint32_t name_id, stdlib::CallableNative native);

    /**
     * Converts a Value to a human-readable display string representation.
     *
     * @param value Target Value to format.
     * @return String representation of the value.
     */
    [[nodiscard]] std::string format_value(core::Value value) const;

private:
    /**
     * Core bytecode execution and instruction dispatch loop.
     *
     * @return Execution result of the top-level script or active frame.
     */
    InterpretResult run();

    /**
     * Recursively loads string constants and compiled functions onto the heap
     * and links their handles into chunk constants.
     *
     * @param chunk Bytecode chunk to load and patch.
     */
    void load_chunk_objects(core::Chunk& chunk);

    /**
     * Captures a local variable at the given stack slot into an open upvalue.
     *
     * @param location VM stack slot index.
     * @return Heap identifier of the open or existing ObjUpvalue.
     */
    [[nodiscard]] size_t capture_upvalue(size_t location);

    /**
     * Closes all open upvalues pointing to stack slots at or above `slot`.
     *
     * @param slot Threshold stack slot offset.
     */
    void close_upvalue(size_t slot);

    /**
     * Reads the current runtime Value referenced by a closure's upvalue.
     *
     * @param closure_id Heap identifier of the closure.
     * @param slot Index into the closure's upvalue table.
     * @return Evaluated Value or std::nullopt on error.
     */
    [[nodiscard]] std::optional<core::Value> get_upvalue_value(size_t closure_id, size_t slot) const;

    /**
     * Mutates the value referenced by a closure's upvalue.
     *
     * @param closure_id Heap identifier of the closure.
     * @param slot Index into the closure's upvalue table.
     * @param value New Value to store.
     * @return True on success, false otherwise.
     */
    bool set_upvalue_value(size_t closure_id, size_t slot, core::Value value);

    /// Evaluation stack for operand values and local variables.
    std::vector<core::Value> m_stack{};

    /// Active call frame stack tracking execution state.
    std::vector<CallFrame> m_frames{};

    /// Managed object heap storing dynamic entities (strings, arrays, functions, classes).
    Heap m_heap{};

    /// Identifiers of currently open upvalues pointing to stack slots.
    std::vector<size_t> m_open_upvalues{};

    /// Global variable table mapping name identifiers to Values.
    std::unordered_map<uint32_t, core::Value> m_globals{};

    /// Native functions table mapping name identifiers to native callables.
    std::unordered_map<uint32_t, stdlib::CallableNative> m_natives{};

    /// Maximum call stack depth before triggering a stack overflow runtime error.
    static constexpr size_t kMaxFrames = 64;

    /// Initial capacity pre-allocated for the operand evaluation stack.
    static constexpr size_t kStackInitialCapacity = 256;
};

} // namespace ely::vm
