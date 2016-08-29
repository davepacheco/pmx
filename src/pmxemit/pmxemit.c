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

	pmx_emit_metadata(pmxp, "generator", "pmxemit");
	pmx_emit_metadata(pmxp, "generator_version", "1.0.0");
	pmx_emit_metadata(pmxp, "generated_at", nowstr);

	if (pmx_errno(pmxp) != PMXE_OK) {
		errx(EXIT_FAILURE, "%s", pmx_errmsg(pmxp));
	}

	pmx_free(pmxp);
	return (0);
}
