/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2016, Joyent, Inc.
 */

/*
 * pmx_impl.h: private implementation declarations for postmortem exporter
 */

#ifndef	_PMX_IMPL_H
#define	_PMX_IMPL_H

#include <stdarg.h>

#include <pmx/pmx.h>

/* Maximum length of internal error messages. */
#define	PMX_ERRMSGLEN 256

typedef enum {
	PMXS_INIT,	/* initial state */
	PMXS_TOP,	/* ready to emit arbitrary nodes and edges */

	PMXS_NODE,	/* emitting a node */
	PMXS_EDGE,	/* emitting an edge */

	PMXS_FINI,	/* no more output accepted */
} pmx_state_t;

/*
 * These values are defined in the specification.
 * XXX Not yet, but they should be.
 */
typedef enum {
	PMXN_ODDBALL = 1
} pmx_nodetype_t;

/*
 * A pmx_stream_t represents an export operation.  The stream progresses through
 * the states above, and the end result is a representation of JavaScript state
 * from a program.
 */
struct pmx_stream {
	/* state of the export */
	pmx_state_t	pxs_state;

	/* output and error streams */
	FILE		*pxs_outstream;
	FILE		*pxs_errstream;

	/* most recent error code and message */
	pmx_error_t	pxs_error;
	char		pxs_errmsg[PMX_ERRMSGLEN];

	/* booleans and counters used to help validate output */
	pmx_boolean_t	pxs_emitted_hole;
	pmx_boolean_t	pxs_emitted_null;
	pmx_boolean_t	pxs_emitted_undefined;
	pmx_boolean_t	pxs_emitted_true;
	pmx_boolean_t	pxs_emitted_false;
	unsigned long	pxs_nwarnings;
	unsigned long	pxs_nfields;
	/* XXX need way for callers to check for warnings */

	/* counters (primarily for debugging) */
	unsigned long	pxs_nmetadata;
	unsigned long	pxs_nnodes;
	unsigned long	pxs_nedges;
};

/*
 * Macros used to apply function attributes.
 */
#define	PMX_NORETURN	__attribute__((__noreturn__))
#define	PMX_PRINTFLIKE1 __attribute__((__format__(__printf__, 1, 2)))
#define	PMX_PRINTFLIKE2 __attribute__((__format__(__printf__, 2, 3)))
#define	PMX_PRINTFLIKE3 __attribute__((__format__(__printf__, 3, 4)))
#define	PMX_UNUSED	__attribute__((__unused__))

extern void pmx_set_errno(pmx_stream_t *, pmx_error_t);
PMX_PRINTFLIKE3
    extern void pmx_error(pmx_stream_t *, pmx_error_t, const char *, ...);
extern void pmx_verror(pmx_stream_t *, pmx_error_t, const char *, va_list);

PMX_PRINTFLIKE2
    extern void pmx_warn(pmx_stream_t *, const char *, ...);
extern void pmx_vwarn(pmx_stream_t *, const char *, va_list);

extern int pmx_assfail(const char *, const char *, unsigned int);
PMX_NORETURN PMX_PRINTFLIKE1 extern void pmx_panic(const char *, ...);
PMX_NORETURN extern void pmx_vpanic(const char *, va_list);

pmx_boolean_t pmx_cstr_printable(const char *);

#define	VERIFY(X) ((void)((X) || pmx_assfail(#X, __FILE__, __LINE__)))

#endif /* not defined _PMX_IMPL_H */
