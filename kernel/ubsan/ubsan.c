/*
 * Copyright (c) 2014, 2015, 2024 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ubsan/ubsan.c
 * Undefined behavior sanitizer runtime support.
 */

#include <kernel.h>
#include <scheduler.h>
#include <vga_buffer.h>

#define SCRAM_UNDEFINED_BEHAVIOR 3
struct scram_undefined_behavior {
    const char *filename;
    unsigned long line;
    unsigned long column;
    const char *violation;
};

[[noreturn]] void scram(int, const struct scram_undefined_behavior *info);

void scram(const int event, const struct scram_undefined_behavior *info)
{
    if (scheduler_get_current_thread()) {
        printf(KYEL "\nCurrent thread:" KWHT " %s (%d)\n",
                scheduler_get_current_process()->file_name,
                scheduler_get_current_process()->pid);
    }
    printf(KYEL "Event:" KWHT " %d\n", event);
    printf(KYEL "File:" KWHT " %s\n", info->filename);
    printf(KYEL "Line:" KWHT " %lu\n", info->line);
    printf(KYEL "Column:" KWHT " %lu\n", info->column);
    printf(KYEL "Violation:" KWHT " %s\n", info->violation);
    panic("Undefined behavior detected");

    __builtin_unreachable();
}

struct ubsan_source_location {
    const char *filename;
    uint32_t line;
    uint32_t column;
};

struct ubsan_type_descriptor {
    uint16_t type_kind;
    uint16_t type_info;
    char type_name[];
};

typedef uintptr_t ubsan_value_handle_t;

static const struct ubsan_source_location unknown_location = {
    "<unknown file>",
    0,
    0,
};

[[noreturn]] static void ubsan_abort(const struct ubsan_source_location *location, const char *violation)
{
    if (!location || !location->filename)
        location = &unknown_location;

    struct scram_undefined_behavior info;
    info.filename  = location->filename;
    info.line      = location->line;
    info.column    = location->column;
    info.violation = violation;
    scram(SCRAM_UNDEFINED_BEHAVIOR, &info);
}

#define ABORT_VARIANT(name, params, call)                                                                              \
    [[noreturn]] void __ubsan_handle_##name##_abort params                                                             \
    {                                                                                                                  \
        __ubsan_handle_##name call;                                                                                    \
        __builtin_unreachable();                                                                                       \
    }

#define ABORT_VARIANT_VP(name) ABORT_VARIANT(name, (void *a), (a))
#define ABORT_VARIANT_VP_VP(name) ABORT_VARIANT(name, (void *a, void *b), (a, b))
#define ABORT_VARIANT_VP_IP(name) ABORT_VARIANT(name, (void *a, intptr_t b), (a, b))
#define ABORT_VARIANT_VP_VP_VP(name) ABORT_VARIANT(name, (void *a, void *b, void *c), (a, b, c))
#define ABORT_VARIANT_VP_VP_UP(name) ABORT_VARIANT(name, (void *a, void *b, uintptr_t c), (a, b, c))
#define ABORT_VARIANT_VP_VP_VP_VP(name) ABORT_VARIANT(name, (void *a, void *b, void *c, void *d), (a, b, c, d))
#define ABORT_VARIANT_VP_VP_UP_VP(name) ABORT_VARIANT(name, (void *a, void *b, uintptr_t c, void *d), (a, b, c, d))

struct ubsan_type_mismatch_v1_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
    unsigned char log_alignment;
    unsigned char type_check_kind;
};

void __ubsan_handle_type_mismatch_v1(void *data_raw, void *pointer_raw)
{
    struct ubsan_type_mismatch_v1_data *data = (struct ubsan_type_mismatch_v1_data *)data_raw;
    ubsan_value_handle_t pointer             = (ubsan_value_handle_t)pointer_raw;
    uintptr_t alignment                      = (uintptr_t)1UL << data->log_alignment;
    const char *violation                    = "type mismatch";
    if (!pointer)
        violation = "null pointer access";
    else if (alignment && (pointer & (alignment - 1)))
        violation = "unaligned access";
    ubsan_abort(&data->location, violation);
}

ABORT_VARIANT_VP_VP(type_mismatch_v1)

struct ubsan_alignment_assumption_data {
    struct ubsan_source_location location;
    struct ubsan_source_location assumption_location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_alignment_assumption(void *data_raw, void *pointer_raw, void *alignment_raw, void *offset_raw)
{
    struct ubsan_alignment_assumption_data *data = (struct ubsan_alignment_assumption_data *)data_raw;
    ubsan_value_handle_t pointer                 = (ubsan_value_handle_t)pointer_raw;
    ubsan_value_handle_t alignment               = (ubsan_value_handle_t)alignment_raw;
    ubsan_value_handle_t offset                  = (ubsan_value_handle_t)offset_raw;
    (void)pointer;
    (void)alignment;
    (void)offset;
    const char *violation = "alignment assumption failed";
    ubsan_abort(&data->location, violation);
}

ABORT_VARIANT_VP_VP_VP_VP(alignment_assumption)

struct ubsan_overflow_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_add_overflow(void *data_raw, void *lhs_raw, void *rhs_raw)
{
    struct ubsan_overflow_data *data = (struct ubsan_overflow_data *)data_raw;
    ubsan_value_handle_t lhs         = (ubsan_value_handle_t)lhs_raw;
    ubsan_value_handle_t rhs         = (ubsan_value_handle_t)rhs_raw;
    (void)lhs;
    (void)rhs;
    ubsan_abort(&data->location, "addition overflow");
}

ABORT_VARIANT_VP_VP_VP(add_overflow)

void __ubsan_handle_sub_overflow(void *data_raw, void *lhs_raw, void *rhs_raw)
{
    struct ubsan_overflow_data *data = (struct ubsan_overflow_data *)data_raw;
    ubsan_value_handle_t lhs         = (ubsan_value_handle_t)lhs_raw;
    ubsan_value_handle_t rhs         = (ubsan_value_handle_t)rhs_raw;
    (void)lhs;
    (void)rhs;
    ubsan_abort(&data->location, "subtraction overflow");
}

ABORT_VARIANT_VP_VP_VP(sub_overflow)

void __ubsan_handle_mul_overflow(void *data_raw, void *lhs_raw, void *rhs_raw)
{
    struct ubsan_overflow_data *data = (struct ubsan_overflow_data *)data_raw;
    ubsan_value_handle_t lhs         = (ubsan_value_handle_t)lhs_raw;
    ubsan_value_handle_t rhs         = (ubsan_value_handle_t)rhs_raw;
    (void)lhs;
    (void)rhs;
    ubsan_abort(&data->location, "multiplication overflow");
}

ABORT_VARIANT_VP_VP_VP(mul_overflow)

void __ubsan_handle_negate_overflow(void *data_raw, void *old_value_raw)
{
    struct ubsan_overflow_data *data = (struct ubsan_overflow_data *)data_raw;
    ubsan_value_handle_t old_value   = (ubsan_value_handle_t)old_value_raw;
    (void)old_value;
    ubsan_abort(&data->location, "negation overflow");
}

ABORT_VARIANT_VP_VP(negate_overflow)

void __ubsan_handle_divrem_overflow(void *data_raw, void *lhs_raw, void *rhs_raw)
{
    struct ubsan_overflow_data *data = (struct ubsan_overflow_data *)data_raw;
    ubsan_value_handle_t lhs         = (ubsan_value_handle_t)lhs_raw;
    ubsan_value_handle_t rhs         = (ubsan_value_handle_t)rhs_raw;
    (void)lhs;
    (void)rhs;
    ubsan_abort(&data->location, "division remainder overflow");
}

ABORT_VARIANT_VP_VP_VP(divrem_overflow)

struct ubsan_shift_out_of_bounds_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *lhs_type;
    struct ubsan_type_descriptor *rhs_type;
};

void __ubsan_handle_shift_out_of_bounds(void *data_raw, void *lhs_raw, void *rhs_raw)
{
    struct ubsan_shift_out_of_bounds_data *data = (struct ubsan_shift_out_of_bounds_data *)data_raw;
    ubsan_value_handle_t lhs                    = (ubsan_value_handle_t)lhs_raw;
    ubsan_value_handle_t rhs                    = (ubsan_value_handle_t)rhs_raw;
    (void)lhs;
    (void)rhs;
    ubsan_abort(&data->location, "shift out of bounds");
}

ABORT_VARIANT_VP_VP_VP(shift_out_of_bounds)

struct ubsan_out_of_bounds_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *array_type;
    struct ubsan_type_descriptor *index_type;
};

void __ubsan_handle_out_of_bounds(void *data_raw, void *index_raw)
{
    struct ubsan_out_of_bounds_data *data = (struct ubsan_out_of_bounds_data *)data_raw;
    ubsan_value_handle_t index            = (ubsan_value_handle_t)index_raw;
    (void)index;
    ubsan_abort(&data->location, "out of bounds");
}

ABORT_VARIANT_VP_VP(out_of_bounds)

struct ubsan_unreachable_data {
    struct ubsan_source_location location;
};

[[noreturn]] void __ubsan_handle_builtin_unreachable(void *data_raw) // NOLINT(*-reserved-identifier)
{
    struct ubsan_unreachable_data *data = (struct ubsan_unreachable_data *)data_raw;
    ubsan_abort(&data->location, "reached unreachable");
}

[[noreturn]] void __ubsan_handle_missing_return(void *data_raw) // NOLINT(*-reserved-identifier)
{
    struct ubsan_unreachable_data *data = (struct ubsan_unreachable_data *)data_raw;
    ubsan_abort(&data->location, "missing return");
}

struct ubsan_vla_bound_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_vla_bound_not_positive(void *data_raw, void *bound_raw)
{
    struct ubsan_vla_bound_data *data = (struct ubsan_vla_bound_data *)data_raw;
    ubsan_value_handle_t bound        = (ubsan_value_handle_t)bound_raw;
    (void)bound;
    ubsan_abort(&data->location, "negative variable array length");
}

ABORT_VARIANT_VP_VP(vla_bound_not_positive)

struct ubsan_float_cast_overflow_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *from_type;
    struct ubsan_type_descriptor *to_type;
};

void __ubsan_handle_float_cast_overflow(void *data_raw, void *from_raw)
{
    struct ubsan_float_cast_overflow_data *data = (struct ubsan_float_cast_overflow_data *)data_raw;
    ubsan_value_handle_t from                   = (ubsan_value_handle_t)from_raw;
    (void)from;
#if !(defined(__GNUC__) && __GNUC__ < 6)
    ubsan_abort(&data->location, "float cast overflow");
#else
    ubsan_abort(((void)data, &unknown_location), "float cast overflow");
#endif
}

ABORT_VARIANT_VP_VP(float_cast_overflow)

struct ubsan_invalid_value_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_load_invalid_value(void *data_raw, void *value_raw)
{
    struct ubsan_invalid_value_data *data = (struct ubsan_invalid_value_data *)data_raw;
    ubsan_value_handle_t value            = (ubsan_value_handle_t)value_raw;
    (void)value;
    ubsan_abort(&data->location, "invalid value load");
}

ABORT_VARIANT_VP_VP(load_invalid_value)

struct ubsan_implicit_conversion_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
    struct ubsan_type_descriptor *from_type;
    struct ubsan_type_descriptor *to_type;
    unsigned char kind;
};

void __ubsan_handle_implicit_conversion(void *data_raw, void *src_raw, void *dst_raw)
{
    struct ubsan_implicit_conversion_data *data = (struct ubsan_implicit_conversion_data *)data_raw;
    ubsan_value_handle_t src                    = (ubsan_value_handle_t)src_raw;
    ubsan_value_handle_t dst                    = (ubsan_value_handle_t)dst_raw;
    (void)src;
    (void)dst;
    ubsan_abort(&data->location, "implicit conversion");
}

ABORT_VARIANT_VP_VP_VP(implicit_conversion)

struct ubsan_invalid_builtin_data {
    struct ubsan_source_location location;
    unsigned char kind;
};

void __ubsan_handle_invalid_builtin(void *data_raw)
{
    struct ubsan_invalid_builtin_data *data = (struct ubsan_invalid_builtin_data *)data_raw;
    ubsan_abort(&data->location, "invalid builtin");
}

ABORT_VARIANT_VP(invalid_builtin)

struct ubsan_invalid_objc_cast_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *expected_type;
};

void __ubsan_handle_invalid_objc_cast(void *data_raw, void *pointer_raw)
{
    struct ubsan_invalid_builtin_data *data = (struct ubsan_invalid_builtin_data *)data_raw;
    ubsan_value_handle_t pointer            = (ubsan_value_handle_t)pointer_raw;
    (void)pointer;
    ubsan_abort(&data->location, "invalid objc cast");
}

ABORT_VARIANT_VP_VP(invalid_objc_cast)

struct ubsan_function_type_mismatch_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_function_type_mismatch(void *data_raw, void *value_raw)
{
    struct ubsan_function_type_mismatch_data *data = (struct ubsan_function_type_mismatch_data *)data_raw;
    ubsan_value_handle_t value                     = (ubsan_value_handle_t)value_raw;
    (void)value;
    ubsan_abort(&data->location, "function type mismatch");
}

ABORT_VARIANT_VP_VP(function_type_mismatch)

struct ubsan_nonnull_return_v1_data {
    struct ubsan_source_location attr_location;
};

void __ubsan_handle_nonnull_return_v1(void *data_raw, void *location_raw)
{
    struct ubsan_nonnull_return_data *data = (struct ubsan_nonnull_return_data *)data_raw;
    (void)data;
    struct ubsan_source_location *location = location_raw;
    ubsan_abort(location, "null return");
}

void __ubsan_handle_nullability_return_v1(void *data_raw, void *location_raw)
{
    struct ubsan_nonnull_return_data *data = (struct ubsan_nonnull_return_data *)data_raw;
    (void)data;
    struct ubsan_source_location *location = location_raw;
    ubsan_abort(location, "nullability return");
}

ABORT_VARIANT_VP_VP(nonnull_return_v1)
ABORT_VARIANT_VP_VP(nullability_return_v1)

struct ubsan_nonnull_arg_data {
    struct ubsan_source_location location;
    struct ubsan_source_location attr_location;
    int arg_index;
};

void __ubsan_handle_nonnull_arg(void *data_raw)
{
    struct ubsan_nonnull_arg_data *data = (struct ubsan_nonnull_arg_data *)data_raw;
#if !(defined(__GNUC__) && __GNUC__ < 6)
#else
    ubsan_value_handle_t index = (ubsan_value_handle_t)index_raw;
    (void)index;
#endif
    ubsan_abort(&data->location, "null argument");
}

void __ubsan_handle_nullability_arg(void *data_raw)
{
    struct ubsan_nonnull_arg_data *data = (struct ubsan_nonnull_arg_data *)data_raw;
    ubsan_abort(&data->location, "nullability argument");
}

ABORT_VARIANT_VP(nonnull_arg)
ABORT_VARIANT_VP(nullability_arg)

struct ubsan_pointer_overflow_data {
    struct ubsan_source_location location;
};

void __ubsan_handle_pointer_overflow(void *data_raw, void *base_raw, void *result_raw)
{
    struct ubsan_pointer_overflow_data *data = (struct ubsan_pointer_overflow_data *)data_raw;
    ubsan_value_handle_t base                = (ubsan_value_handle_t)base_raw;
    ubsan_value_handle_t result              = (ubsan_value_handle_t)result_raw;
    (void)base;
    (void)result;
    ubsan_abort(&data->location, "pointer overflow");
}

ABORT_VARIANT_VP_VP_VP(pointer_overflow)

struct ubsan_cfi_bad_icall_data {
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_cfi_bad_icall(void *data_raw, void *value_raw)
{
    struct ubsan_cfi_bad_icall_data *data = (struct ubsan_cfi_bad_icall_data *)data_raw;
    ubsan_value_handle_t value            = (ubsan_value_handle_t)value_raw;
    (void)value;
    ubsan_abort(&data->location, "control flow integrity check failure during indirect call");
}

ABORT_VARIANT_VP_VP(cfi_bad_icall)

struct ubsan_cfi_check_fail_data {
    unsigned char check_kind;
    struct ubsan_source_location location;
    struct ubsan_type_descriptor *type;
};

void __ubsan_handle_cfi_check_fail(void *data_raw, void *function_raw, uintptr_t vtable_is_valid)
{
    struct ubsan_cfi_check_fail_data *data = (struct ubsan_cfi_check_fail_data *)data_raw;
    ubsan_value_handle_t function          = (ubsan_value_handle_t)function_raw;
    (void)function;
    (void)vtable_is_valid;
    ubsan_abort(&data->location, "control flow integrity check failure");
}

ABORT_VARIANT_VP_VP_UP(cfi_check_fail)

void __ubsan_handle_cfi_bad_type(void *data_raw, void *function_raw, uintptr_t vtable_is_valid,
                                 const void *report_options_raw)
{
    struct ubsan_cfi_check_fail_data *data = (struct ubsan_cfi_check_fail_data *)data_raw;
    ubsan_value_handle_t function          = (ubsan_value_handle_t)function_raw;
    (void)function;
    (void)vtable_is_valid;
    (void)report_options_raw;
    ubsan_abort(&data->location, "control flow integrity bad type");
}

ABORT_VARIANT_VP_VP_UP_VP(cfi_bad_type)
