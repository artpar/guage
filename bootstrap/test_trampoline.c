/* Test program for trampoline data structures */
#include "trampoline.h"
#include "cell.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Stub for parse() - not needed for these tests but required for linking */
Cell* parse(const char* input) {
    (void)input;
    return cell_error("not-implemented", cell_symbol("parse-stub"));
}

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

/* ========== Handler Tests ========== */

void test_handle_eval_return() {
    printf("Test: handle_eval_return\n");
    EvalStack* stack = stack_create();

    /* Test 1: Return with empty stack - should set final result */
    Cell* value = cell_number(42.0);
    StackFrame* ret_frame = frame_create_return(value);

    handle_eval_return(ret_frame, stack);

    Cell* result = stack_get_result(stack);
    assert(result == value);
    assert(cell_get_number(result) == 42.0);

    frame_destroy(ret_frame);
    cell_release(value);
    stack_destroy(stack);

    /* Test 2: Return with parent frame - should store in parent's value field */
    stack = stack_create();
    Cell* parent_expr = cell_symbol("x");
    Cell* parent_env = cell_nil();
    StackFrame* parent = frame_create_eval(parent_expr, parent_env);
    stack_push(stack, parent);

    Cell* child_value = cell_number(99.0);
    StackFrame* child_ret = frame_create_return(child_value);

    handle_eval_return(child_ret, stack);

    /* Parent should have the value now */
    assert(parent->value == child_value);
    assert(cell_get_number(parent->value) == 99.0);

    frame_destroy(child_ret);
    cell_release(child_value);
    cell_release(parent_expr);
    cell_release(parent_env);
    stack_destroy(stack);

    printf("  ✓ PASS\n");
}

void test_handle_eval_quote() {
    printf("Test: handle_eval_quote\n");
    EvalStack* stack = stack_create();

    Cell* expr = cell_symbol("foo");
    StackFrame* quote_frame = frame_create_quote(expr);

    handle_eval_quote(quote_frame, stack);

    /* Should push a return frame with the quoted expression */
    assert(!stack_is_empty(stack));
    StackFrame* ret_frame = stack_pop(stack);
    assert(ret_frame->state == EVAL_RETURN);
    assert(ret_frame->value == expr);

    frame_destroy(ret_frame);
    frame_destroy(quote_frame);
    cell_release(expr);
    stack_destroy(stack);

    printf("  ✓ PASS\n");
}

void test_handle_eval_expr_atoms() {
    printf("Test: handle_eval_expr (atoms)\n");
    EvalStack* stack = stack_create();

    /* Test number */
    Cell* num = cell_number(42.0);
    StackFrame* frame = frame_create_eval(num, cell_nil());
    handle_eval_expr(frame, stack);

    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(cell_is_number(ret->value));
    assert(cell_get_number(ret->value) == 42.0);
    frame_destroy(ret);
    frame_destroy(frame);
    cell_release(num);

    /* Test boolean */
    Cell* bool_val = cell_bool(true);
    frame = frame_create_eval(bool_val, cell_nil());
    handle_eval_expr(frame, stack);

    ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(cell_is_bool(ret->value));
    assert(cell_get_bool(ret->value) == true);
    frame_destroy(ret);
    frame_destroy(frame);
    cell_release(bool_val);

    /* Test nil */
    Cell* nil_val = cell_nil();
    frame = frame_create_eval(nil_val, cell_nil());
    handle_eval_expr(frame, stack);

    ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(cell_is_nil(ret->value));
    frame_destroy(ret);
    frame_destroy(frame);
    cell_release(nil_val);

    stack_destroy(stack);
    printf("  ✓ PASS\n");
}

void test_handle_eval_expr_keyword() {
    printf("Test: handle_eval_expr (keyword)\n");
    EvalStack* stack = stack_create();

    /* Keywords (symbols starting with :) are self-evaluating */
    Cell* keyword = cell_symbol(":foo");
    StackFrame* frame = frame_create_eval(keyword, cell_nil());

    handle_eval_expr(frame, stack);

    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(ret->value == keyword);
    assert(cell_is_symbol(ret->value));

    frame_destroy(ret);
    frame_destroy(frame);
    cell_release(keyword);
    stack_destroy(stack);

    printf("  ✓ PASS\n");
}

void test_handle_eval_if() {
    printf("Test: handle_eval_if\n");
    EvalStack* stack = stack_create();

    Cell* then_branch = cell_number(1.0);
    Cell* else_branch = cell_number(2.0);
    Cell* env = cell_nil();

    /* Test true condition */
    Cell* true_cond = cell_bool(true);
    StackFrame* if_frame = frame_create_if(true_cond, then_branch, else_branch, env);

    /* First call - should evaluate condition */
    handle_eval_if(if_frame, stack);
    assert(!stack_is_empty(stack));

    /* Simulate condition evaluation returning true */
    StackFrame* eval_frame = stack_pop(stack);
    frame_destroy(eval_frame);
    if_frame->value = true_cond;
    cell_retain(true_cond);

    /* Second call - should choose then branch */
    handle_eval_if(if_frame, stack);
    eval_frame = stack_pop(stack);
    assert(eval_frame->state == EVAL_EXPR);
    assert(eval_frame->expr == then_branch);

    frame_destroy(eval_frame);
    frame_destroy(if_frame);
    cell_release(true_cond);
    cell_release(then_branch);
    cell_release(else_branch);
    cell_release(env);
    stack_destroy(stack);

    printf("  ✓ PASS\n");
}

int main() {
    printf("Running Trampoline Data Structure Tests\n");
    printf("========================================\n\n");

    /* Initialize cell system */
    /* Note: Might need initialization if cell system requires it */

    /* Data structure tests */
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

    /* Handler tests */
    printf("\nHandler Tests\n");
    printf("-------------\n");
    test_handle_eval_return();
    test_handle_eval_quote();
    test_handle_eval_expr_atoms();
    test_handle_eval_expr_keyword();
    test_handle_eval_if();

    printf("\n========================================\n");
    printf("All tests passed! ✓\n");

    return 0;
}
