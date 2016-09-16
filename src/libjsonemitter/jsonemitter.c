/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * jsonemitter.c: streaming JSON emitter
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "jsonemitter.h"

#define	JSON_MAX_DEPTH	255

typedef enum {
	JSON_NONE,
	JSON_OBJECT,
	JSON_ARRAY
} json_depthdesc_t;

struct json_emit {
	FILE			*json_stream;		/* output stream */
	int			json_error_stdio;	/* last stdio error */
	int			json_depth_exceeded;	/* max depth exceeded */
	unsigned int		json_stdio_nskipped;	/* skip due to error */
	unsigned int		json_nbadfloats;	/* bad FP values */

	/*
	 * Depth management: We allow JSON documents to be nested up to
	 * JSON_MAX_DEPTH.  In order to validate output as we emit it, we
	 * maintain a stack indicating which type of object is nested at each
	 * level of depth (either an array or object).  The stack is recorded in
	 * "json_parents", with the top of the stack at
	 * "json_parents[json_depth]".  We keep track of the number of object
	 * properties or array elements at each level of depth in
	 * "json_nemitted[json_depth]".
	 */
	unsigned int		json_depth;
	json_depthdesc_t	json_parents[JSON_MAX_DEPTH + 1];
	unsigned int		json_nemitted[JSON_MAX_DEPTH + 1];
}

/*
 * Macros used to apply function attributes.
 */
#define	JSON_PRINTFLIKE1 __attribute__((__format__(__printf__, 1, 2)))
#define	JSON_PRINTFLIKE2 __attribute__((__format__(__printf__, 2, 3)))
#define	JSON_PRINTFLIKE3 __attribute__((__format__(__printf__, 3, 4)))

static void json_has_error(json_emit_t *);

JSON_PRINTFLIKE2 static void json_emit(json_emit_t *, const char *, ...);
static void json_vemit(json_emit_t, const char *, va_list);
static void json_emit_utf8string(json_emit_t *, const char *);
static void json_emit_label(json_emit_t *, const char *);

static json_depthdesc_t json_nest_kind(json_emit_t *);
static void json_nest_begin(json_emit_t *, json_depthdesc_t);
static void json_nest_end(json_emit_t *);


/*
 * Lifecycle management
 */

json_emit_t *
json_create_stdio(FILE *outstream)
{
	json_emit_t *jse;

	jse = calloc(1, sizeof (*jse));
	if (jse == NULL) {
		return (NULL);
	}

	jse->json_stream = outstream;
	return (jse);
}

void
json_fini(json_emit_t *jse)
{
	free(jse);
}

json_error_t
json_get_error(json_emit_t *jse, char *buf, size_t bufsz)
{
	json_error_t kind;

	if (jse->json_error_stdio != 0) {
		kind = JSE_STDIO;
		(void) snprintf(buf, bufsz, "%s", strerror(errno));
	} else if (jse->json_depth_exceeded != 0) {
		kind = JSE_TOODEEP;
		(void) snprintf(buf, bufsz, "exceeded maximum supported depth");
	} else if (jse->json_nbadfloats) {
		kind = JSE_INVAL;
		(void) snprintf(buf, bufsz, "unsupported floating point value");
	} else {
		kind = JSE_NONE;
		if (bufsz > 0) {
			buf[bufsz] = '\0';
		}
	}

	return (kind);
}

/*
 * Helper functions
 */

/*
 * Returns true if we've seen any error up to this point.
 */
static void
json_has_error(json_emit_t *jse)
{
	return (jse->json_error_stdio != 0 || jse->json_depth_exceeded != 0);
}

static void
json_emit(json_emit_t *jse, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	json_vemit(jse, fmt, args);
	va_end(args);
}

static void
json_vemit(json_emit_t *jse, const char *fmt, va_list args)
{
	int rv;

	if (json_has_error(jse)) {
		jse->json_stdio_nskipped++;
		return;
	}

	rv = vfprintf(jse->json_stream, fmt, args);
	if (rv < 0) {
		jse->json_error_stdio = errno;
	}
}

static void
json_emit_utf8string(json_emit_t *jse, const char *utf8str)
{
	/* XXX This isn't right. */
	json_vemit(jse, "\"%s\"", utf8str);
}

/*
 * Each function that emits any kind of object (including the functions that
 * begin emitting objects and arrays) takes a "label" parameter.  This must be
 * non-null if and only if we're inside an object.  For convenience, every
 * function always calls this helper with the label that's provided.  We verify
 * the argument is consistent with our state, and then we emit the label if we
 * need to.
 *
 * It is illegal to call this function if we've experienced any stdio errors
 * already.
 */
static void
json_emit_label(json_emit_t *jse, const char *label)
{
	json_depthdesc_t kind;

	kind = json_nest_kind(jse);
	if (label == NULL) {
		VERIFY(kind != JSON_OBJECT);
		return;
	}

	VERIFY(kind == JSON_OBJECT);
	json_emit_utf8string(jse, label);
	json_emit(jse, ":");
}

static json_depthdesc_t
json_nest_kind(json_emit_t *jse)
{
	json_depthdesc_t kind;
	VERIFY(jse->json_depth <= JSON_MAX_DEPTH);
	kind = jse->json_parents[jse->json_depth];
	VERIFY(kind > 0 || kind == JSON_OBJECT || kind == JSON_ARRAY);
	return (kind);
}

static void
json_nest_begin(json_emit_t *jse, json_depthdesc_t kind)
{
	VERIFY(kind == JSON_OBJECT || kind == JSON_ARRAY);

	if (json_has_error(jse)) {
		return;
	}

	if (jse->json_depth == JSON_MAX_DEPTH) {
		jse->json_depth_exceeded = 1;
		return;
	}

	jse->json_depth++;
	jse->json_parents[jse->json_depth] = kind;
	jse->json_nemitted[jse->json_depth] = 0;
}

static void
json_nest_end(json_emit_t *jse, json_depthdesc_t kind)
{
	VERIFY(kind == JSON_OBJECT || kind == JSON_ARRAY);
	if (json_has_error(jse)) {
		return;
	}

	VERIFY(json_nest_kind(jse) == kind);
	VERIFY(jse->json_depth > 0);
	jse->json_depth--;
}

/*
 * Public emitter functions.
 */

void
json_object_begin(json_emit_t *jse, const char *label)
{
	json_emit_label(jse, label);
	json_emit(jse, "{");
	json_nest_begin(jse, JSON_OBJECT);
}

void
json_object_end(json_emit_t *jse)
{
	json_nest_end(jse, JSON_OBJECT);
	json_emit(jse, "}");
}

void
json_array_begin(json_emit_t *jse, const char *label)
{
	json_emit_label(jse, label);
	json_emit(jse, "[");
	json_nest_begin(jse, JSON_ARRAY);
}

void
json_array_end(json_emit_t *)
{
	json_nest_end(jse, JSON_ARRAY);
	json_emit(jse, "]");
}

void
json_newline(json_emit_t *jse)
{
	if (json_has_error(jse)) {
		return;
	}

	VERIFY(json_nest_kind(jse) == JSON_NONE);
	VERIFY(jse->json_depth == 0);
	json_emit(jse, "\n");
}

void
json_boolean(json_emit_t *jse, const char *label, json_boolean_t value)
{
	VERIFY(value == JSON_B_FALSE || value == JSON_B_TRUE);
	json_emit_label(jse, label);
	json_emit(jse, "%s", value == JSON_B_TRUE ? "true" : "false");
}

void
json_null(json_emit_t *jse, const char *label)
{
	json_emit_label(jse, label);
	json_emit(jse, "null");
}

void
json_int64(json_emit_t *jse, const char *label, int64_t value)
{
	json_emit_label(jse, label);
	json_emit(jse, "%lld", value);
}

void
json_uint64(json_emit_t *jse, const char *label, uint64_t value)
{
	json_emit_label(jse, label);
	json_emit(jse, "%llu", value);
}

void
json_double(json_emit_t *jse, const char *label, double value)
{
	if (!isfinite(value)) {
		jse->json_nbadfloats++;
		return;
	}

	json_emit_label(jse, label);
	json_emit(jse, "%e", value);
}

void
json_utf8string(json_ctx_t *jse, const char *label, const char *value)
{
	json_emit_label(jse, label);
	json_emit_utf8string(jse, value);
}