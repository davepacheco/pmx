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

#include <pmx/pmx.h>

#define	EXIT_USAGE 2

int
main(int argc __attribute__((__unused__)),
    char *argv[] __attribute__((__unused__)))
{
	pmx_stream_t *pmxp;
	time_t nowt;
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
	 *	0x1000	oddball value: null
	 *	0x2000	oddball value: false
	 *	0x3000	oddball value: true
	 *	0x4000	oddball value: undefined
	 *	0x5000	oddball value: the_hole
	 */
	pmx_emit_string_data(pmxp, 0x0100, strlen("null"), (uint8_t *)"null");
	pmx_emit_string_data(pmxp, 0x0200, strlen("false"), (uint8_t *)"false");
	pmx_emit_string_data(pmxp, 0x0300, strlen("true"), (uint8_t *)"true");
	pmx_emit_string_data(pmxp, 0x0400, strlen("undefined"),
	    (uint8_t *)"undefined");
	pmx_emit_string_data(pmxp, 0x0500, strlen("the_hole"),
	    (uint8_t *)"the_hole");
	pmx_emit_node_null(pmxp, 0x1000, 0x0100);
	pmx_emit_node_boolean(pmxp, 0x2000, PB_FALSE, 0x0200);
	pmx_emit_node_boolean(pmxp, 0x3000, PB_TRUE, 0x0300);
	pmx_emit_node_undefined(pmxp, 0x4000, 0x0400);
	pmx_emit_node_hole(pmxp, 0x5000, 0x0500);

	if (pmx_errno(pmxp) != PMXE_OK) {
		errx(EXIT_FAILURE, "%s", pmx_errmsg(pmxp));
	}

	pmx_free(pmxp);
	return (0);
}
