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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pmx/pmx.h>
#include "pmx_impl.h"

/*
 * Statically-allocated buffer for fatal error messages.
 */
const char *pmx_panichdr = "\npmx_panic:\n";
char pmx_panicstr[512];

/*
 * Lifecycle of a pmx_stream_t
 */
pmx_stream_t *
pmx_create_stream(FILE *outfp, FILE *errfp)
{
	pmx_stream_t *pmxp;

	assert(outfp != NULL);

	pmxp = calloc(1, sizeof (*pmxp));
	if (pmxp == NULL) {
		return (NULL);
	}

	pmxp->pxs_outstream = outfp;
	pmxp->pxs_outstream = errfp;
	return (pmxp);
}

void
pmx_free(pmx_stream_t *pmxp)
{
	free(pmxp);
}


/*
 * Error management: public interfaces
 */

pmx_error_t
pmx_errno(pmx_stream_t *pmxp)
{
	assert(pmxp->pxs_error >= 0);
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

	pmx_panic(pmxp, "unrecognized pmx_error_t: %d\n", pmxp->pxs_error);
}

/*
 * Error management: internal interfaces
 */

void
pmx_set_errno(pmx_stream_t *pmxp, pmx_error_t pmxerr)
{
	assert(pmxerr >= 0);
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
	assert(pmxerr >= 0);
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
}

void
pmx_panic(pmx_stream_t *pmxp, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	pmx_vpanic(pmxp, fmt, args);
	va_end(args);
}

void
pmx_vpanic(PMX_UNUSED pmx_stream_t *pmxp, const char *fmt, va_list args)
{
	int nbytes;

	nbytes = vsnprintf(pmx_panicstr, sizeof (pmx_panicstr), fmt, args);
	(void) write(STDERR_FILENO, pmx_panichdr, strlen(pmx_panichdr));
	(void) write(STDERR_FILENO, pmx_panicstr, nbytes);
	abort();
}
