#ifndef VTX_COMPILER_SCANNER_H
#define VTX_COMPILER_SCANNER_H

#include <vtx/core/value.h>
#include "token.h"

/**
 * Fast, zero-allocation lexical scanner maintaining character cursor positions
 * directly over a source code string buffer.
 */
typedef struct VtxScanner {
    /// Pointer to the beginning of the lexeme currently being scanned.
    const char* start;
    /// Current character read cursor within the source stream.
    const char* current;
    /// 1-based source code line currently being tracked.
    uint32_t line;
} VtxScanner;

/**
 * Initialises an VtxScanner instance over a null-terminated UTF-8 source string.
 *
 * @param scanner Pointer to the VtxScanner structure to initialise.
 * @param source Null-terminated C string containing the source code.
 */
void vtx_scanner_init(VtxScanner* scanner, const char* source);

/**
 * Consumes characters from the source buffer and yields the next lexical token.
 *
 * Emits VTX_TT_EOF when the terminating null character is reached, or
 * VTX_TT_ERROR if an unexpected character or unterminated literal is encountered.
 *
 * @param scanner Pointer to the active VtxScanner.
 * @return Constructed VtxToken referencing the scanned lexeme slice.
 */
VTX_NODISCARD VtxToken vtx_scanner_scan_token(VtxScanner* scanner);

#endif
