/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * pmx_subr.c: general-purpose functions used by postmortem export
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pmx/pmx.h>
#include <libjsonemitter/jsonemitter.h>
#include "pmx_impl.h"

#define	MILLISEC	1000
#define	MICROSEC	1000000

/*
 * Statically-allocated buffer for fatal error messages.
 */
const char *pmx_panichdr = "\npmx_panic:\n";
char pmx_panicstr[512];

static void pmx_node_begin(pmx_stream_t *, pmx_value_t, pmx_nodetype_t);
static void pmx_node_field_jsv(pmx_stream_t *, const char *, pmx_value_t);
static void pmx_node_end(pmx_stream_t *);
static void pmx_emit_oddball(pmx_stream_t *, pmx_value_t, pmx_boolean_t *,
    const char *, pmx_value_t);

/*
 * Lifecycle of a pmx_stream_t
 */
pmx_stream_t *
pmx_create_stream(FILE *outfp, FILE *errfp)
{
	pmx_stream_t *pmxp;

	VERIFY(outfp != NULL);

	pmxp = calloc(1, sizeof (*pmxp));
	if (pmxp == NULL) {
		return (NULL);
	}

	pmxp->pxs_outstream = outfp;
	pmxp->pxs_errstream = errfp;
	pmxp->pxs_jsonout = json_create_stdio(outfp);
	if (pmxp->pxs_jsonout == NULL) {
		pmx_free(pmxp);
		return (NULL);
	}

	/*
	 * XXX This is where we should emit the nodetypes and edgetypes that we
	 * know about (mapping string values to numeric identifiers).
	 */
	pmxp->pxs_state = PMXS_TOP;
	return (pmxp);
}

void
pmx_free(pmx_stream_t *pmxp)
{
	if (pmxp != NULL) {
		json_fini(pmxp->pxs_jsonout);
		free(pmxp);
	}
}


/*
 * Error management: public interfaces
 */

pmx_error_t
pmx_errno(pmx_stream_t *pmxp)
{
	return (pmxp->pxs_error);
}

const char *
pmx_errmsg(pmx_stream_t *pmxp)
{
	if (pmxp->pxs_errmsg[0] != '\0') {
		return (pmxp->pxs_errmsg);
	}

	switch (pmxp->pxs_error) {
	case PMXE_OK:		return ("no error");
	case PMXE_ENOMEM:	return ("not enough space");
	case PMXE_EIO:		return ("i/o error");
	default:		break;
	}

	pmx_panic("unrecognized pmx_error_t: %d\n", pmxp->pxs_error);
}

/*
 * Error management: internal interfaces
 */

void
pmx_set_errno(pmx_stream_t *pmxp, pmx_error_t pmxerr)
{
	pmxp->pxs_error = pmxerr;
	pmxp->pxs_errmsg[0] = '\0';
}

void
pmx_error(pmx_stream_t *pmxp, pmx_error_t pmxerr, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	pmx_verror(pmxp, pmxerr, fmt, args);
	va_end(args);
}

void
pmx_verror(pmx_stream_t *pmxp, pmx_error_t pmxerr, const char *fmt,
    va_list args)
{
	pmxp->pxs_error = pmxerr;
	(void) vsnprintf(pmxp->pxs_errmsg, sizeof (pmxp->pxs_errmsg), fmt,
	    args);
}

void
pmx_warn(pmx_stream_t *pmxp, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	pmx_vwarn(pmxp, fmt, args);
	va_end(args);
}

void
pmx_vwarn(pmx_stream_t *pmxp, const char *fmt, va_list args)
{
	(void) vfprintf(pmxp->pxs_errstream, fmt, args);
	pmxp->pxs_nwarnings++;
}

int
pmx_assfail(const char *condition, const char *file, unsigned int line)
{
	pmx_panic("assertion failed: %s at file %s line %d\n",
	    condition, file, line);
	return (0);
}

void
pmx_panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	pmx_vpanic(fmt, args);
	va_end(args);
}

void
pmx_vpanic(const char *fmt, va_list args)
{
	int nbytes;

	nbytes = vsnprintf(pmx_panicstr, sizeof (pmx_panicstr), fmt, args);
	(void) write(STDERR_FILENO, pmx_panichdr, strlen(pmx_panichdr));
	(void) write(STDERR_FILENO, pmx_panicstr, nbytes);
	abort();
}

/*
 * Assertion helpers
 */
pmx_boolean_t
pmx_cstr_printable(const char *str)
{
	const char *p;

	for (p = str; *p != '\0'; p++) {
		/*
		 * XXX This ought to check for printable characters according to
		 * the C locale.
		 */
		if (!isascii(*p)) {
			return (PB_FALSE);
		}
	}

	return (PB_TRUE);
}

/*
 * Emitter helper functions
 */

static void
pmx_node_begin(pmx_stream_t *pmxp, pmx_value_t ident, pmx_nodetype_t subtype)
{
	VERIFY(pmxp->pxs_state == PMXS_TOP);
	VERIFY(pmxp->pxs_nfields == 0);
	VERIFY(subtype != PMXN_NONE);
	pmxp->pxs_state = PMXS_NODE;
	pmxp->pxs_subtype = subtype;

	json_object_begin(pmxp->pxs_jsonout, NULL);
	json_utf8string(pmxp->pxs_jsonout, "type", "node");
	json_uint64(pmxp->pxs_jsonout, "subtype", subtype);
	json_uint64(pmxp->pxs_jsonout, "ident", ident);
}

static void
pmx_node_end(pmx_stream_t *pmxp)
{
	VERIFY(pmxp->pxs_state == PMXS_NODE);
	json_object_end(pmxp->pxs_jsonout);
	json_newline(pmxp->pxs_jsonout);
	pmxp->pxs_state = PMXS_TOP;
	pmxp->pxs_subtype = PMXN_NONE;
	pmxp->pxs_nfields = 0;
}

static void
pmx_node_field_jsv(pmx_stream_t *pmxp, const char *label, pmx_value_t val)
{
	VERIFY(pmxp->pxs_state == PMXS_NODE);
	VERIFY(strchr(label, '"') == NULL);
	pmxp->pxs_nfields++;
	json_uint64(pmxp->pxs_jsonout, label, val);
}

static void
pmx_emit_oddball(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_boolean_t *emitted,
    const char *internal_label, pmx_value_t label)
{
	if (*emitted) {
		pmx_warn(pmxp, "already emitted value for oddball \"%s\"",
		    internal_label);
	}

	pmx_node_begin(pmxp, jsv, PMXN_ODDBALL);
	pmx_node_field_jsv(pmxp, "name", label);
	pmx_node_end(pmxp);
	*emitted = PB_TRUE;
}

/*
 * Emitters.
 *
 * These will likely be refactored once we add support for multiple backend
 * types.
 */

void
pmx_emit_metadata(pmx_stream_t *pmxp, const char *key, const char *value)
{
	VERIFY(pmx_cstr_printable(key));
	VERIFY(strchr(key, '"') == NULL);
	VERIFY(pmx_cstr_printable(value));
	VERIFY(strchr(value, '"') == NULL);

	json_object_begin(pmxp->pxs_jsonout, NULL);
	json_utf8string(pmxp->pxs_jsonout, "type", "metadata");
	json_utf8string(pmxp->pxs_jsonout, "key", key);
	json_utf8string(pmxp->pxs_jsonout, "value", value);
	json_object_end(pmxp->pxs_jsonout);
	json_newline(pmxp->pxs_jsonout);
	pmxp->pxs_nmetadata++;
}

void
pmx_emit_node_boolean(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_boolean_t val,
    pmx_value_t label)
{
	if (val) {
		pmx_emit_oddball(pmxp, jsv, &pmxp->pxs_emitted_true,
		    "true", label);
	} else {
		pmx_emit_oddball(pmxp, jsv, &pmxp->pxs_emitted_false,
		    "false", label);
	}
}

void
pmx_emit_node_hole(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t label)
{
	pmx_emit_oddball(pmxp, jsv, &pmxp->pxs_emitted_hole, "the_hole", label);
}

void
pmx_emit_node_null(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t label)
{
	pmx_emit_oddball(pmxp, jsv, &pmxp->pxs_emitted_null, "null", label);
}

void
pmx_emit_node_undefined(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t label)
{
	pmx_emit_oddball(pmxp, jsv, &pmxp->pxs_emitted_undefined, "undefined",
	    label);
}

void
pmx_emit_node_heapnumber(pmx_stream_t *pmxp, pmx_value_t jsv, double d)
{
	json_emit_t *jse = pmxp->pxs_jsonout;

	pmx_node_begin(pmxp, jsv, PMXN_HEAPNUMBER);
	json_double(jse, "value", d);
	pmx_node_end(pmxp);
}

void
pmx_emit_node_date(pmx_stream_t *pmxp, pmx_value_t jsv, struct timespec *ts)
{
	uint64_t millis;

	millis = (uint64_t)ts->tv_sec * MILLISEC +
	    (uint64_t)ts->tv_nsec / MICROSEC;
	pmx_node_begin(pmxp, jsv, PMXN_DATE);
	json_uint64(pmxp->pxs_jsonout, "timestamp", millis);
	pmx_node_end(pmxp);
}

void
pmx_emit_node_string_flat(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t len,
    pmx_value_t bytes)
{
	pmx_node_begin(pmxp, jsv, PMXN_STRING_FLAT);
	json_uint64(pmxp->pxs_jsonout, "length", len);
	json_uint64(pmxp->pxs_jsonout, "data", bytes);
	pmx_node_end(pmxp);
}

void
pmx_emit_node_string_cons(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t len,
    pmx_value_t s1, pmx_value_t s2)
{
	pmx_node_begin(pmxp, jsv, PMXN_STRING_CONS);
	json_uint64(pmxp->pxs_jsonout, "length", len);
	json_uint64(pmxp->pxs_jsonout, "s1", s1);
	json_uint64(pmxp->pxs_jsonout, "s1", s2);
	pmx_node_end(pmxp);
}

void
pmx_function_start(pmx_stream_t *pmxp, pmx_value_t jsv)
{
	pmx_node_begin(pmxp, jsv, PMXN_FUNCINFO);
}

void
pmx_function_label(pmx_stream_t *pmxp, pmx_value_t jsv)
{
	VERIFY(pmxp->pxs_subtype == PMXN_FUNCINFO);
	json_uint64(pmxp->pxs_jsonout, "name", jsv);
}

void
pmx_function_script_name(pmx_stream_t *pmxp, pmx_value_t jsv)
{
	VERIFY(pmxp->pxs_subtype == PMXN_FUNCINFO);
	json_uint64(pmxp->pxs_jsonout, "script_name", jsv);
}

void
pmx_function_position(pmx_stream_t *pmxp, pmx_value_t jsv)
{
	VERIFY(pmxp->pxs_subtype == PMXN_FUNCINFO);
	json_uint64(pmxp->pxs_jsonout, "position", jsv);
}

void
pmx_function_done(pmx_stream_t *pmxp)
{
	VERIFY(pmxp->pxs_subtype == PMXN_FUNCINFO);
	pmx_node_end(pmxp);
}


void
pmx_closure_start(pmx_stream_t *pmxp, pmx_value_t jsv, pmx_value_t funcinfo)
{
	pmx_node_begin(pmxp, jsv, PMXN_CLOSURE);
	json_uint64(pmxp->pxs_jsonout, "metadata", funcinfo);
}

void
pmx_closure_parent(pmx_stream_t *pmxp, pmx_value_t parent)
{
	VERIFY(pmxp->pxs_subtype == PMXN_CLOSURE);
	json_uint64(pmxp->pxs_jsonout, "parent", parent);
}

void
pmx_closure_done(pmx_stream_t *pmxp)
{
	VERIFY(pmxp->pxs_subtype == PMXN_CLOSURE);
	pmx_node_end(pmxp);
}


void
pmx_object_start(pmx_stream_t *pmxp, pmx_value_t jsv)
{
	pmx_node_begin(pmxp, jsv, PMXN_OBJECT);
}

void
pmx_object_constructor(pmx_stream_t *pmxp, pmx_value_t cons)
{
	VERIFY(pmxp->pxs_subtype == PMXN_OBJECT);
	json_uint64(pmxp->pxs_jsonout, "constructor", cons);
}

void
pmx_object_done(pmx_stream_t *pmxp)
{
	VERIFY(pmxp->pxs_subtype == PMXN_OBJECT);
	pmx_node_end(pmxp);
}

/* XXX should length here be a value (so it can be a heap number?) */
void
pmx_array(pmx_stream_t *pmxp, pmx_value_t jsv, size_t len)
{
	pmx_node_begin(pmxp, jsv, PMXN_ARRAY);
	json_uint64(pmxp->pxs_jsonout, "length", len);
	pmx_node_end(pmxp);
}

void
pmx_emit_string_data(pmx_stream_t *pmxp, pmx_value_t jsv, size_t sz,
    const uint8_t *bytes)
{
	size_t i;

	/*
	 * XXX This is absolutely awful, on levels:
	 * - one character at a time
	 * - hardcoding the things we need to escape here
	 * - not handling non-ascii
	 * - not handling ASCII control characters
	 *
	 * XXX We need to decide if the exporter should faithfully represent the
	 * core file (e.g., so that an N-byte ASCII string is represented
	 * precisely with those N bytes, even if they're non-ASCII, and the
	 * consumer is expected to validate and detect this), or if the exporter
	 * should attempt to sanitize it (e.g., report a substring, and mark the
	 * string as potentially invalid).  It's a tough call: we don't want to
	 * sanitize too much, since it's important for consumers to understand
	 * details like in-memory representation in order to understand memory
	 * usage, but we don't want every consumer to have to handle every edge
	 * case.
	 *
	 * If the answer is that this is a faithful representation, then this
	 * interface should just emit a bunch of bytes, and we should consider
	 * base64-encoding it (in the spec).  Otherwise, maybe we produce the
	 * best UTF-8 encoding we can and mark the string suspect if that does
	 * not go well.
	 *
	 * XXX JSON appears to have a way to represent strings with arbitrary
	 * unicode characters.  Is it possible to have invalid input, or is
	 * every byte sequence valid?
	 *
	 * XXX This needs to be better-specified in the spec.
	 */
	(void) fprintf(pmxp->pxs_outstream,
	    "{\"type\":\"string\",\"ident\":%" PRIu64 ",\"contents\":\"",
	    (uint64_t)jsv);
	for (i = 0; i < sz; i++) {
		if (!isascii(bytes[i])) {
			pmx_warn(pmxp, "pmx_emit_string_data for 0x%" PRIx64
			    ": skipping unsupported string", (uint64_t)jsv);
			pmxp->pxs_nwarnings++;
			break;
		}

		if (bytes[i] == '"') {
			fputc('\\', pmxp->pxs_outstream);
		}

		(void) fputc(bytes[i], pmxp->pxs_outstream);
	}

	(void) fprintf(pmxp->pxs_outstream, "\"}\n");
}
