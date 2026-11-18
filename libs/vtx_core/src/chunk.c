#include <vtx/core/chunk.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void vtx_chunk_init(VtxChunk* chunk) {
    assert(chunk != NULL && "Chunk pointer is null.");

    // Set all counters
    chunk->code_capacity = 0;
    chunk->code_count = 0;
    chunk->constants_capacity = 0;
    chunk->constants_count = 0;
    chunk->lines_capacity = 0;
    chunk->lines_count = 0;
    chunk->string_constants_capacity = 0;
    chunk->string_constants_count = 0;
    chunk->function_constants_capacity = 0;
    chunk->function_constants_count = 0;

    // Set all pointers to NULL
    chunk->code = NULL;
    chunk->constants = NULL;
    chunk->lines = NULL;
    chunk->string_constants = NULL;
    chunk->function_constants = NULL;
}

void vtx_chunk_free(VtxChunk* chunk) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Free all memory blocks
    free(chunk->code);
    free(chunk->constants);
    free(chunk->lines);

    for (size_t i = 0; i < chunk->string_constants_count; ++i) {
        free(chunk->string_constants[i]);
    }
    free(chunk->string_constants);

    for (size_t i = 0; i < chunk->function_constants_count; ++i) {
        if (chunk->function_constants[i].chunk != NULL) {
            vtx_chunk_free(chunk->function_constants[i].chunk);
            free(chunk->function_constants[i].chunk);
        }
        free(chunk->function_constants[i].name);
    }
    free(chunk->function_constants);

    // Init clean up
    vtx_chunk_init(chunk);
}

static char* dup_cstr(const char* src) {
    if (src == NULL) return NULL;
    const size_t len = strlen(src);
    char* copy = (char*)malloc(len + 1);
    assert(copy != NULL && "Out of memory, cannot duplicate string.");
    memcpy(copy, src, len + 1);
    return copy;
}

void vtx_chunk_copy(VtxChunk* dst, const VtxChunk* src) {
    assert(dst != NULL && "Destination chunk pointer is NULL.");
    assert(src != NULL && "Source chunk pointer is NULL.");
    if (dst == src) return;

    vtx_chunk_init(dst);

    if (src->code_count > 0) {
        dst->code = (uint8_t*)malloc(sizeof(uint8_t) * src->code_count);
        assert(dst->code != NULL && "Out of memory, cannot copy code buffer.");
        memcpy(dst->code, src->code, sizeof(uint8_t) * src->code_count);
        dst->code_count = src->code_count;
        dst->code_capacity = src->code_count;
    }

    if (src->constants_count > 0) {
        dst->constants = (VtxValue*)malloc(sizeof(VtxValue) * src->constants_count);
        assert(dst->constants != NULL && "Out of memory, cannot copy constants buffer.");
        memcpy(dst->constants, src->constants, sizeof(VtxValue) * src->constants_count);
        dst->constants_count = src->constants_count;
        dst->constants_capacity = src->constants_count;
    }

    if (src->lines_count > 0) {
        dst->lines = (VtxLineInfo*)malloc(sizeof(VtxLineInfo) * src->lines_count);
        assert(dst->lines != NULL && "Out of memory, cannot copy lines buffer.");
        memcpy(dst->lines, src->lines, sizeof(VtxLineInfo) * src->lines_count);
        dst->lines_count = src->lines_count;
        dst->lines_capacity = src->lines_count;
    }

    if (src->string_constants_count > 0) {
        dst->string_constants = (char**)malloc(sizeof(char*) * src->string_constants_count);
        assert(dst->string_constants != NULL && "Out of memory, cannot copy string constants buffer.");
        dst->string_constants_count = src->string_constants_count;
        dst->string_constants_capacity = src->string_constants_count;
        for (size_t i = 0; i < src->string_constants_count; ++i) {
            dst->string_constants[i] = dup_cstr(src->string_constants[i]);
        }
    }

    if (src->function_constants_count > 0) {
        dst->function_constants = (VtxFunctionDescriptor*)malloc(
            sizeof(VtxFunctionDescriptor) * src->function_constants_count
        );
        assert(dst->function_constants != NULL && "Out of memory, cannot copy function constants buffer.");
        dst->function_constants_count = src->function_constants_count;
        dst->function_constants_capacity = src->function_constants_count;
        for (size_t i = 0; i < src->function_constants_count; ++i) {
            const VtxFunctionDescriptor* src_fd = &src->function_constants[i];
            VtxFunctionDescriptor* dst_fd = &dst->function_constants[i];
            dst_fd->arity = src_fd->arity;
            dst_fd->name = dup_cstr(src_fd->name);
            if (src_fd->chunk != NULL) {
                dst_fd->chunk = (VtxChunk*)malloc(sizeof(VtxChunk));
                assert(dst_fd->chunk != NULL && "Out of memory, cannot allocate copied function chunk.");
                vtx_chunk_copy(dst_fd->chunk, src_fd->chunk);
            } else {
                dst_fd->chunk = NULL;
            }
        }
    }
}

void vtx_chunk_write(VtxChunk* chunk, uint8_t byte, size_t line) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Dynamic resizing of code buffer
    if (chunk->code_count + 1 > chunk->code_capacity) {
        size_t new_capacity = chunk->code_capacity < 8 ? 8 : chunk->code_capacity * 2;
        uint8_t* new_code = (uint8_t*)realloc(chunk->code, sizeof(uint8_t) * new_capacity);
        assert(new_code != NULL && "Out of memory, cannot reallocate code buffer.");
        chunk->code = new_code;
        chunk->code_capacity = new_capacity;
    }

    // Write on the buffer
    chunk->code[chunk->code_count++] = byte;

    // Record the line
    if (chunk->lines_count == 0 || chunk->lines[chunk->lines_count - 1].line != (uint32_t)line) {
        if (chunk->lines_count + 1 > chunk->lines_capacity) {
            size_t new_capacity = chunk->lines_capacity < 8 ? 8 : chunk->lines_capacity * 2;
            VtxLineInfo* new_lines = (VtxLineInfo*)realloc(chunk->lines, sizeof(VtxLineInfo) * new_capacity);
            assert(new_lines != NULL && "Out of memory, cannot reallocate lines buffer.");
            chunk->lines = new_lines;
            chunk->lines_capacity = new_capacity;
        }

        chunk->lines[chunk->lines_count].offset = (uint32_t)(chunk->code_count - 1);
        chunk->lines[chunk->lines_count].line = (uint32_t)line;
        chunk->lines_count++;
    }
}

VTX_NODISCARD size_t vtx_chunk_add_constant(VtxChunk* chunk, VtxValue value) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Check for sizes and reallocate
    if (chunk->constants_count + 1 > chunk->constants_capacity) {
        size_t new_capacity = chunk->constants_capacity < 8 ? 8 : chunk->constants_capacity * 2;
        VtxValue* new_constants = (VtxValue*)realloc(chunk->constants, sizeof(VtxValue) * new_capacity);
        assert(new_constants != NULL && "Out of memory, cannot reallocate constants buffer.");
        chunk->constants = new_constants;
        chunk->constants_capacity = new_capacity;
    }

    size_t index = chunk->constants_count++;
    chunk->constants[index] = value;
    return index;
}

VTX_NODISCARD size_t vtx_chunk_add_function_constant(VtxChunk* chunk, VtxFunctionDescriptor fd) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    if (chunk->function_constants_count + 1 > chunk->function_constants_capacity) {
        size_t new_capacity = chunk->function_constants_capacity < 8 ? 8 : chunk->function_constants_capacity * 2;
        VtxFunctionDescriptor* new_fd = (VtxFunctionDescriptor*)realloc(chunk->function_constants, sizeof(VtxFunctionDescriptor) * new_capacity);
        assert(new_fd != NULL && "Out of memory, cannot reallocate function constants buffer.");
        chunk->function_constants = new_fd;
        chunk->function_constants_capacity = new_capacity;
    }

    size_t index = chunk->function_constants_count++;
    chunk->function_constants[index] = fd;

    return index;
}

VTX_NODISCARD size_t vtx_chunk_add_string_constant(VtxChunk* chunk, const char* str, size_t len) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    if (chunk->string_constants_count + 1 > chunk->string_constants_capacity) {
        size_t new_capacity = chunk->string_constants_capacity < 8 ? 8 : chunk->string_constants_capacity * 2;
        char** new_str_const = (char**)realloc(chunk->string_constants, sizeof(char*) * new_capacity);
        assert(new_str_const != NULL && "Out of memory, cannot reallocate strings buffer.");
        chunk->string_constants = new_str_const;
        chunk->string_constants_capacity = new_capacity;
    }

    size_t index = chunk->string_constants_count++;
    chunk->string_constants[index] = (char*)malloc(sizeof(char) * (len + 1));
    assert(chunk->string_constants[index] != NULL && "Out of memory, cannot allocate new string constant.");
    memcpy(chunk->string_constants[index], str, len * sizeof(char));
    chunk->string_constants[index][len] = '\0';

    return index;
}

VTX_NODISCARD size_t vtx_chunk_get_line(const VtxChunk* chunk, size_t instruction_offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    if (chunk->lines_count == 0) return 0;

    size_t low = 0;
    size_t high = chunk->lines_count - 1;
    size_t result = chunk->lines[0].line;

    while (low <= high) {
        size_t mid = low + (high - low) / 2;

        if (chunk->lines[mid].offset <= (uint32_t)instruction_offset) {
            result = chunk->lines[mid].line;
            low = mid + 1;
        } else {
            if (mid == 0) break;
            high = mid - 1;
        }
    }

    return result;
}
