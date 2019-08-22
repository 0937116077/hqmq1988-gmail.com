/*
 * Support for decoding of VT ioctl commands.
 *
 * Copyright (c) 2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"

#include <linux/kd.h>
#include <linux/keyboard.h>

#include "print_fields.h"
#include "print_utils.h"

#include "xlat/kd_default_led_flags.h"
#include "xlat/kd_kbd_modes.h"
#include "xlat/kd_kbd_types.h"
#include "xlat/kd_keymap_flags.h"
#include "xlat/kd_key_tables.h"
#include "xlat/kd_key_types.h"
#include "xlat/kd_key_fn_keys.h"
#include "xlat/kd_key_fn_key_vals.h"
#include "xlat/kd_key_spec_keys.h"
#include "xlat/kd_key_pad_keys.h"
#include "xlat/kd_key_dead_keys.h"
#include "xlat/kd_key_cur_keys.h"
#include "xlat/kd_key_shift_keys.h"
#include "xlat/kd_key_ascii_keys.h"
#include "xlat/kd_key_lock_keys.h"
#include "xlat/kd_key_slock_keys.h"
#include "xlat/kd_key_brl_keys.h"
#include "xlat/kd_led_flags.h"
#include "xlat/kd_meta_vals.h"
#include "xlat/kd_modes.h"

#define XLAT_MACROS_ONLY
# include "xlat/kd_ioctl_cmds.h"
#undef XLAT_MACROS_ONLY

enum {
	KERNEL_PIT_TICK_RATE = 1193182,
	KERNEL_E_TABSZ = 256,
};

static int
kiocsound(struct tcb *const tcp, const unsigned int arg)
{
	unsigned int freq = arg ? KERNEL_PIT_TICK_RATE / arg : 0;

	tprintf(", %u", arg);
	if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_RAW) {
		if (freq)
			tprintf_comment("%u Hz", freq);
		else
			tprints_comment("off");
	}

	return RVAL_IOCTL_DECODED;
}

static int
kd_mk_tone(struct tcb *const tcp, const unsigned int arg)
{
	unsigned int ticks = arg >> 16;
	unsigned int count = arg & 0xFFFF;
	unsigned int freq = ticks && count ? KERNEL_PIT_TICK_RATE / count : 0;

	if (ticks)
		tprintf(", %u<<16|%u", ticks, count);
	else
		tprintf(", %u", count);

	if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_RAW) {
		if (freq)
			tprintf_comment("%u Hz, %u ms", freq, ticks);
		else
			tprints_comment("off");
	}

	return RVAL_IOCTL_DECODED;
}

static void
print_leds(struct tcb *const tcp, const kernel_ulong_t arg,
	   const bool get, const bool dflt)
{
	unsigned char val;

	if (get) {
		if (umove_or_printaddr(tcp, arg, &val))
			return;
	} else {
		val = arg;
	}

	if (get)
		tprints("[");
	printflags(dflt ? kd_default_led_flags : kd_led_flags, val,
		   "LED_???");
	if (get)
		tprints("]");
}

static int
kd_leds(struct tcb *const tcp, const unsigned int code,
	const kernel_ulong_t arg)
{
	bool get = false;
	bool dflt = false;

	switch (code) {
	case KDGETLED:
	case KDGKBLED:
		get = true;
	}

	switch (code) {
	case KDGKBLED:
	case KDSKBLED:
		dflt = true;
	}

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	print_leds(tcp, arg, get, dflt);

	return RVAL_IOCTL_DECODED;
}

static int
kd_get_kb_type(struct tcb *const tcp, const kernel_ulong_t arg)
{
	unsigned char val;

	if (entering(tcp)) {
		tprints(", ");
		return 0;
	}

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	tprints("[");
	printxval(kd_kbd_types, val, "KB_???");
	tprints("]");

	return RVAL_IOCTL_DECODED;
}

static int
kd_io(struct tcb *const tcp, kernel_ulong_t arg)
{
	enum { GPFIRST = 0x3b4, GPLAST = 0x3df };

	tprintf(", %#" PRI_klx, arg);

	if (arg >= GPFIRST && arg <= GPLAST
	    && xlat_verbose(xlat_verbosity) != XLAT_STYLE_RAW)
		tprintf_comment("GPFIRST + %" PRI_klu, arg - GPFIRST);

	return RVAL_IOCTL_DECODED;
}

static int
kd_set_mode(struct tcb *const tcp, const kernel_ulong_t arg)
{
	tprints(", ");

	printxval(kd_modes, arg, "KD_???");

	return RVAL_IOCTL_DECODED;
}

static int
kd_get_mode(struct tcb *const tcp, const kernel_ulong_t arg)
{
	unsigned int val;

	if (entering(tcp)) {
		tprints(", ");
		return 0;
	}

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	tprints("[");
	printxval(kd_modes, val, "KD_???");
	tprints("]");

	return RVAL_IOCTL_DECODED;
}

static int
kd_screen_map(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	if (entering(tcp) || !syserror(tcp))
		printstr_ex(tcp, arg, KERNEL_E_TABSZ, QUOTE_FORCE_HEX);
	else
		printaddr(arg);

	return RVAL_IOCTL_DECODED;
}

static bool
print_scrmap_array_member(struct tcb *tcp, void *elem_buf,
			  size_t elem_size, void *data)
{
	unsigned short val = * (unsigned short *) elem_buf;

	if ((xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV) ||
	    ((val & ~UNI_DIRECT_MASK) != UNI_DIRECT_BASE))
		tprintf("%#hx", val);

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
		return true;

	if ((val & ~UNI_DIRECT_MASK) == UNI_DIRECT_BASE)
		(xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE
			? tprintf_comment : tprintf)("UNI_DIRECT_BASE|%#hx",
						     val & UNI_DIRECT_MASK);

	return true;
}

static int
kd_uni_screen_map(struct tcb *const tcp, const kernel_ulong_t arg,
		  const bool get)
{
	unsigned short elem;

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	print_array(tcp, arg, KERNEL_E_TABSZ, &elem, sizeof(elem),
		    tfetch_mem, print_scrmap_array_member, 0);

	return RVAL_IOCTL_DECODED;
}

static int
kd_set_kbd_mode(struct tcb *const tcp, const unsigned int arg)
{
	tprints(", ");

	printxval_d(kd_kbd_modes, arg, "K_???");

	return RVAL_IOCTL_DECODED;
}

static int
kd_get_kbd_mode(struct tcb *const tcp, const kernel_ulong_t arg)
{
	unsigned int val;

	if (entering(tcp)) {
		tprints(", ");
		return 0;
	}

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	tprints("[");
	printxval_d(kd_kbd_modes, val, "K_???");
	tprints("]");

	return RVAL_IOCTL_DECODED;
}

static int
kd_kbd_entry(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	static const struct xlat *xlat_tables[] = {
		/* KT_LATIN */
		[KT_FN]    = kd_key_fn_keys,
		[KT_SPEC]  = kd_key_spec_keys,
		[KT_PAD]   = kd_key_pad_keys,
		[KT_DEAD]  = kd_key_dead_keys,
		/* KT_CONS */
		[KT_CUR]   = kd_key_cur_keys,
		[KT_SHIFT] = kd_key_shift_keys,
		/* KT_META */
		[KT_ASCII] = kd_key_ascii_keys,
		[KT_LOCK]  = kd_key_lock_keys,
		/* KT_LETTER */
		[KT_SLOCK] = kd_key_slock_keys,
		/* KT_DEAD2 */
		[KT_BRL]   = kd_key_brl_keys,
	};

	struct kbentry val;
	unsigned char ktyp;
	unsigned char kval;
	const char *str = NULL;

	if (entering(tcp)) {
		tprints(", ");

		if (umoven(tcp, arg, offsetofend(struct kbentry, kb_index),
			   &val)) {
			printaddr(arg);
			return RVAL_IOCTL_DECODED;
		}

		tprints("{");

		const char *keymap_str = xlookup(kd_key_tables, val.kb_table);

		if (keymap_str) {
			tprints("kb_table=");
			print_xlat_ex(val.kb_table, keymap_str,
				      XLAT_STYLE_DEFAULT);
		} else {
			PRINT_FIELD_FLAGS("", val, kb_table, kd_keymap_flags,
					  "K_???");
		}

		PRINT_FIELD_U(", ", val, kb_index);

		if (get)
			return 0;
	} else if (syserror(tcp)) {
		goto out;
	}

	if (umove(tcp, arg + offsetof(struct kbentry, kb_value),
			 &val.kb_value)) {
		tprints(", kb_value=???");
		goto out;
	}

	PRINT_FIELD_X(", ", val, kb_value);

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
		goto out;

	ktyp = KTYP(val.kb_value);
	kval = KVAL(val.kb_value);

	if (ktyp < ARRAY_SIZE(xlat_tables) && xlat_tables[ktyp])
		str = xlookup(xlat_tables[ktyp], val.kb_value);

	if (str) {
		tprints_comment(str);
	} else {
		tprints(" /* K(");
		printxvals_ex(ktyp, NULL, XLAT_STYLE_ABBREV,
			      kd_key_types, NULL);
		tprints(", ");

		switch (ktyp) {
		case KT_LATIN:
		case KT_META:
		case KT_LETTER:
		case KT_DEAD2:
			print_char(kval, SCF_QUOTES);
			break;
		default:
			tprintf("%#hhx", kval);
		}

		tprints(") */");
	}

out:
	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
kd_kbd_str_entry(struct tcb *const tcp, const kernel_ulong_t arg,
		 const bool get)
{
	struct kbsentry val;

	if (entering(tcp)) {
		tprints(", ");

		if (umove_or_printaddr(tcp, arg, &(val.kb_func)))
			return RVAL_IOCTL_DECODED;

		PRINT_FIELD_XVAL("{", val, kb_func, kd_key_fn_key_vals,
				 "KVAL(K_???"")");

		if (get)
			return 0;
	} else if (syserror(tcp)) {
		goto out;
	}

	tprints(", kb_string=");

	int ret = umovestr(tcp, arg + offsetof(struct kbsentry, kb_string),
			   sizeof(val.kb_string), (char *) val.kb_string);

	if (ret < 0) {
		tprints("???");
		goto out;
	}

	if (print_quoted_string((char *) val.kb_string,
				MIN(max_strlen,
				   (unsigned int) ret ?: sizeof(val.kb_string)),
				QUOTE_OMIT_TRAILING_0))
		tprints("...");

out:
	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static bool
print_kbdiacr_array_member(struct tcb *tcp, void *elem_buf,
			   size_t elem_size, void *data)
{
	struct kbdiacr *val = elem_buf;

	PRINT_FIELD_CHAR("{",  *val, diacr, SCF_QUOTES);
	PRINT_FIELD_CHAR(", ", *val, base, SCF_QUOTES);
	PRINT_FIELD_CHAR(", ", *val, result, SCF_QUOTES);
	tprints("}");

	return true;
}

static int
kd_diacr(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	unsigned int kb_cnt; /* struct kbdiacrs.kb_cnt */
	struct kbdiacr elem;

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	if (umove_or_printaddr(tcp, arg, &kb_cnt))
		return RVAL_IOCTL_DECODED;

	tprintf("{kb_cnt=%u, kbdiacr=", kb_cnt);

	print_array(tcp, arg + offsetof(struct kbdiacrs, kbdiacr),
		    MIN(kb_cnt, 256), &elem, sizeof(elem), tfetch_mem,
		    print_kbdiacr_array_member, 0);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static bool
print_kbdiacruc_array_member(struct tcb *tcp, void *elem_buf,
			     size_t elem_size, void *data)
{
	struct kbdiacruc *val = elem_buf;

	PRINT_FIELD_X("{",  *val, diacr);
	PRINT_FIELD_X(", ", *val, base);
	PRINT_FIELD_X(", ", *val, result);
	tprints("}");

	return true;
}

static int
kd_diacr_uc(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	unsigned int kb_cnt; /* struct kbdiacrs.kb_cnt */
	struct kbdiacruc elem;

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	if (umove_or_printaddr(tcp, arg, &kb_cnt))
		return RVAL_IOCTL_DECODED;

	tprintf("{kb_cnt=%u, kbdiacruc=", kb_cnt);

	print_array(tcp, arg + offsetof(struct kbdiacrsuc, kbdiacruc),
		    MIN(kb_cnt, 256), &elem, sizeof(elem), tfetch_mem,
		    print_kbdiacruc_array_member, 0);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
kd_keycode(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	struct kbkeycode val;

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	if (umove_or_printaddr(tcp, arg, &val))
		return RVAL_IOCTL_DECODED;

	PRINT_FIELD_X("{",  val, scancode);
	PRINT_FIELD_X(", ", val, keycode);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
kd_sigaccept(struct tcb *const tcp, const kernel_ulong_t arg)
{
	tprints(", ");

	if (arg < INT_MAX)
		printsignal(arg);
	else
		tprintf("%" PRI_klu, arg);

	return RVAL_IOCTL_DECODED;
}

static int
kd_kbdrep(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct kbd_repeat val;

	tprints(", ");

	PRINT_FIELD_D("{",  val, delay);
	PRINT_FIELD_D(", ", val, period);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
kd_font(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	/*
	 * [GP]IO_FONT are equivalent to KDFONTOP with width == 8
	 * and charcount == 256, so the total size
	 * is (width + 7) / 8 * charcount == 256.
	 */
	printstr_ex(tcp, arg, 256, QUOTE_FORCE_HEX);

	return RVAL_IOCTL_DECODED;
}

static int
kd_kbmeta(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	unsigned int val;

	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	if (get) {
		if (umove_or_printaddr(tcp, arg, &val))
			return RVAL_IOCTL_DECODED;
	} else {
		val = arg;
	}

	if (get)
		tprints("[");
	printxval(kd_meta_vals, val, "K_???");
	if (get)
		tprints("]");

	return RVAL_IOCTL_DECODED;
}

static int
kd_cmap(struct tcb *const tcp, const kernel_ulong_t arg, const bool get)
{
	if (entering(tcp)) {
		tprints(", ");

		if (get)
			return 0;
	}

	printstr_ex(tcp, arg, 3 * 16, QUOTE_FORCE_HEX);

	return RVAL_IOCTL_DECODED;
}

int
kd_ioctl(struct tcb *const tcp, const unsigned int code,
	 kernel_ulong_t arg)
{
	arg = truncate_kulong_to_current_wordsize(arg);

	switch (code) {
	case KIOCSOUND:
		return kiocsound(tcp, arg);
	case KDMKTONE:
		return kd_mk_tone(tcp, arg);

	case KDGETLED:
	case KDSETLED:
	case KDGKBLED:
	case KDSKBLED:
		return kd_leds(tcp, code, arg);

	case KDGKBTYPE:
		return kd_get_kb_type(tcp, arg);

	case KDADDIO:
	case KDDELIO:
		return kd_io(tcp, arg);

	case KDSETMODE:
		return kd_set_mode(tcp, arg);
	case KDGETMODE:
		return kd_get_mode(tcp, arg);

	case GIO_SCRNMAP:
	case PIO_SCRNMAP:
		return kd_screen_map(tcp, arg, code == GIO_SCRNMAP);

	case GIO_UNISCRNMAP:
	case PIO_UNISCRNMAP:
		return kd_uni_screen_map(tcp, arg, code == GIO_UNISCRNMAP);

	case KDGKBMODE:
		return kd_get_kbd_mode(tcp, arg);
	case KDSKBMODE:
		return kd_set_kbd_mode(tcp, arg);

	case KDGKBENT:
	case KDSKBENT:
		return kd_kbd_entry(tcp, arg, code == KDGKBENT);

	case KDGKBSENT:
	case KDSKBSENT:
		return kd_kbd_str_entry(tcp, arg, code == KDGKBSENT);

	case KDGKBDIACR:
	case KDSKBDIACR:
		return kd_diacr(tcp, arg, code == KDGKBDIACR);

	case KDGKBDIACRUC:
	case KDSKBDIACRUC:
		return kd_diacr_uc(tcp, arg, code == KDGKBDIACRUC);

	case KDGETKEYCODE:
	case KDSETKEYCODE:
		return kd_keycode(tcp, arg, code == KDGETKEYCODE);

	case KDSIGACCEPT:
		return kd_sigaccept(tcp, arg);

	case KDKBDREP:
		return kd_kbdrep(tcp, arg);

	case GIO_FONT:
	case PIO_FONT:
		return kd_font(tcp, arg, code == GIO_FONT);

	case KDGKBMETA:
	case KDSKBMETA:
		return kd_kbmeta(tcp, arg, code == KDGKBMETA);

	case GIO_CMAP:
	case PIO_CMAP:
		return kd_cmap(tcp, arg, code == GIO_CMAP);

	/* no arguments */
	case KDENABIO:
	case KDDISABIO:
	case KDMAPDISP:
	case KDUNMAPDISP:
	case PIO_UNIMAPCLR:
	case PIO_FONTRESET:
		return RVAL_IOCTL_DECODED;
	}

	/* GIO_UNIMAP, PIO_UNIMAP, GIO_FONTX, PIO_FONTX, KDFONTOP */
	return kd_mpers_ioctl(tcp, code, arg);
}
