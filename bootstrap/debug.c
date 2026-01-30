#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Initialize debug context */
DebugContext* debug_context_new(void) {
    DebugContext* ctx = (DebugContext*)malloc(sizeof(DebugContext));
    ctx->stack = NULL;
    ctx->trace_enabled = false;
    ctx->break_on_error = false;
    ctx->breakpoint_count = 0;
    return ctx;
}

/* Free debug context */
void debug_context_free(DebugContext* ctx) {
    /* Free remaining stack frames */
    while (ctx->stack != NULL) {
        debug_pop_frame(ctx);
    }
    free(ctx);
}

/* Push stack frame */
void debug_push_frame(DebugContext* ctx, const char* name, Cell* args) {
    StackFrame* frame = (StackFrame*)malloc(sizeof(StackFrame));
    frame->function_name = name ? strdup(name) : strdup("<anonymous>");
    frame->args = args;
    if (args) cell_retain(args);
    frame->file = NULL;
    frame->line = 0;
    frame->parent = ctx->stack;
    ctx->stack = frame;
}

/* Pop stack frame */
void debug_pop_frame(DebugContext* ctx) {
    if (ctx->stack == NULL) return;

    StackFrame* frame = ctx->stack;
    ctx->stack = frame->parent;

    free((void*)frame->function_name);
    if (frame->args) cell_release(frame->args);
    free(frame);
}

/* Print stack trace — enhanced with file/line and box-drawing */
void debug_print_stack(DebugContext* ctx) {
    fprintf(stderr, "\n  Call stack (most recent first):\n");

    StackFrame* frame = ctx->stack;
    int depth = 0;

    /* Count frames for box drawing */
    StackFrame* counter = ctx->stack;
    int total = 0;
    while (counter) { total++; counter = counter->parent; }

    while (frame != NULL) {
        /* Box drawing connector */
        const char* connector;
        if (depth == 0 && frame->parent == NULL) {
            connector = "─";  /* Single frame */
        } else if (depth == 0) {
            connector = "┌";  /* Top */
        } else if (frame->parent == NULL) {
            connector = "└";  /* Bottom */
        } else {
            connector = "├";  /* Middle */
        }

        fprintf(stderr, "  %s─ %s(", connector, frame->function_name);
        if (frame->args) {
            cell_print(frame->args);
        }
        fprintf(stderr, ")");

        /* Show file:line if available */
        if (frame->file) {
            fprintf(stderr, "  %s:%d", frame->file, frame->line);
        }
        fprintf(stderr, "\n");

        frame = frame->parent;
        depth++;
    }

    if (depth == 0) {
        fprintf(stderr, "  (empty)\n");
    }
}
