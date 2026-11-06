#include <ely/core/chunk.h>

#include <stddef.h>
#include <stdlib.h>

void ely_chunk_init(ElyChunk* chunk) {
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

    // Set all pointers to NULL
    chunk->code = NULL;
    chunk->constants = NULL;
    chunk->lines = NULL;
    chunk->string_constants = NULL;
}

void ely_chunk_free(ElyChunk* chunk) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Free all memory blocks
    free(chunk->code);
    free(chunk->constants);
    free(chunk->lines);

    for (size_t i = 0; i < chunk->string_constants_count; ++i) {
        free(chunk->string_constants[i]);
    }
    free(chunk->string_constants);

    // Init clean up
    ely_chunk_init(chunk);
}

void ely_chunk_write(ElyChunk* chunk, uint8_t byte, size_t line) {
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
            ElyLineInfo* new_lines = (ElyLineInfo*)realloc(chunk->lines, sizeof(ElyLineInfo) * new_capacity);
            assert(new_lines != NULL && "Out of memory, cannot reallocate lines buffer.");
            chunk->lines = new_lines;
            chunk->lines_capacity = new_capacity;
        }

        chunk->lines[chunk->lines_count].offset = (uint32_t)(chunk->code_count - 1);
        chunk->lines[chunk->lines_count].line = (uint32_t)line;
        chunk->lines_count++;
    }
}

ELY_NODISCARD size_t ely_chunk_add_constant(ElyChunk* chunk, ElyValue value) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Check for sizes and reallocate
    if (chunk->constants_count + 1 > chunk->constants_capacity) {
        size_t new_capacity = chunk->constants_capacity < 8 ? 8 : chunk->constants_capacity * 2;
        ElyValue* new_constants = (ElyValue*)realloc(chunk->constants, sizeof(ElyValue) * new_capacity);
        assert(new_constants != NULL && "Out of memory, cannot reallocate constants buffer.");
        chunk->constants = new_constants;
        chunk->constants_capacity = new_capacity;
    }

    size_t index = chunk->constants_count++;
    chunk->constants[index] = value;
    return index;
}

ELY_NODISCARD size_t ely_chunk_add_string_constant(ElyChunk* chunk, const char* str, size_t len) {
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

ELY_NODISCARD size_t ely_chunk_get_line(const ElyChunk* chunk, size_t instruction_offset) {
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
