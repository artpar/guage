/* log.h — Compile-time log level macros
 *
 * Usage:
 *   LOG_TRACE("msg %d", val);   // Ultra-verbose, hot paths
 *   LOG_DEBUG("msg %d", val);   // Development debugging
 *   LOG_INFO("msg %d", val);    // Operational events
 *   LOG_WARN("msg %d", val);    // Unexpected but recoverable
 *   LOG_ERROR("msg %d", val);   // Failures
 *
 * Build flags:
 *   -DLOG_LEVEL=0   ALL (trace+debug+info+warn+error)
 *   -DLOG_LEVEL=1   DEBUG+ (no trace)
 *   -DLOG_LEVEL=2   INFO+  (default)
 *   -DLOG_LEVEL=3   WARN+
 *   -DLOG_LEVEL=4   ERROR only
 *   -DLOG_LEVEL=5   SILENT (all noop)
 *
 * All macros compile to nothing when below the level threshold.
 * No runtime cost. No branches. Dead code elimination.
 */
#ifndef GUAGE_LOG_H
#define GUAGE_LOG_H

#include <stdio.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 2  /* Default: INFO+ */
#endif

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_SILENT 5

/* Core macro — all levels route through this.
 * fflush ensures correct cross-thread ordering in log files. */
#define LOG_EMIT(tag, fmt, ...) \
    do { fprintf(stderr, "[" tag "] %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); fflush(stderr); } while(0)

#if LOG_LEVEL <= LOG_LEVEL_TRACE
#define LOG_TRACE(fmt, ...) LOG_EMIT("TRACE", fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) LOG_EMIT("DEBUG", fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG_EMIT("INFO ", fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) LOG_EMIT("WARN ", fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG_EMIT("ERROR", fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#endif

#endif /* GUAGE_LOG_H */
