/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * pmxemit.c: use libpmx to emit a sample postmortem export.
 *
 * This program should not use private libpmx functions.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <pmx/pmx.h>

#define	EXIT_USAGE 2

int
main(int argc __attribute__((__unused__)),
    char *argv[] __attribute__((__unused__)))
{
	pmx_stream_t *pmxp;
	time_t nowt;
	struct timespec ts;
	struct tm nowtm;
	char nowstr[sizeof ("2016-08-29T00:00:00Z")];

	if (argc != 1) {
		errx(EXIT_USAGE, "no arguments expected");
	}

	pmxp = pmx_create_stream(stdout, stderr);
	if (pmxp == NULL) {
		err(EXIT_FAILURE, "pmx_create_stream");
	}

	(void) time(&nowt);
	(void) gmtime_r(&nowt, &nowtm);
	(void) strftime(nowstr, sizeof (nowstr), "%FT%TZ", &nowtm);

	/*
	 * XXX Compare this to the spec.
	 */
	pmx_emit_metadata(pmxp, "generator", "pmxemit");
	pmx_emit_metadata(pmxp, "generator_version", "1.0.0");
	pmx_emit_metadata(pmxp, "generated_at", nowstr);
	pmx_emit_metadata(pmxp, "version_major", "0");
	pmx_emit_metadata(pmxp, "version_minor", "1");
	pmx_emit_metadata(pmxp, "target_source", "synthetic");

	/*
	 * For this test, we make up an address space:
	 *
	 *	ADDRESS	CONTENTS
	 *	0x0100	string "null"
	 *	0x0200	string "false"
	 *	0x0300	string "true"
	 *	0x0400	string "undefined"
	 *	0x0500	string "the_hole"
	 *	0x0600	string "hello "
	 *	0x0700	string "world"
	 *	0x1000	oddball value: null
	 *	0x2000	oddball value: false
	 *	0x3000	oddball value: true
	 *	0x4000	oddball value: undefined
	 *	0x5000	oddball value: the_hole
	 *	0x6000	heap number with value 10.052016
	 *	0x7000	date with timestamp 1475688184306
	 *	0x8000	flat string of length 6 with contents at 0x600
	 *	0x9000	flat string of length 5 with contents at 0x700
	 *	0xa000	cons string of length 11 from 0x8000 and 0x9000
	 *	0xb000	an array with 3 elements: null, null, 0xa000
	 *	0xc000	an object with null constructor
	 *	0xd000	a chunk of function metadata for a function called
	 *		"hello world" in a script called "world"
	 *	0xe000	a closure for the function defined at 0xd000
	 *	0xf000	an object constructed using the closure at 0xe000
	 */
	pmx_emit_string_data(pmxp, 0x0100, strlen("null"), (uint8_t *)"null");
	pmx_emit_string_data(pmxp, 0x0200, strlen("false"), (uint8_t *)"false");
	pmx_emit_string_data(pmxp, 0x0300, strlen("true"), (uint8_t *)"true");
	pmx_emit_string_data(pmxp, 0x0400, strlen("undefined"),
	    (uint8_t *)"undefined");
	pmx_emit_string_data(pmxp, 0x0500, strlen("the_hole"),
	    (uint8_t *)"the_hole");
	pmx_emit_string_data(pmxp, 0x0600, strlen("hello "),
	    (uint8_t *)"hello ");
	pmx_emit_string_data(pmxp, 0x0700, strlen("world"), (uint8_t *)"world");
	pmx_emit_node_null(pmxp, 0x1000, 0x0100);
	pmx_emit_node_boolean(pmxp, 0x2000, PB_FALSE, 0x0200);
	pmx_emit_node_boolean(pmxp, 0x3000, PB_TRUE, 0x0300);
	pmx_emit_node_undefined(pmxp, 0x4000, 0x0400);
	pmx_emit_node_hole(pmxp, 0x5000, 0x0500);
	pmx_emit_node_heapnumber(pmxp, 0x6000, 10.052016);

	ts.tv_sec = 1475688184;
	ts.tv_nsec = 306000000;
	pmx_emit_node_date(pmxp, 0x7000, &ts);

	pmx_emit_node_string_flat(pmxp, 0x8000, PMX_SMI_VALUE(6), 0x600);
	pmx_emit_node_string_flat(pmxp, 0x9000, PMX_SMI_VALUE(5), 0x700);
	pmx_emit_node_string_cons(pmxp, 0xa000, PMX_SMI_VALUE(11),
	    0x8000, 0x9000);

	pmx_array(pmxp, 0xb000, 3);
	/* TODO */

	pmx_object_start(pmxp, 0xc000);
	pmx_object_constructor(pmxp, 0x1000);
	pmx_object_done(pmxp);

	pmx_function_start(pmxp, 0xd000);
	pmx_function_label(pmxp, 0xa000);
	pmx_function_script_name(pmxp, 0x9000);
	pmx_function_position(pmxp, 100);
	pmx_function_done(pmxp);

	pmx_closure_start(pmxp, 0xe000, 0xd000);
	pmx_closure_parent(pmxp, 0x0100);
	pmx_closure_done(pmxp);

	pmx_object_start(pmxp, 0xf000);
	pmx_object_constructor(pmxp, 0xe000);
	pmx_object_done(pmxp);

	if (pmx_errno(pmxp) != PMXE_OK) {
		errx(EXIT_FAILURE, "%s", pmx_errmsg(pmxp));
	}

	pmx_free(pmxp);
	return (0);
}
