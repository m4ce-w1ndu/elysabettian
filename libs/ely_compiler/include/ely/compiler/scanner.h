#ifndef ELY_COMPILER_SCANNER_H
#define ELY_COMPILER_SCANNER_H

#include <ely/core/value.h>
#include "token.h"

/**
 * Fast, zero-allocation lexical scanner maintaining character cursor positions
 * directly over a source code string buffer.
 */
typedef struct ElyScanner {
    /// Pointer to the beginning of the lexeme currently being scanned.
    const char* start;
    /// Current character read cursor within the source stream.
    const char* current;
    /// 1-based source code line currently being tracked.
    uint32_t line;
} ElyScanner;

/**
 * Initialises an ElyScanner instance over a null-terminated UTF-8 source string.
 *
 * @param scanner Pointer to the ElyScanner structure to initialise.
 * @param source Null-terminated C string containing the source code.
 */
void ely_scanner_init(ElyScanner* scanner, const char* source);

/**
 * Consumes characters from the source buffer and yields the next lexical token.
 *
 * Emits ELY_TT_EOF when the terminating null character is reached, or
 * ELY_TT_ERROR if an unexpected character or unterminated literal is encountered.
 *
 * @param scanner Pointer to the active ElyScanner.
 * @return Constructed ElyToken referencing the scanned lexeme slice.
 */
ELY_NODISCARD ElyToken ely_scanner_scan_token(ElyScanner* scanner);

#endif
