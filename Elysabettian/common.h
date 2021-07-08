//
//  common.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef common_h
#define common_h

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* NaN boxing optimizations */
#define NAN_BOXING

/* Debug flags */
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXEC

/* GC / VM debug flags */
#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC

/* Max locals count */
#define UINT8_COUNT (UINT8_MAX + 1)

#endif /* common_h */

#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXEC

#undef DEBUG_STRESS_GC
#undef DEBUG_LOG_GC
