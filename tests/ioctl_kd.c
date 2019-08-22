/*
 * This file is part of ioctl_kd strace test.
 *
 * Copyright (c) 2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scno.h"

#include <linux/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

#if RETVAL_INJECTED
# define RETVAL "42 (INJECTED)\n"
#else
# define RETVAL "-1 EBADF (%m)\n"
#endif

#define ARG_XLAT_KNOWN(val_, str_) val_, XLAT_KNOWN(val_, str_)
#define ARG_XLAT_UNKNOWN(val_, str_) val_, XLAT_UNKNOWN(val_, str_)

#if XLAT_RAW

# define IOC_FMT "%#x"
# define IOC_ARG(ioc_) ioc_

# define ABBR(s_) ""
# define RAW(s_) s_
# define VERB(s_) ""
# define NABBR(s_) s_
# define NRAW(s_) ""
# define NVERB(s_) s_

#elif XLAT_VERBOSE

# define IOC_FMT "%#x /* %s */"
# define IOC_ARG(ioc_) ioc_, #ioc_

# define ABBR(s_) ""
# define RAW(s_) ""
# define VERB(s_) s_
# define NABBR(s_) s_
# define NRAW(s_) s_
# define NVERB(s_) ""

#else /* !XLAT_RAW && !XLAT_VERBOSE */

# define IOC_FMT "%s"
# define IOC_ARG(ioc_) #ioc_

# define ABBR(s_) s_
# define RAW(s_) ""
# define VERB(s_) ""
# define NABBR(s_) ""
# define NRAW(s_) s_
# define NVERB(s_) s_

#endif /* XLAT_RAW, XLAT_VERBOSE */

struct arg_val {
	kernel_ulong_t val;
	const char *str;
};

static long
sys_ioctl(kernel_long_t fd, kernel_ulong_t cmd, kernel_ulong_t arg)
{
	return syscall(__NR_ioctl, fd, cmd, arg);
}

int
main(int argc, char *argv[])
{
	static const kernel_ulong_t magic =
		(kernel_ulong_t) 0xdeadbeefbadc0dedULL;

	static const uint32_t unknown_ioctls[] = {
		0xffffffff, 0xc0007fff, 0xfffb800c, 0xfff8c000,
		0xffffffff, 0xffffffff, 0xffffffff, 0xf3ffffff,
	};

	enum { MAP_ELEM_BIT = sizeof(unknown_ioctls[0]) * CHAR_BIT };

	long rc;
	int saved_errno;

#if RETVAL_INJECTED
	unsigned long num_skip;
	bool locked = false;

	if (argc < 2)
		error_msg_and_fail("Usage: %s NUM_SKIP", argv[0]);

	num_skip = strtoul(argv[1], NULL, 0);

	for (unsigned int i = 0; i < num_skip; i++) {
		long rc = sys_ioctl(-1, KDGETLED, 0);
		printf("ioctl(-1, " IOC_FMT ", NULL) = %s%s\n",
		       IOC_ARG(KDGETLED), sprintrc(rc),
		       rc == 42 ? " (INJECTED)" : "");

		if (rc != 42)
			continue;

		locked = true;
		break;
	}

	if (!locked)
		error_msg_and_fail("Hasn't locked on ioctl(-1"
				   ", EVIOCGID, NULL) returning 42");
#endif

	for (size_t i = 0; i < ARRAY_SIZE(unknown_ioctls); i++) {
		for (size_t j = 0; j < MAP_ELEM_BIT; j++) {
			if (!((unknown_ioctls[i] >> j) & 1))
				continue;

			const unsigned int id = i * MAP_ELEM_BIT + j;

			sys_ioctl(-1, 'K' << 8 | id, magic);
			printf("ioctl(-1, "
			       NABBR("%#x") VERB(" /* ")
			       NRAW("_IOC(_IOC_NONE, 0x4b, %#x, 0)") VERB(" */")
			       ", %#lx) = " RETVAL,
#if XLAT_RAW || XLAT_VERBOSE
			       'K' << 8 | id,
#endif
#if !XLAT_RAW
			       id,
#endif
			       (unsigned long) magic);
		}
	}

	/* KIOCSOUND */
	sys_ioctl(-1, KIOCSOUND, 0);
	printf("ioctl(-1, " IOC_FMT ", 0" NRAW(" /* off */") ") = " RETVAL,
	       IOC_ARG(KIOCSOUND));

	sys_ioctl(-1, KIOCSOUND, 1);
	printf("ioctl(-1, " IOC_FMT ", 1" NRAW(" /* 1193182 Hz */")
	       ") = " RETVAL, IOC_ARG(KIOCSOUND));

	sys_ioctl(-1, KIOCSOUND, 440);
	printf("ioctl(-1, " IOC_FMT ", 440" NRAW(" /* 2711 Hz */")
	       ") = " RETVAL, IOC_ARG(KIOCSOUND));

	sys_ioctl(-1, KIOCSOUND,
		  (kernel_ulong_t) (0xbadc0ded00000000ULL | 1193182));
	printf("ioctl(-1, " IOC_FMT ", 1193182" NRAW(" /* 1 Hz */")
	       ") = " RETVAL, IOC_ARG(KIOCSOUND));

	sys_ioctl(-1, KIOCSOUND, 1193183);
	printf("ioctl(-1, " IOC_FMT ", 1193183" NRAW(" /* off */")
	       ") = " RETVAL, IOC_ARG(KIOCSOUND));

	sys_ioctl(-1, KIOCSOUND, (kernel_ulong_t) 0xbadc0deddeadfaceULL);
	printf("ioctl(-1, " IOC_FMT ", 3735943886" NRAW(" /* off */")
	       ") = " RETVAL, IOC_ARG(KIOCSOUND));


	/* KDMKTONE */
	sys_ioctl(-1, KDMKTONE, 0);
	printf("ioctl(-1, " IOC_FMT ", 0" NRAW(" /* off */") ") = " RETVAL,
	       IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE, 440);
	printf("ioctl(-1, " IOC_FMT ", 440" NRAW(" /* off */") ") = " RETVAL,
	       IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE, 0xffff);
	printf("ioctl(-1, " IOC_FMT ", 65535" NRAW(" /* off */") ") = " RETVAL,
	       IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE, 0x10000);
	printf("ioctl(-1, " IOC_FMT ", 1<<16|0" NRAW(" /* off */")
	       ") = " RETVAL, IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE,
		  (kernel_ulong_t) (0xbadc0ded00000000ULL | 0x10001));
	printf("ioctl(-1, " IOC_FMT ", 1<<16|1" NRAW(" /* 1193182 Hz, 1 ms */")
	       ") = " RETVAL, IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE, 0x1ffff);
	printf("ioctl(-1, " IOC_FMT ", 1<<16|65535" NRAW(" /* 18 Hz, 1 ms */")
	       ") = " RETVAL, IOC_ARG(KDMKTONE));

	sys_ioctl(-1, KDMKTONE, (kernel_ulong_t) 0xbadc0deddeadfaceULL);
	printf("ioctl(-1, " IOC_FMT ", 57005<<16|64206"
	       NRAW(" /* 18 Hz, 57005 ms */") ") = " RETVAL, IOC_ARG(KDMKTONE));


	/* KDGETLED */
	static const struct arg_val led_vecs[] = {
		{ ARG_STR(0) },
		{ ARG_XLAT_KNOWN(0x1, "LED_SCR") },
		{ ARG_XLAT_KNOWN(0x7, "LED_SCR|LED_NUM|LED_CAP") },
		{ ARG_XLAT_KNOWN(0xfe, "LED_NUM|LED_CAP|0xf8") },
		{ (kernel_ulong_t) 0xbadc0dedfeedfaf0ULL,
		  XLAT_UNKNOWN(0xf0, "LED_???") },
	};

	unsigned char *leds = tail_alloc(sizeof(*leds));

	sys_ioctl(-1, KDGETLED, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGETLED));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGETLED, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGETLED));
	}

	sys_ioctl(-1, KDGETLED, (uintptr_t) (leds + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGETLED), leds + 1);

	for (size_t i = 0; i < ARRAY_SIZE(led_vecs); i++) {
		*leds = led_vecs[i].val;
		rc = sys_ioctl(-1, KDGETLED, (uintptr_t) leds);
		if (rc >= 0) {
			printf("ioctl(-1, " IOC_FMT ", [%s]) = " RETVAL,
			       IOC_ARG(KDGETLED), led_vecs[i].str);
		} else {
			printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
			       IOC_ARG(KDGETLED), leds);
		}
	}


	/* KDSETLED */
	for (size_t i = 0; i < ARRAY_SIZE(led_vecs); i++) {
		sys_ioctl(-1, KDSETLED, led_vecs[i].val);
		printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
		       IOC_ARG(KDSETLED), led_vecs[i].str);
	}

	sys_ioctl(-1, KDSETLED, (kernel_ulong_t) 0xdeadc0defeedfaceULL);
	printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
	       IOC_ARG(KDSETLED), XLAT_STR(LED_NUM|LED_CAP|0xc8));

	/* KDGKBTYPE */
	static const struct arg_val kbt_vecs[] = {
		{ ARG_XLAT_UNKNOWN(0, "KB_???") },
		{ ARG_XLAT_KNOWN(0x1, "KB_84") },
		{ ARG_XLAT_KNOWN(0x2, "KB_101") },
		{ ARG_XLAT_KNOWN(0x3, "KB_OTHER") },
		{ ARG_XLAT_UNKNOWN(0x4, "KB_???") },
		{ (kernel_ulong_t) 0xbadc0dedcacafefeULL,
		  XLAT_UNKNOWN(0xfe, "KB_???") },
	};

	unsigned char *kbt = tail_alloc(sizeof(*kbt));

	sys_ioctl(-1, KDGKBTYPE, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGKBTYPE));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGKBTYPE, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGKBTYPE));
	}

	sys_ioctl(-1, KDGKBTYPE, (uintptr_t) (kbt + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGKBTYPE), kbt + 1);

	for (size_t i = 0; i < ARRAY_SIZE(kbt_vecs); i++) {
		*kbt = kbt_vecs[i].val;
		rc = sys_ioctl(-1, KDGKBTYPE, (uintptr_t) kbt);
		if (rc >= 0) {
			printf("ioctl(-1, " IOC_FMT ", [%s]) = " RETVAL,
			       IOC_ARG(KDGKBTYPE), kbt_vecs[i].str);
		} else {
			printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
			       IOC_ARG(KDGKBTYPE), kbt);
		}
	}


	/* KDADDIO */
	static const struct arg_val iop_vecs[] = {
		{ ARG_STR(0) },
		{ ARG_STR(0x3b3) },
		{ ARG_STR(0x3b4) NRAW(" /* GPFIRST + 0 */") },
		{ ARG_STR(0x3c0) NRAW(" /* GPFIRST + 12 */") },
		{ ARG_STR(0x3df) NRAW(" /* GPFIRST + 43 */") },
		{ ARG_STR(0x3e0) },
		{ ARG_STR(0xdeadc0de) },
		{ (kernel_ulong_t) 0xbadc0dedfacefeedULL,
#if SIZEOF_LONG > 4
		  "0xbadc0dedfacefeed"
#else
		  "0xfacefeed"
#endif
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(iop_vecs); i++) {
		sys_ioctl(-1, KDADDIO, iop_vecs[i].val);
		printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
		       IOC_ARG(KDADDIO), iop_vecs[i].str);
	}


	/* KDDELIO */
	for (size_t i = 0; i < ARRAY_SIZE(iop_vecs); i++) {
		sys_ioctl(-1, KDDELIO, iop_vecs[i].val);
		printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
		       IOC_ARG(KDDELIO), iop_vecs[i].str);
	}


	/* KDENABIO */
	sys_ioctl(-1, KDENABIO, (kernel_ulong_t) 0xbadc0deddeadface);
	printf("ioctl(-1, " IOC_FMT ") = " RETVAL, IOC_ARG(KDENABIO));


	/* KDDISABIO */
	sys_ioctl(-1, KDDISABIO, (kernel_ulong_t) 0xbadc0deddeadface);
	printf("ioctl(-1, " IOC_FMT ") = " RETVAL, IOC_ARG(KDDISABIO));


	/* KDSETMODE */
	static const struct arg_val mode_vecs[] = {
		{ ARG_XLAT_KNOWN(0, "KD_TEXT") },
		{ ARG_XLAT_KNOWN(0x1, "KD_GRAPHICS") },
		{ ARG_XLAT_KNOWN(0x3, "KD_TEXT1") },
		{ ARG_XLAT_UNKNOWN(0x4, "KD_???") },
		{ (kernel_ulong_t) 0xbadc0dedcacafefeULL,
		  "0xcacafefe" NRAW(" /* KD_??? */") },
	};

	for (size_t i = 0; i < ARRAY_SIZE(mode_vecs); i++) {
		sys_ioctl(-1, KDSETMODE, mode_vecs[i].val);
		printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
		       IOC_ARG(KDSETMODE), mode_vecs[i].str);
	}


	/* KDGETMODE */
	unsigned int *mode = tail_alloc(sizeof(*mode));

	sys_ioctl(-1, KDGETMODE, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGETMODE));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGETMODE, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGETMODE));
	}

	sys_ioctl(-1, KDGETMODE, (uintptr_t) (mode + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGETMODE), mode + 1);

	for (size_t i = 0; i < ARRAY_SIZE(mode_vecs); i++) {
		*mode = mode_vecs[i].val;
		rc = sys_ioctl(-1, KDGETMODE, (uintptr_t) mode);
		if (rc >= 0) {
			printf("ioctl(-1, " IOC_FMT ", [%s]) = " RETVAL,
			       IOC_ARG(KDGETMODE), mode_vecs[i].str);
		} else {
			printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
			       IOC_ARG(KDGETMODE), mode);
		}
	}


	/* KDMAPDISP */
	sys_ioctl(-1, KDMAPDISP, (kernel_ulong_t) 0xbadc0deddeadface);
	printf("ioctl(-1, " IOC_FMT ") = " RETVAL, IOC_ARG(KDMAPDISP));


	/* KDUNMAPDISP */
	sys_ioctl(-1, KDUNMAPDISP, (kernel_ulong_t) 0xbadc0deddeadface);
	printf("ioctl(-1, " IOC_FMT ") = " RETVAL, IOC_ARG(KDUNMAPDISP));


	/* GIO_SCRNMAP */
	char *scrnmap = tail_alloc(E_TABSZ);

	fill_memory_ex(scrnmap, E_TABSZ, 0, 0xff);

	sys_ioctl(-1, GIO_SCRNMAP, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(GIO_SCRNMAP));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, GIO_SCRNMAP, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(GIO_SCRNMAP));
	}

	sys_ioctl(-1, GIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(GIO_SCRNMAP), scrnmap + E_TABSZ);

	sys_ioctl(-1, GIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 31);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(GIO_SCRNMAP), scrnmap + E_TABSZ - 31);

	sys_ioctl(-1, GIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 32);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(GIO_SCRNMAP), scrnmap + E_TABSZ - 32);

	rc = sys_ioctl(-1, GIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 33);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", ", IOC_ARG(GIO_SCRNMAP));
	if ((rc >= 0) && (DEFAULT_STRLEN <= 32)) {
		print_quoted_hex(scrnmap + E_TABSZ - 33, 32);
		printf("...");
	} else {
		printf("%p", scrnmap + E_TABSZ - 33);
	}
	errno = saved_errno;
	printf(") = " RETVAL);

	rc = sys_ioctl(-1, GIO_SCRNMAP, (uintptr_t) scrnmap);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", ", IOC_ARG(GIO_SCRNMAP));
	if (rc >= 0) {
		print_quoted_hex(scrnmap, MIN(E_TABSZ, DEFAULT_STRLEN));
		if (DEFAULT_STRLEN < E_TABSZ)
			printf("...");
	} else {
		printf("%p", scrnmap);
	}
	errno = saved_errno;
	printf(") = " RETVAL);


	/* PIO_SCRNMAP */
	sys_ioctl(-1, PIO_SCRNMAP, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(PIO_SCRNMAP));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, PIO_SCRNMAP, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(PIO_SCRNMAP));
	}

	sys_ioctl(-1, PIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(PIO_SCRNMAP), scrnmap + E_TABSZ);

	sys_ioctl(-1, PIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 31);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(PIO_SCRNMAP), scrnmap + E_TABSZ - 31);

	sys_ioctl(-1, PIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 32);
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(PIO_SCRNMAP), scrnmap + E_TABSZ - 32);

	rc = sys_ioctl(-1, PIO_SCRNMAP, (uintptr_t) scrnmap + E_TABSZ - 33);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", ", IOC_ARG(PIO_SCRNMAP));
	if (DEFAULT_STRLEN <= 32) {
		print_quoted_hex(scrnmap + E_TABSZ - 33, 32);
		printf("...");
	} else {
		printf("%p", scrnmap + E_TABSZ - 33);
	}
	errno = saved_errno;
	printf(") = " RETVAL);

	rc = sys_ioctl(-1, PIO_SCRNMAP, (uintptr_t) scrnmap);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", ", IOC_ARG(PIO_SCRNMAP));
	print_quoted_hex(scrnmap, MIN(E_TABSZ, DEFAULT_STRLEN));
	errno = saved_errno;
	printf("%s) = " RETVAL, DEFAULT_STRLEN < E_TABSZ ? "..." : "");


	/* KDGKBMODE */
	static const struct arg_val kbmode_vecs[] = {
		{ ARG_XLAT_UNKNOWN(-1, "K_???") },
		{ ARG_XLAT_KNOWN(0, "K_RAW") },
		{ ARG_XLAT_KNOWN(1, "K_XLATE") },
		{ ARG_XLAT_KNOWN(4, "K_OFF") },
		{ ARG_XLAT_UNKNOWN(5, "K_???") },
		{ (kernel_ulong_t) 0xbadc0dedfeeddeadULL,
		  XLAT_UNKNOWN(-17965395, "K_???") },
	};

	sys_ioctl(-1, KDGKBMODE, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGKBMODE));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGKBMODE, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGKBMODE));
	}

	sys_ioctl(-1, KDGKBMODE, (uintptr_t) (mode + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGKBMODE), mode + 1);

	for (size_t i = 0; i < ARRAY_SIZE(kbmode_vecs); i++) {
		*mode = kbmode_vecs[i].val;
		rc = sys_ioctl(-1, KDGKBMODE, (uintptr_t) mode);
		if (rc >= 0) {
			printf("ioctl(-1, " IOC_FMT ", [%s]) = " RETVAL,
			       IOC_ARG(KDGKBMODE), kbmode_vecs[i].str);
		} else {
			printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
			       IOC_ARG(KDGKBMODE), mode);
		}
	}


	/* KDSKBMODE */
	for (size_t i = 0; i < ARRAY_SIZE(kbmode_vecs); i++) {
		sys_ioctl(-1, KDSKBMODE, kbmode_vecs[i].val);
		printf("ioctl(-1, " IOC_FMT ", %s) = " RETVAL,
		       IOC_ARG(KDSKBMODE), kbmode_vecs[i].str);
	}


	/* KDGKBENT */
	static const struct arg_val kbtbl_vecs[] = {
		{ ARG_XLAT_KNOWN(0, "K_NORMTAB") },
		{ ARG_XLAT_KNOWN(0x1, "K_SHIFTTAB") },
		{ ARG_XLAT_KNOWN(0x3, "K_ALTSHIFTTAB") },
		{ ARG_XLAT_KNOWN(0x4, "1<<KG_CTRL") },
		{ ARG_XLAT_KNOWN(0xff,
				 "1<<KG_SHIFT|1<<KG_ALTGR|1<<KG_CTRL"
				 "|1<<KG_ALT|1<<KG_SHIFTL|1<<KG_SHIFTR"
				 "|1<<KG_CTRLL|1<<KG_CTRLR") },
	};

	static const struct arg_val kbval_vecs[] = {
		{ ARG_STR(0) NRAW(" /* K(KT_LATIN, '\\x00') */") },
		{ ARG_STR(0x10) NRAW(" /* K(KT_LATIN, '\\x10') */") },
		{ ARG_STR(0x20) NRAW(" /* K(KT_LATIN, ' ') */") },
		{ ARG_STR(0x7e) NRAW(" /* K(KT_LATIN, '~') */") },
		{ ARG_STR(0x7f) NRAW(" /* K(KT_LATIN, '\\x7f') */") },

		{ ARG_STR(0x100) NRAW(" /* K_F1 */") },
		{ ARG_STR(0x11d) NRAW(" /* K_PAUSE */") },
		{ ARG_STR(0x1ff) NRAW(" /* K_UNDO */") },

		{ ARG_STR(0x200) NRAW(" /* K_HOLE */") },
		{ ARG_STR(0x213) NRAW(" /* K_BARENUMLOCK */") },
		{ ARG_STR(0x214) NRAW(" /* K(KT_SPEC, 0x14) */") },
		{ ARG_STR(0x27d) NRAW(" /* K(KT_SPEC, 0x7d) */") },
		{ ARG_STR(0x27e) NRAW(" /* K_ALLOCATED */") },
		{ ARG_STR(0x27f) NRAW(" /* K_NOSUCHMAP */") },
		{ ARG_STR(0x280) NRAW(" /* K(KT_SPEC, 0x80) */") },
		{ ARG_STR(0x2ff) NRAW(" /* K(KT_SPEC, 0xff) */") },

		{ ARG_STR(0x300) NRAW(" /* K_P0 */") },
		{ ARG_STR(0x313) NRAW(" /* K_PPARENR */") },
		{ ARG_STR(0x314) NRAW(" /* K(KT_PAD, 0x14) */") },
		{ ARG_STR(0x37f) NRAW(" /* K(KT_PAD, 0x7f) */") },
		{ ARG_STR(0x3ff) NRAW(" /* K(KT_PAD, 0xff) */") },

		{ ARG_STR(0x400) NRAW(" /* K_DGRAVE */") },
		{ ARG_STR(0x41a) NRAW(" /* K_DGREEK */") },
		{ ARG_STR(0x41b) NRAW(" /* K(KT_DEAD, 0x1b) */") },
		{ ARG_STR(0x47f) NRAW(" /* K(KT_DEAD, 0x7f) */") },
		{ ARG_STR(0x4ff) NRAW(" /* K(KT_DEAD, 0xff) */") },

		{ ARG_STR(0x500) NRAW(" /* K(KT_CONS, 0) */") },
		{ ARG_STR(0x540) NRAW(" /* K(KT_CONS, 0x40) */") },
		{ ARG_STR(0x5ff) NRAW(" /* K(KT_CONS, 0xff) */") },

		{ ARG_STR(0x600) NRAW(" /* K_DOWN */") },
		{ ARG_STR(0x603) NRAW(" /* K_UP */") },
		{ ARG_STR(0x604) NRAW(" /* K(KT_CUR, 0x4) */") },
		{ ARG_STR(0x680) NRAW(" /* K(KT_CUR, 0x80) */") },
		{ ARG_STR(0x6ff) NRAW(" /* K(KT_CUR, 0xff) */") },

		{ ARG_STR(0x700) NRAW(" /* K_SHIFT */") },
		{ ARG_STR(0x708) NRAW(" /* K_CAPSSHIFT */") },
		{ ARG_STR(0x709) NRAW(" /* K(KT_SHIFT, 0x9) */") },
		{ ARG_STR(0x7ff) NRAW(" /* K(KT_SHIFT, 0xff) */") },

		{ ARG_STR(0x800) NRAW(" /* K(KT_META, '\\x00') */") },
		{ ARG_STR(0x841) NRAW(" /* K(KT_META, 'A') */") },
		{ ARG_STR(0x8ff) NRAW(" /* K(KT_META, '\\xff') */") },

		{ ARG_STR(0x900) NRAW(" /* K_ASC0 */") },
		{ ARG_STR(0x909) NRAW(" /* K_ASC9 */") },
		{ ARG_STR(0x90a) NRAW(" /* K_HEX0 */") },
		{ ARG_STR(0x919) NRAW(" /* K_HEXf */") },
		{ ARG_STR(0x91a) NRAW(" /* K(KT_ASCII, 0x1a) */") },
		{ ARG_STR(0x9ff) NRAW(" /* K(KT_ASCII, 0xff) */") },

		{ ARG_STR(0xa00) NRAW(" /* K_SHIFTLOCK */") },
		{ ARG_STR(0xa08) NRAW(" /* K_CAPSSHIFTLOCK */") },
		{ ARG_STR(0xa09) NRAW(" /* K(KT_LOCK, 0x9) */") },
		{ ARG_STR(0xaff) NRAW(" /* K(KT_LOCK, 0xff) */") },

		{ ARG_STR(0xb00) NRAW(" /* K(KT_LETTER, '\\x00') */") },
		{ ARG_STR(0xb40) NRAW(" /* K(KT_LETTER, '@') */") },
		{ ARG_STR(0xb7f) NRAW(" /* K(KT_LETTER, '\\x7f') */") },
		{ ARG_STR(0xbff) NRAW(" /* K(KT_LETTER, '\\xff') */") },

		{ ARG_STR(0xc00) NRAW(" /* K_SHIFT_SLOCK */") },
		{ ARG_STR(0xc08) NRAW(" /* K_CAPSSHIFT_SLOCK */") },
		{ ARG_STR(0xc09) NRAW(" /* K(KT_SLOCK, 0x9) */") },
		{ ARG_STR(0xcff) NRAW(" /* K(KT_SLOCK, 0xff) */") },

		{ ARG_STR(0xd00) NRAW(" /* K(KT_DEAD2, '\\x00') */") },
		{ ARG_STR(0xd13) NRAW(" /* K(KT_DEAD2, '\\x13') */") },
		{ ARG_STR(0xd5c) NRAW(" /* K(KT_DEAD2, '\\\\') */") },
		{ ARG_STR(0xdff) NRAW(" /* K(KT_DEAD2, '\\xff') */") },

		{ ARG_STR(0xe00) NRAW(" /* K_BRL_BLANK */") },
		{ ARG_STR(0xe0a) NRAW(" /* K_BRL_DOT10 */") },
		{ ARG_STR(0xe0b) NRAW(" /* K(KT_BRL, 0xb) */") },
		{ ARG_STR(0xeff) NRAW(" /* K(KT_BRL, 0xff) */") },

		{ ARG_STR(0xf00) NRAW(" /* K(0xf, 0) */") },
		{ ARG_STR(0xfed) NRAW(" /* K(0xf, 0xed) */") },
		{ ARG_STR(0xf00d) NRAW(" /* K(0xf0, 0xd) */") },
	};

	struct kbentry *kbe = tail_alloc(sizeof(*kbe));

	sys_ioctl(-1, KDGKBENT, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGKBENT));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGKBENT, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGKBENT));
	}

	sys_ioctl(-1, KDGKBENT, (uintptr_t) (kbe + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGKBENT), kbe + 1);

	kbe->kb_value = 0xa8a8;
	sys_ioctl(-1, KDGKBENT, (uintptr_t) kbe + 2);
	printf("ioctl(-1, " IOC_FMT ", {kb_table=%s, kb_index=168"
#if RETVAL_INJECTED
	       ", kb_value=???"
#endif
	       "}) = " RETVAL,
	       IOC_ARG(KDGKBENT),
	       XLAT_STR(1<<KG_ALT|1<<KG_SHIFTR|1<<KG_CTRLR));

	for (size_t i = 0;
	     i < MAX(ARRAY_SIZE(kbtbl_vecs), ARRAY_SIZE(kbval_vecs)); i++) {
		kbe->kb_table = kbtbl_vecs[i % ARRAY_SIZE(kbtbl_vecs)].val;
		kbe->kb_index = i * 3141;
		kbe->kb_value = kbval_vecs[i % ARRAY_SIZE(kbval_vecs)].val;

		rc = sys_ioctl(-1, KDGKBENT, (uintptr_t) kbe);
		saved_errno = errno;
		printf("ioctl(-1, " IOC_FMT ", {kb_table=%s, kb_index=%u",
		       IOC_ARG(KDGKBENT),
		       kbtbl_vecs[i % ARRAY_SIZE(kbtbl_vecs)].str,
		       kbe->kb_index);
		if (rc >= 0) {
			printf(", kb_value=%s",
			       kbval_vecs[i % ARRAY_SIZE(kbval_vecs)].str);
		}
		errno = saved_errno;
		printf("}) = " RETVAL);
	}


	/* KDSKBENT */
	sys_ioctl(-1, KDSKBENT, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDSKBENT));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDSKBENT, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDSKBENT));
	}

	sys_ioctl(-1, KDSKBENT, (uintptr_t) (kbe + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDSKBENT), kbe + 1);

	kbe->kb_value = 0xa8a8;
	sys_ioctl(-1, KDSKBENT, (uintptr_t) kbe + 2);
	printf("ioctl(-1, " IOC_FMT ", {kb_table=%s, kb_index=168"
	       ", kb_value=???}) = " RETVAL,
	       IOC_ARG(KDSKBENT),
	       XLAT_STR(1<<KG_ALT|1<<KG_SHIFTR|1<<KG_CTRLR));

	for (size_t i = 0;
	     i < MAX(ARRAY_SIZE(kbtbl_vecs), ARRAY_SIZE(kbval_vecs)); i++) {
		kbe->kb_table =
			kbtbl_vecs[(i + 5) % ARRAY_SIZE(kbtbl_vecs)].val;
		kbe->kb_index = i * 3141;
		kbe->kb_value = kbval_vecs[i % ARRAY_SIZE(kbval_vecs)].val;

		rc = sys_ioctl(-1, KDSKBENT, (uintptr_t) kbe);
		printf("ioctl(-1, " IOC_FMT ", {kb_table=%s, kb_index=%u"
		       ", kb_value=%s}) = " RETVAL,
		       IOC_ARG(KDSKBENT),
		       kbtbl_vecs[(i + 5) % ARRAY_SIZE(kbtbl_vecs)].str,
		       kbe->kb_index,
		       kbval_vecs[i % ARRAY_SIZE(kbval_vecs)].str);
	}


	/* KDGKBSENT */
	static struct arg_val kbse_offsets[] = {
		{ sizeof(struct kbsentry) - 1, "KVAL(K_F2)" },
		{ sizeof(struct kbsentry) - 2, "KVAL(K_F1)" },
		{ sizeof(struct kbsentry) - 34, "KVAL(K_F214)" },
		{ sizeof(struct kbsentry) - 35, "KVAL(K_F213)" },
		{ 1, "KVAL(K_F1)" },
		{ 0, "KVAL(K_F245)" },
	};

	static const struct arg_val kbfn_vecs[] = {
		{ ARG_XLAT_KNOWN(0, "KVAL(K_F1)") },
		{ ARG_XLAT_KNOWN(0x10, "KVAL(K_F17)") },
		{ ARG_XLAT_KNOWN(0x7f, "KVAL(K_F118)") },
		{ ARG_XLAT_KNOWN(0xff, "KVAL(K_UNDO)") },
	};

	struct kbsentry *kbse = tail_alloc(sizeof(*kbse));

	fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string), 0, 0xff);
	kbse->kb_func = 0xfe;

	sys_ioctl(-1, KDGKBSENT, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDGKBSENT));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDGKBSENT, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDGKBSENT));
	}

	sys_ioctl(-1, KDGKBSENT, (uintptr_t) (kbse + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDGKBSENT), kbse + 1);


	for (size_t i = 0; i < ARRAY_SIZE(kbse_offsets); i++) {
		sys_ioctl(-1, KDGKBSENT,
			  (uintptr_t) kbse + kbse_offsets[i].val);
		saved_errno = errno;
		printf("ioctl(-1, " IOC_FMT ", {kb_func=%s",
	               IOC_ARG(KDGKBSENT),
		       sprintxlat(kbse_offsets[i].str,
				  (kbse_offsets[i].val + 254) % 0xff, NULL));
#if RETVAL_INJECTED
		printf(", kb_string=");
		if (kbse_offsets[i].val < 255 * 2) {
			print_quoted_stringn((char *) kbse->kb_string +
					     kbse_offsets[i].val,
					     MIN(DEFAULT_STRLEN,
						 sizeof(kbse->kb_string)));
		} else {
			printf("???");
		}
#endif
		errno = saved_errno;
		printf("}) = " RETVAL);
	}

	fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string), 0x80, 0x7f);
	kbse->kb_func = KVAL(K_PGDN);

	sys_ioctl(-1, KDGKBSENT, (uintptr_t) kbse);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", {kb_func="
	       XLAT_KNOWN(0x19, "KVAL(K_PGDN)"),
               IOC_ARG(KDGKBSENT));
#if RETVAL_INJECTED
	printf(", kb_string=");
	print_quoted_stringn((char *) kbse->kb_string,
			     MIN(DEFAULT_STRLEN, sizeof(kbse->kb_string)));
#endif
	errno = saved_errno;
	printf("}) = " RETVAL);

	for (size_t i = 0; i < ARRAY_SIZE(kbfn_vecs); i++) {
		kbse->kb_func = kbfn_vecs[i].val;
		fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string),
			       i * 357 + 42, i * 257 + 13);

		rc = sys_ioctl(-1, KDGKBSENT, (uintptr_t) kbse);
		saved_errno = errno;
		printf("ioctl(-1, " IOC_FMT ", {kb_func=%s",
		       IOC_ARG(KDGKBSENT),
		       kbfn_vecs[i].str);
#if RETVAL_INJECTED
		printf(", kb_string=");
		print_quoted_stringn((char *) kbse->kb_string,
				     MIN(DEFAULT_STRLEN,
					 sizeof(kbse->kb_string)));
#endif
		errno = saved_errno;
		printf("}) = " RETVAL);
	}


	/* KDSKBSENT */
	fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string), 0, 0xff);
	kbse->kb_func = 0xfe;

	sys_ioctl(-1, KDSKBSENT, 0);
	printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL, IOC_ARG(KDSKBSENT));

	if (F8ILL_KULONG_SUPPORTED) {
		sys_ioctl(-1, KDSKBSENT, F8ILL_KULONG_MASK);
		printf("ioctl(-1, " IOC_FMT ", NULL) = " RETVAL,
		       IOC_ARG(KDSKBSENT));
	}

	sys_ioctl(-1, KDSKBSENT, (uintptr_t) (kbse + 1));
	printf("ioctl(-1, " IOC_FMT ", %p) = " RETVAL,
	       IOC_ARG(KDSKBSENT), kbse + 1);


	for (size_t i = 0; i < ARRAY_SIZE(kbse_offsets); i++) {
		sys_ioctl(-1, KDSKBSENT,
			  (uintptr_t) kbse + kbse_offsets[i].val);
		saved_errno = errno;
		printf("ioctl(-1, " IOC_FMT ", {kb_func=%s, kb_string=",
	               IOC_ARG(KDSKBSENT),
		       sprintxlat(kbse_offsets[i].str,
				  (kbse_offsets[i].val + 254) % 0xff, NULL));
		if (kbse_offsets[i].val < 255 * 2) {
			print_quoted_stringn((char *) kbse->kb_string +
					     kbse_offsets[i].val,
					     MIN(DEFAULT_STRLEN,
						 sizeof(kbse->kb_string)));
		} else {
			printf("???");
		}
		errno = saved_errno;
		printf("}) = " RETVAL);
	}

	fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string), 0x80, 0x7f);
	kbse->kb_func = KVAL(K_PGDN);

	sys_ioctl(-1, KDSKBSENT, (uintptr_t) kbse);
	saved_errno = errno;
	printf("ioctl(-1, " IOC_FMT ", {kb_func="
	       XLAT_KNOWN(0x19, "KVAL(K_PGDN)") ", kb_string=",
               IOC_ARG(KDSKBSENT));
	print_quoted_stringn((char *) kbse->kb_string,
			     MIN(DEFAULT_STRLEN, sizeof(kbse->kb_string)));
	errno = saved_errno;
	printf("}) = " RETVAL);

	for (size_t i = 0; i < ARRAY_SIZE(kbfn_vecs); i++) {
		kbse->kb_func = kbfn_vecs[i].val;
		fill_memory_ex(kbse->kb_string, sizeof(kbse->kb_string),
			       i * 357 + 42, i * 257 + 13);

		rc = sys_ioctl(-1, KDSKBSENT, (uintptr_t) kbse);
		saved_errno = errno;
		printf("ioctl(-1, " IOC_FMT ", {kb_func=%s, kb_string=",
		       IOC_ARG(KDSKBSENT),
		       kbfn_vecs[i].str);
		print_quoted_stringn((char *) kbse->kb_string,
				     MIN(DEFAULT_STRLEN,
					 sizeof(kbse->kb_string)));
		errno = saved_errno;
		printf("}) = " RETVAL);
	}


	/* KDGKBDIACR */
	/* KDSKBDIACR */
	/* KDGETKEYCODE */
	/* KDSETKEYCODE */
	/* KDSIGACCEPT */
	/* KDKBDREP */
	/* GIO_FONT */
	/* PIO_FONT */
	/* KDGKBMETA */
	/* KDSKBMETA */
	/* KDGKBLED */
	/* KDSKBLED */
	/* GIO_UNIMAP */
	/* PIO_UNIMAP */
	/* PIO_UNIMAPCLR */
	/* GIO_UNISCRNMAP */
	/* PIO_UNISCRNMAP */
	/* GIO_FONTX */
	/* PIO_FONTX */
	/* PIO_FONTRESET */
	/* GIO_CMAP */
	/* PIO_CMAP */
	/* KDFONTOP */
	/* KDGKBDIACRUC */
	/* KDSKBDIACRUC */

	puts("+++ exited with 0 +++");
	return 0;
}
