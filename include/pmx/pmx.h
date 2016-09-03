/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * pmx.h: interface for exporting postmortem debugging metadata from Node.js
 * programs.
 */

#ifndef	_PMX_H
#define	_PMX_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef uintptr_t pmx_value_t;
typedef struct pmx_stream pmx_stream_t;

/*
 * Note: when adding new error codes, update pmx_errmsg().
 */
typedef enum {
    PMXE_OK,		/* no error */
    PMXE_ENOMEM,	/* memory allocation failure */
    PMXE_EIO,		/* error writing to underlying file stream */
} pmx_error_t;

typedef enum {
    PB_FALSE,
    PB_TRUE
} pmx_boolean_t;

pmx_stream_t *pmx_create_stream(FILE *, FILE *);
void pmx_free(pmx_stream_t *);

pmx_error_t pmx_errno(pmx_stream_t *);
const char *pmx_errmsg(pmx_stream_t *);

void pmx_emit_metadata(pmx_stream_t *, const char *, const char *);
void pmx_emit_node_boolean(pmx_stream_t *, pmx_value_t, pmx_boolean_t,
    pmx_value_t);
void pmx_emit_node_null(pmx_stream_t *, pmx_value_t, pmx_value_t);
void pmx_emit_node_hole(pmx_stream_t *, pmx_value_t, pmx_value_t);
void pmx_emit_node_undefined(pmx_stream_t *, pmx_value_t, pmx_value_t);

void pmx_emit_node_heapnumber(pmx_stream_t *, pmx_value_t, double);
void pmx_emit_node_external(pmx_stream_t *, pmx_value_t, uintptr_t, size_t);
void pmx_emit_node_date(pmx_stream_t *, pmx_value_t, struct timespec *);
void pmx_emit_node_regexp(pmx_stream_t *, pmx_value_t, pmx_value_t,
    pmx_value_t);
void pmx_emit_node_string_flat(pmx_stream_t *, pmx_value_t, size_t,
    pmx_value_t);
void pmx_emit_node_string_cons(pmx_stream_t *, pmx_value_t, size_t,
    pmx_value_t, pmx_value_t);
void pmx_emit_node_string_slice(pmx_stream_t *, pmx_value_t, size_t,
    pmx_value_t, uintptr_t, uintptr_t);

void pmx_emit_string_data(pmx_stream_t *, pmx_value_t, size_t, const uint8_t *);

void pmx_function_start(pmx_stream_t *, pmx_value_t);
void pmx_function_label(pmx_stream_t *, const char *);
void pmx_function_script_name(pmx_stream_t *, const char *);
void pmx_function_position(pmx_stream_t *, const char *);
void pmx_function_done(pmx_stream_t *);

void pmx_closure_start(pmx_stream_t *, pmx_value_t, pmx_value_t);
void pmx_closure_parent(pmx_stream_t *, pmx_value_t);
void pmx_closure_variable(pmx_stream_t *, pmx_value_t, const char *);
void pmx_closure_done(pmx_stream_t *);

void pmx_object_start(pmx_stream_t *, pmx_value_t);
void pmx_object_constructor(pmx_stream_t *, pmx_value_t, pmx_value_t);
void pmx_object_property(pmx_stream_t *, pmx_value_t, const char *);
void pmx_object_done(pmx_stream_t *);

void pmx_array_start(pmx_stream_t *, pmx_value_t);
void pmx_array_element(pmx_stream_t *, pmx_value_t, int, size_t);
void pmx_array_done(pmx_stream_t *);

#endif /* not defined _PMX_H_ */
