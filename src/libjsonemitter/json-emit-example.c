/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * json-emit-example.c: use libjsonemitter to emit a sample JSON object
 *
 * This program should not use private jsonemitter functions.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsonemitter.h"

static int jsx_example_coverage(json_emit_t *);
static int jsx_example_maxdepth(json_emit_t *);
static int jsx_example_toodeep(json_emit_t *);

/* XXX invalid floating point, ... */
struct {
    const char	*jsx_name;
    int 	(*jsx_func)(json_emit_t *);
} json_examples[] = { {
	.jsx_name = "coverage",
	.jsx_func = jsx_example_coverage
}, {
	.jsx_name = "max depth",
	.jsx_func = jsx_example_maxdepth
}, {
	.jsx_name = "max depth",
	.jsx_func = jsx_example_toodeep
} };

int
main(int argc __attribute__((__unused__)),
    char *argv[] __attribute__((__unused__)))
{
	json_emit_t *jse;
	char errbuf[256];
	int i, nexamples;

	nexamples = sizeof (json_examples) / sizeof (json_examples[0]);
	for (i = 0; i < nexamples; i++) {
		(void) fprintf(stderr, "example: %s\n",
		    json_examples[i].jsx_name);
		jse = json_create_stdio(stdout);
		if (jse == NULL) {
			err(EXIT_FAILURE, "json_create_stream");
		}

		if (json_examples[i].jsx_func(jse) != 0) {
			warnx("jsonemit: example function failed");
		}

		if (json_get_error(jse, errbuf, sizeof (errbuf)) != JSE_NONE) {
			warnx("jsonemit: %s", errbuf);
		}

		(void) printf("\n");
		json_fini(jse);
	}

	return (0);
}

static int
jsx_example_coverage(json_emit_t *jse)
{
	json_object_begin(jse, NULL);

	json_object_begin(jse, "empty object");
	json_object_end(jse);

	json_object_begin(jse, "object with one property");
	json_null(jse, "a_null");
	json_object_end(jse);

	json_object_begin(jse, "object with object first");
	json_object_begin(jse, "an_object");
	json_object_end(jse);
	json_null(jse, "a_null");
	json_object_end(jse);

	json_object_begin(jse, "object with object last");
	json_null(jse, "a_null");
	json_object_begin(jse, "an_object");
	json_object_end(jse);
	json_object_end(jse);

	json_array_begin(jse, "empty array");
	json_array_end(jse);

	json_array_begin(jse, "non-empty array");
	json_null(jse, NULL);
	json_int64(jse, NULL, 1);
	json_int64(jse, NULL, 5);
	json_int64(jse, NULL, 9);
	json_array_end(jse);

	json_array_begin(jse, "one-element primitive array");
	json_null(jse, NULL);
	json_array_end(jse);

	json_array_begin(jse, "one-element non-primitive array");
	json_object_begin(jse, NULL);
	json_object_end(jse);
	json_array_end(jse);

	json_array_begin(jse, "array with object first");
	json_object_begin(jse, NULL);
	json_object_end(jse);
	json_null(jse, NULL);
	json_array_end(jse);

	json_array_begin(jse, "array with object last");
	json_null(jse, NULL);
	json_object_begin(jse, NULL);
	json_object_end(jse);
	json_array_end(jse);

	json_int64(jse, "int64: max value", INT64_MAX);
	json_int64(jse, "int64: min value", INT64_MIN);
	json_uint64(jse, "unt64: max value", UINT64_MAX);
	json_uint64(jse, "unt64: min value", 0);
	json_double(jse, "double: 0", 0.0);
	json_double(jse, "double: ordinary positive value", 3.7);
	json_double(jse, "double: ordinary negative value", -3.7);
	json_double(jse, "double: large value", 4.56e123);
	json_double(jse, "double: tiny value", 4.56e-123);
	json_double(jse, "double: precise value", 1.2345678901234567890e123);
	json_boolean(jse, "boolean: true", JSON_B_TRUE);
	json_boolean(jse, "boolean: false", JSON_B_FALSE);
	json_utf8string(jse, "string: empty", "");
	json_utf8string(jse, "string: non-empty", "bump!");
	/* XXX add utf8 literals */
	json_utf8string(jse, "string: special values",
	    "newline\ntab\treturn\rspace quote\"squote'backslash\\");

	json_object_end(jse);
	json_newline(jse);
	return (0);
}

static int
jsx_example_maxdepth(json_emit_t *jse)
{
	int i, maxdepth;

	maxdepth = 255; /* See JSON_MAX_DEPTH, which is private. */
	for (i = 0; i < maxdepth; i++) {
		if (i % 2 == 1) {
			json_array_begin(jse, "p");
		} else {
			json_object_begin(jse, NULL);
		}
	}

	for (i = maxdepth - 1; i >= 0; i--) {
		if (i % 2 == 1) {
			json_array_end(jse);
		} else {
			json_object_end(jse);
		}
	}

	return (0);
}

static int
jsx_example_toodeep(json_emit_t *jse)
{
	int i, maxdepth;

	maxdepth = 256; /* More than JSON_MAX_DEPTH, which is private. */
	for (i = 0; i < maxdepth; i++) {
		if (i % 2 == 1) {
			json_array_begin(jse, "p");
		} else {
			json_object_begin(jse, NULL);
		}
	}

	for (i = maxdepth - 1; i >= 0; i--) {
		if (i % 2 == 1) {
			json_array_end(jse);
		} else {
			json_object_end(jse);
		}
	}

	return (0);
}
