/*
 * Support for decoding of personality-dependent VT ioctl commands.
 *
 * Copyright (c) 2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"

#include <linux/kd.h>

#include DEF_MPERS_TYPE(struct_unimapdesc)
#include DEF_MPERS_TYPE(struct_consolefontdesc)
#include DEF_MPERS_TYPE(struct_console_font)
#include DEF_MPERS_TYPE(struct_console_font_op)

typedef struct unimapdesc struct_unimapdesc;
typedef struct consolefontdesc struct_consolefontdesc;
typedef struct console_font struct_console_font;
typedef struct console_font_op struct_console_font_op;

#include MPERS_DEFS

#include "print_fields.h"

#include "xlat/kd_font_flags.h"
#include "xlat/kd_font_ops.h"

#define XLAT_MACROS_ONLY
# include "xlat/kd_ioctl_cmds.h"
#undef XLAT_MACROS_ONLY


static bool
print_unipair_array_member(struct tcb *tcp, void *elem_buf,
			   size_t elem_size, void *data)
{
	struct unipair *val = elem_buf;

	PRINT_FIELD_X("{",  *val, unicode);
	PRINT_FIELD_X(", ", *val, fontpos);
	tprints("}");

	return true;
}

static void
print_unimapdesc(struct tcb *const tcp, const struct_unimapdesc *umd,
		 const bool get)
{
	PRINT_FIELD_U("{", *umd, entry_ct);

	if (get) {
		PRINT_FIELD_PTR(", ", *umd, entries);
	} else {
		struct unipair elem;

		tprints(", entries=");
		print_array(tcp, (mpers_ptr_t) umd->entries, umd->entry_ct,
			    &elem, sizeof(elem), tfetch_mem,
			    print_unipair_array_member, 0);
	}

	tprints("}");
}

static int
kd_unimap(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	struct_unimapdesc val;

	tprints(entering(tcp) ? ", " : " => ");

	if (umove_or_printaddr_ignore_syserror(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	if (exiting(tcp) && syserror(tcp) && val.entries)
		return RVAL_IOCTL_DECODED;

	print_unimapdesc(tcp, &val, get && entering(tcp));

	return get && entering(tcp) ? 0 : RVAL_IOCTL_DECODED;
}

static void
print_consolefontdesc(struct tcb *const tcp, const struct_consolefontdesc *cfd,
		      const bool get)
{
	PRINT_FIELD_U("{", *cfd, charcount);
	PRINT_FIELD_U(", ", *cfd, charheight);

	if (get) {
		PRINT_FIELD_PTR(", ", *cfd, chardata);
	} else {
		tprints(", chardata=");
		printstr_ex(tcp, (mpers_ptr_t) cfd->chardata,
			    MIN(cfd->charcount, 512), QUOTE_FORCE_HEX);
	}

	tprints("}");
}

static int
kd_fontx(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	struct_consolefontdesc val;

	tprints(entering(tcp) ? ", " : " => ");

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	print_consolefontdesc(tcp, &val, get && entering(tcp));

	return get && entering(tcp) ? 0 : RVAL_IOCTL_DECODED;
}

static void
print_console_font_op(struct tcb *const tcp, const struct_console_font_op *cfo)
{
	enum { KERNEL_MAX_FONT_NAME = 32 };

	tprints("{");

	if (entering(tcp)) {
		PRINT_FIELD_XVAL("", *cfo, op, kd_font_ops, "KD_FONT_OP_???");

		switch (cfo->op) {
		case KD_FONT_OP_SET_DEFAULT:
		case KD_FONT_OP_COPY:
			break;
		default:
			PRINT_FIELD_FLAGS(", ", *cfo, flags, kd_font_flags,
					  "KD_FONT_FLAG_???");
		}

		tprints(", ");
	}

	switch (cfo->op) {
	case KD_FONT_OP_COPY:
		break;
	default:
		PRINT_FIELD_U("",   *cfo, width);
		PRINT_FIELD_U(", ", *cfo, height);
	}

	switch (cfo->op) {
	case KD_FONT_OP_SET_DEFAULT:
	case KD_FONT_OP_COPY:
		break;
	default:
		PRINT_FIELD_U(", ", *cfo, charcount);
	}

	switch (cfo->op) {
	case KD_FONT_OP_GET:
		if (entering(tcp)) {
			PRINT_FIELD_PTR(", ", *cfo, data);
			break;
		}
		ATTRIBUTE_FALLTHROUGH;

	case KD_FONT_OP_SET:
		tprints(", data=");
		printstr_ex(tcp, (mpers_ptr_t) cfo->data,
			    ROUNDUP_DIV(MIN(cfo->width, 32), 8) * 32 *
				MIN(cfo->charcount, 512),
			    QUOTE_FORCE_HEX);
		break;

	case KD_FONT_OP_SET_DEFAULT:
		tprints(", data=");
		printstrn(tcp, (mpers_ptr_t) cfo->data, KERNEL_MAX_FONT_NAME);
		break;

	case KD_FONT_OP_COPY:
		break;

	default:
		PRINT_FIELD_PTR(", ", *cfo, data);
	}
}

static int
kd_font_op(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_console_font_op val;

	tprints(entering(tcp) ? ", " : " => ");

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	print_console_font_op(tcp, &val);

	return val.op == KD_FONT_OP_COPY ? RVAL_IOCTL_DECODED : 0;
}

MPERS_PRINTER_DECL(int, kd_mpers_ioctl, struct tcb *const tcp,
		   const unsigned int code, const kernel_ulong_t arg)
{
	switch (code)
	{
	case GIO_UNIMAP:
	case PIO_UNIMAP:
		return kd_unimap(tcp, arg, code == GIO_UNIMAP);

	case GIO_FONTX:
	case PIO_FONTX:
		return kd_fontx(tcp, arg, code == GIO_FONTX);

	case KDFONTOP:
		return kd_font_op(tcp, arg);
	}

	return RVAL_DECODED;
}
