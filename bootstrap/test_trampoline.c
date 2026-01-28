/* Test program for trampoline data structures */
#include "trampoline.h"
#include "cell.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_stack_create_destroy() {
    printf("Test: stack_create_destroy\n");
    EvalStack* stack = stack_create();
    assert(stack != NULL);
    assert(stack_is_empty(stack));
    assert(stack_size(stack) == 0);
    stack_destroy(stack);
    printf("  ✓ PASS\n");
}

void test_stack_push_pop() {
    printf("Test: stack_push_pop\n");
    EvalStack* stack = stack_create();

    Cell* expr = cell_number(42.0);
    Cell* env = cell_nil();
    StackFrame* frame = frame_create_eval(expr, env);

    stack_push(stack, frame);
    assert(stack_size(stack) == 1);
    assert(!stack_is_empty(stack));

    StackFrame* popped = stack_pop(stack);
    assert(popped == frame);
    assert(stack_is_empty(stack));

    frame_destroy(popped);
    stack_destroy(stack);

    /* Clean up created cells */
    cell_release(expr);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_stack_multiple_frames() {
    printf("Test: stack_multiple_frames\n");
    EvalStack* stack = stack_create();

    /* Push 10 frames */
    Cell* expr = cell_number(1.0);
    Cell* env = cell_nil();

    for (int i = 0; i < 10; i++) {
        StackFrame* frame = frame_create_eval(expr, env);
        stack_push(stack, frame);
    }

    assert(stack_size(stack) == 10);

    /* Pop all frames */
    for (int i = 0; i < 10; i++) {
        StackFrame* frame = stack_pop(stack);
        frame_destroy(frame);
    }

    assert(stack_is_empty(stack));

    stack_destroy(stack);
    cell_release(expr);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_stack_growth() {
    printf("Test: stack_growth (dynamic resize)\n");
    EvalStack* stack = stack_create();

    Cell* expr = cell_number(1.0);
    Cell* env = cell_nil();

    /* Push 200 frames (should cause multiple resizes) */
    for (int i = 0; i < 200; i++) {
        StackFrame* frame = frame_create_eval(expr, env);
        stack_push(stack, frame);
    }

    assert(stack_size(stack) == 200);

    /* Pop all frames */
    for (int i = 0; i < 200; i++) {
        StackFrame* frame = stack_pop(stack);
        frame_destroy(frame);
    }

    stack_destroy(stack);
    cell_release(expr);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_stack_peek() {
    printf("Test: stack_peek\n");
    EvalStack* stack = stack_create();

    Cell* expr = cell_number(42.0);
    Cell* env = cell_nil();
    StackFrame* frame = frame_create_eval(expr, env);

    stack_push(stack, frame);

    /* Peek should return same frame without removing */
    StackFrame* peeked = stack_peek(stack);
    assert(peeked == frame);
    assert(stack_size(stack) == 1);

    /* Pop should also return same frame */
    StackFrame* popped = stack_pop(stack);
    assert(popped == frame);
    assert(stack_is_empty(stack));

    frame_destroy(popped);
    stack_destroy(stack);
    cell_release(expr);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_stack_result() {
    printf("Test: stack_result\n");
    EvalStack* stack = stack_create();

    Cell* result = cell_number(100.0);
    stack_set_result(stack, result);

    Cell* retrieved = stack_get_result(stack);
    assert(retrieved == result);

    stack_destroy(stack);
    cell_release(result);

    printf("  ✓ PASS\n");
}

void test_frame_create_eval() {
    printf("Test: frame_create_eval\n");
    Cell* expr = cell_number(42.0);
    Cell* env = cell_nil();

    StackFrame* frame = frame_create_eval(expr, env);
    assert(frame != NULL);
    assert(frame->state == EVAL_EXPR);
    assert(frame->expr == expr);
    assert(frame->env == env);

    frame_destroy(frame);
    cell_release(expr);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_frame_create_apply() {
    printf("Test: frame_create_apply\n");
    Cell* func = cell_symbol("add");
    Cell* args = cell_cons(cell_number(1.0), cell_cons(cell_number(2.0), cell_nil()));
    Cell* env = cell_nil();

    StackFrame* frame = frame_create_apply(func, args, env);
    assert(frame != NULL);
    assert(frame->state == EVAL_APPLY);
    assert(frame->func == func);

    frame_destroy(frame);
    cell_release(func);
    cell_release(args);
    cell_release(env);

    printf("  ✓ PASS\n");
}

void test_frame_create_return() {
    printf("Test: frame_create_return\n");
    Cell* value = cell_number(123.0);

    StackFrame* frame = frame_create_return(value);
    assert(frame != NULL);
    assert(frame->state == EVAL_RETURN);
    assert(frame->value == value);

    frame_destroy(frame);
    cell_release(value);

    printf("  ✓ PASS\n");
}

void test_frame_state_name() {
    printf("Test: frame_state_name\n");
    assert(strcmp(frame_state_name(EVAL_EXPR), "EVAL_EXPR") == 0);
    assert(strcmp(frame_state_name(EVAL_APPLY), "EVAL_APPLY") == 0);
    assert(strcmp(frame_state_name(EVAL_ARGS), "EVAL_ARGS") == 0);
    assert(strcmp(frame_state_name(EVAL_RETURN), "EVAL_RETURN") == 0);
    assert(strcmp(frame_state_name(EVAL_IF), "EVAL_IF") == 0);
    assert(strcmp(frame_state_name(EVAL_DEFINE), "EVAL_DEFINE") == 0);
    assert(strcmp(frame_state_name(EVAL_QUOTE), "EVAL_QUOTE") == 0);

    printf("  ✓ PASS\n");
}

int main() {
    printf("Running Trampoline Data Structure Tests\n");
    printf("========================================\n\n");

    /* Initialize cell system */
    /* Note: Might need initialization if cell system requires it */

    test_stack_create_destroy();
    test_stack_push_pop();
    test_stack_multiple_frames();
    test_stack_growth();
    test_stack_peek();
    test_stack_result();
    test_frame_create_eval();
    test_frame_create_apply();
    test_frame_create_return();
    test_frame_state_name();

    printf("\n========================================\n");
    printf("All tests passed! ✓\n");

    return 0;
}
