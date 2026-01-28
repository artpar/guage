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

/* Print stack trace */
void debug_print_stack(DebugContext* ctx) {
    printf("\n⟳ Call Stack:\n");

    StackFrame* frame = ctx->stack;
    int depth = 0;

    while (frame != NULL) {
        printf("  %d. %s(", depth, frame->function_name);
        if (frame->args) {
            cell_print(frame->args);
        }
        printf(")\n");

        frame = frame->parent;
        depth++;
    }

    if (depth == 0) {
        printf("  (empty)\n");
    }
}
