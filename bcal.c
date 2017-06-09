/*
 * Byte CALculator
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
 * Copyright (C) 2016 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bcal.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <quadmath.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dslib.h"

#define TRUE 1
#define FALSE !TRUE

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */
#define UINT_BUF_LEN 40 /* log10(1 << 128) + '\0' */
#define FLOAT_BUF_LEN 128
#define FLOAT_WIDTH 40

#ifdef __SIZEOF_INT128__
typedef __uint128_t maxuint_t;
typedef __float128 maxfloat_t;
#else
typedef __uint64_t maxuint_t;
typedef double maxfloat_t;
#endif

typedef unsigned char bool;
typedef unsigned long ulong;
typedef unsigned long long ull;

typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

char *VERSION = "1.5";
char *units[] = {"b", "kib", "mib", "gib", "tib", "kb", "mb", "gb", "tb"};

char uint_buf[UINT_BUF_LEN];
char float_buf[FLOAT_BUF_LEN];

void binprint(maxuint_t n)
{
	int count = 127;
	char binstr[129] = {0};

	if (!n) {
		fprintf(stdout, "0b0");
		return;
	}

	while (n && count >= 0) {
		binstr[count--] = "01"[n & 1];
		n >>= 1;
	}

	count++;

	fprintf(stdout, "0b%s", binstr + count);
}

char *getstr_u128(maxuint_t n, char *buf) {
	if (n == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return buf;
	}

	memset(buf, 0, UINT_BUF_LEN);
	char *loc = buf + UINT_BUF_LEN - 1; /* start at the end */

	while (n != 0) {
		if (loc == buf)
			return NULL; /* should not happen */

		*--loc = "0123456789"[n % 10]; /* save the last digit */
		n /= 10; /* drop the last digit */
	}

	return loc;
}

char *getstr_f128(maxfloat_t val, char *buf)
{
	int n = quadmath_snprintf(buf, FLOAT_BUF_LEN, "%#*.10Qe", FLOAT_WIDTH, val);
	buf[n] = '\0';
	return buf;
}

void printval(maxfloat_t val, char *unit)
{
	if (val - (maxuint_t)val == 0)
		fprintf(stdout, "%40s %s\n", getstr_u128((maxuint_t)val, uint_buf), unit);
	else
		fprintf(stdout, "%s %s\n", getstr_f128(val, float_buf), unit);
}

void printhex_u128(maxuint_t n)
{
	ull high = (ull)(n >> (sizeof(maxuint_t) << 2));

	if (high)
		fprintf(stdout, "0x%llx%llx", high, (ull)n);
	else
		fprintf(stdout, "0x%llx", (ull)n);
}

char *strtolower(char *buf)
{
	char *p = buf;

	for (; *p; ++p)
		*p = tolower(*p);

	return buf;
}

/* This function adds check for binary input to strtoul() */
ulong strtoul_b(char *token)
{
	int base = 0;

	/* NOTE: no NULL check here! */

	if (strlen(token) > 2 && token[0] == '0' &&
			(token[1] == 'b' || token[1] == 'B')) {
		base = 2;
	}

	return strtoul(token + base, NULL, base);
}

/* This function adds check for binary input to strtoull() */
ull strtoull_b(char *token)
{
	int base = 0;

	/* NOTE: no NULL check here! */

	if (strlen(token) > 2 && token[0] == '0' &&
			(token[1] == 'b' || token[1] == 'B')) {
		base = 2;
	}

	return strtoull(token + base, NULL, base);
}

maxuint_t convertbyte(char *buf)
{
	/* Convert and print in bytes */
	maxuint_t bytes = strtoull(buf, NULL, 0);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	/* Convert and print in IEC standard units */

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = bytes / (maxfloat_t)1024;
	printval(val, "KiB");

	val = bytes / (maxfloat_t)(1 << 20);
	printval(val, "MiB");

	val = bytes / (maxfloat_t)(1 << 30);
	printval(val, "GiB");

	val = bytes / (maxfloat_t)((maxuint_t)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = bytes / (maxfloat_t)1000;
	printval(val, "kB");

	val = bytes / (maxfloat_t)1000000;
	printval(val, "MB");

	val = bytes / (maxfloat_t)1000000000;
	printval(val, "GB");

	val = bytes / (maxfloat_t)1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertkib(char *buf)
{
	maxfloat_t kib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(kib * 1024);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	printval(kib, "KiB");

	maxfloat_t val = kib / 1024;
	printval(val, "MiB");

	val = kib / (1 << 20);
	printval(val, "GiB");

	val = kib / (1 << 30);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = kib * 1024 / 1000;
	printval(val, "kB");

	val = kib * 1024 / 1000000;
	printval(val, "MB");

	val = kib * 1024 / 1000000000;
	printval(val, "GB");

	val = kib * 1024 / 1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertmib(char *buf)
{
	maxfloat_t mib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(mib * (1 << 20));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = mib * 1024;
	printval(val, "KiB");

	printval(mib, "MiB");

	val = mib / 1024;
	printval(val, "GiB");

	val = mib / (1 << 20);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = mib * (1 << 20)/ 1000;
	printval(val, "kB");

	val = mib * (1 << 20) / 1000000;
	printval(val, "MB");

	val = mib * (1 << 20) / 1000000000;
	printval(val, "GB");

	val = mib * (1 << 20) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertgib(char *buf)
{
	maxfloat_t gib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(gib * (1 << 30));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = gib * (1 << 20);
	printval(val, "KiB");

	val = gib * 1024;
	printval(val, "MiB");

	printval(gib, "GiB");

	val = gib / 1024;
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = gib * (1 << 30)/ 1000;
	printval(val, "kB");

	val = gib * (1 << 30) / 1000000;
	printval(val, "MB");

	val = gib * (1 << 30) / 1000000000;
	printval(val, "GB");

	val = gib * (1 << 30) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t converttib(char *buf)
{
	maxfloat_t tib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(tib * ((maxuint_t)1 << 40));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = tib * (1 << 30);
	printval(val, "KiB");

	val = tib * (1 << 20);
	printval(val, "MiB");

	val = tib * 1024;
	printval(val, "GiB");

	printval(tib, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = tib * ((maxuint_t)1 << 40)/ 1000;
	printval(val, "kB");

	val = tib * ((maxuint_t)1 << 40) / 1000000;
	printval(val, "MB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000;
	printval(val, "GB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertkb(char *buf)
{
	maxfloat_t kb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(kb * 1000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = kb * 1000 / 1024;
	printval(val, "KiB");

	val = kb * 1000 / (1 << 20);
	printval(val, "MiB");

	val = kb * 1000 / (1 << 30);
	printval(val, "GiB");

	val = kb * 1000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	printval(kb, "kB");

	val = kb / 1000;
	printval(val, "MB");

	val = kb / 1000000;
	printval(val, "GB");

	val = kb / 1000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertmb(char *buf)
{
	maxfloat_t mb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(mb * 1000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = mb * 1000000 / 1024;
	printval(val, "KiB");

	val = mb * 1000000 / (1 << 20);
	printval(val, "MiB");

	val = mb * 1000000 / (1 << 30);
	printval(val, "GiB");

	val = mb * 1000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = mb * 1000;
	printval(val, "kB");

	printval(mb, "MB");

	val = mb / 1000;
	printval(val, "GB");

	val = mb / 1000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertgb(char *buf)
{
	maxfloat_t gb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(gb * 1000000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = gb * 1000000000 / 1024;
	printval(val, "KiB");

	val = gb * 1000000000 / (1 << 20);
	printval(val, "MiB");

	val = gb * 1000000000 / (1 << 30);
	printval(val, "GiB");

	val = gb * 1000000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = gb * 1000000;
	printval(val, "kB");

	val = gb * 1000;
	printval(val, "MB");

	printval(gb, "GB");

	val = gb / 1000;
	printval(val, "TB");

	return bytes;
}

maxuint_t converttb(char *buf)
{
	maxfloat_t tb = strtod(buf, NULL);

	maxuint_t bytes = (__uint128_t)(tb * 1000000000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = tb * 1000000000000 / 1024;
	printval(val, "KiB");

	val = tb * 1000000000000 / (1 << 20);
	printval(val, "MiB");

	val = tb * 1000000000000 / (1 << 30);
	printval(val, "GiB");

	val = tb * 1000000000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = tb * 1000000000;
	printval(val, "kB");

	val = tb * 1000000;
	printval(val, "MB");

	val = tb * 1000;
	printval(val, "GB");

	printval(tb, "TB");

	return bytes;
}

bool chs2lba(char *chs, maxuint_t *lba)
{
	int token_no = 0;
	char *ptr, *token;
	ulong chsparam[5] = {0, 0, 0, MAX_HEAD, MAX_SECTOR};

	ptr = token = chs;

	while (*ptr && token_no < 5) {
		if (*ptr == '-') {
			/* Replace '-' with NULL and get the token */
			*ptr = '\0';
			chsparam[token_no++] = strtoul_b(token);
			/* Restore the '-' */
			*ptr++ = '-';
			/* Point to start of next token */
			token = ptr;

			if (*ptr == '\0' && token_no < 5)
				chsparam[token_no++] = strtoul_b(token);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 5)
			chsparam[token_no++] = strtoul_b(token);
	}

	/* Fail if CHS is omitted */
	if (token_no < 3) {
		fprintf(stderr, "CHS2LBA: CHS missing\n");
		return FALSE;
	}

	if (!chsparam[3]) {
		fprintf(stderr, "CHS2LBA: MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!chsparam[4]) {
		fprintf(stderr, "CHS2LBA: MAX_SECTOR = 0\n");
		return FALSE;
	}

	if (!chsparam[2]) {
		fprintf(stderr, "CHS2LBA: S = 0\n");
		return FALSE;
	}

	if (chsparam[1] > chsparam[3]) {
		fprintf(stderr, "CHS2LBA: H > MAX_HEAD\n");
		return FALSE;
	}

	if (chsparam[2] > chsparam[4]) {
		fprintf(stderr, "CHS2LBA: S > MAX_SECTOR\n");
		return FALSE;
	}


	*lba = chsparam[3] * chsparam[4] * chsparam[0]; /* MH * MS * C */
	*lba += chsparam[4] * chsparam[1]; /* MS * H */

	*lba += chsparam[2] - 1; /* S - 1 */

	fprintf(stdout, "\033[1mCHS2LBA\033[0m\n\tC %lu H %lu S %lu MAX_HEAD %lu MAX_SECTOR %lu\n",
		chsparam[0], chsparam[1], chsparam[2], chsparam[3], chsparam[4]);

	return TRUE;
}

bool lba2chs(char *lba, t_chs *p_chs)
{
	int token_no = 0;
	char *ptr, *token;
	maxuint_t chsparam[3] = {0, MAX_HEAD, MAX_SECTOR};

	ptr = token = lba;

	while (*ptr && token_no < 3) {
		if (*ptr == '-') {
			*ptr = '\0';
			chsparam[token_no++] = strtoull_b(token);
			*ptr++ = '-';
			token = ptr;

			if (*ptr == '\0' && token_no < 3)
				chsparam[token_no++] = strtoull_b(token);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 3)
			chsparam[token_no++] = strtoull_b(token);
	}

	/* Fail if LBA is omitted */
	if (!token_no) {
		fprintf(stderr, "LBA2CHS: LBA missing\n");
		return FALSE;
	}

	if (!chsparam[1]) {
		fprintf(stderr, "LBA2CHS: MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!chsparam[2]) {
		fprintf(stderr, "LBA2CHS: MAX_SECTOR = 0\n");
		return FALSE;
	}

	p_chs->c = (ulong)(chsparam[0] / (chsparam[2] * chsparam[1])); /* L / (MS * MH) */

	p_chs->h = (ulong)((chsparam[0] / chsparam[2]) % chsparam[1]); /* (L / MS) % MH */
	if (p_chs->h > MAX_HEAD) {
		fprintf(stderr, "LBA2CHS: H > MAX_HEAD\n");
		return FALSE;
	}

	p_chs->s = (ulong)((chsparam[0] % chsparam[2]) + 1); /* (L % MS) + 1 */
	if (p_chs->s > MAX_SECTOR) {
		fprintf(stderr, "LBA2CHS: S > MAX_SECTOR\n");
		return FALSE;
	}

	fprintf(stdout, "\033[1mLBA2CHS\033[0m\n\tLBA %s ",
		getstr_u128(chsparam[0], uint_buf));
	fprintf(stdout, "MAX_HEAD %s ", getstr_u128(chsparam[1], uint_buf));
	fprintf(stdout, "MAX_SECTOR %s\n", getstr_u128(chsparam[2], uint_buf));

	return TRUE;
}

void usage()
{
	fprintf(stdout, "usage: bcal [-c N] [-f FORMAT] [-s bytes] [-h]\n\
	    [expression] [N unit] \n\n\
Perform storage conversions and calculations.\n\n\
positional arguments:\n\
  expression       evaluate storage arithmetic expression\n\
		   +, -, *, / with decimal inputs supported\n\
		   unit can be multipled or divided by +ve integer(s)\n\
		   units can be added or subtracted from each other\n\
		   Examples:\n\
		       bcal \"(5kb+2mb)/3\"\n\
		       bcal \"5 tb / 12\"\n\
		       bcal \"2.5mb*3\"\n\
  N unit           capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB\n\
		   see https://wiki.ubuntu.com/UnitsPolicy\n\
		   must be space-separated, case is ignored\n\
		   N can be decimal or '0x' prefixed hex value\n\n\
optional arguments:\n\
  -c N             show N in binary, decimal and hex\n\
  -f FORMAT        convert CHS to LBA or LBA to CHS\n\
		   formats are hyphen-separated\n\
		   LBA format:\n\
		       starts with 'l':\n\
		       lLBA-MAX_HEAD-MAX_SECTOR\n\
		   CHS format:\n\
		       starts with 'c':\n\
		       cC-H-S-MAX_HEAD-MAX_SECTOR\n\
		   omitted values are considered 0\n\
		   FORMAT 'c-50--0x12-' denotes:\n\
		     C = 0, H = 50, S = 0, MH = 0x12, MS = 0\n\
		   FORMAT 'l50-0x12' denotes:\n\
		     LBA = 50, MH = 0x12, MS = 0\n\
		   default MAX_HEAD: 16, default MAX_SECTOR: 63\n\
  -s bytes         sector size [default 512]\n\
  -h               show this help and exit\n\n\
Version %s\n\
Copyright © 2016-2017 Arun Prakash Jana <engineerarun@gmail.com>\n\
License: GPLv3\n\
Webpage: https://github.com/jarun/bcal\n", VERSION);
}

int xstricmp(const char *s1, const char *s2)
{
	while (*s1 && (tolower(*s1) == tolower(*s2))) {
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Convert any unit in bytes
 * Failure if out parameter holds -1
 */
maxuint_t unitconv(Data bunit, short *isunit, int *out)
{
	/* Data is a C structure containing a string p and a short indicating if the string is a unit or a plain number */
	char *numstr = bunit.p;
	*out = 0;

	if (numstr == NULL || *numstr == '\0') {
		fprintf(stderr, "unitconv: Invalid token\n");
		*out = -1;
		return 0;
	}

	int len = strlen(numstr) - 1;

	if (isdigit(numstr[len])) {
		if (*isunit != 1) /* ensure this is not the result of a previous operation */
			*isunit = 0;
		return strtoull(numstr, NULL, 0);
	}

	while (isalpha(numstr[len]))
		len--;

	char *punit = numstr + len + 1;

	int count = sizeof(units) / sizeof(units[0]);	/* Number of available units is 9 (from "b" to "tb"). */
	while (--count >= 0)
		if (!xstricmp(units[count], punit))
                        break;

	if (count == -1) {
		fprintf(stderr, "unitconv: No matching unit\n");
		*out = -1;
		return 0;
	}

	*isunit = 1;
	*punit = '\0';

	maxfloat_t byte_metric = 0;

	switch (count) {
	case 0:
		return strtoull(numstr, NULL, 0);
	case 1:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * 1024);	/* Kibibyte */
	case 2:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * (1 << 20));	/* Mebibyte */
	case 3:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * (1 << 30));	/* Gibibyte */
	case 4:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * ((maxuint_t)1 << 40)); /* Tebibyte */
	case 5:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * 1000);	/* Kilobyte */
	case 6:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * 1000000);	/* Megabyte */
	case 7:
		byte_metric = strtod(numstr, NULL);
		return (maxuint_t)(byte_metric * 1000000000);	/* Gigabyte */
	case 8:
		byte_metric = strtod(numstr, NULL);
		return (__uint128_t)(byte_metric * 1000000000000);	/* Terabyte */
	default:
		fprintf(stderr, "unitconv: Unknown unit\n");
		*out = -1;
		return 0;
	}
}

int priority(char sign) /* Get the priority of operators */
{
	switch (sign) {
	case '-': return 1;
	case '+': return 2;
	case '*': return 3;
	case '/': return 4;
	}

	return 0;
}

/* Convert Infix mathematical expression to Postfix */
int infix2postfix(char *exp, queue **resf, queue **resr)
{
	stack *op = NULL;			/* Operator Stack */
	char *token = strtok(exp, " ");
	Data tokenData = {"\0", 0};		/* C structure: distinguish between plain data & unit data */
	int balanced = 0;
	Data ct;

	while (token) {
		strcpy(tokenData.p, token);	/* Copy argument to string part of the structure */

		switch (token[0]) {
		case '+':
		case '-':
		case '*':
		case '/':
			if (token[1] != '\0') {
				fprintf(stderr, "infix2postfix: Invalid token terminator\n");
				return -1;
			}

			while (!isempty(op) && top(op)[0] != '(' &&
			       priority(token[0]) <= priority(top(op)[0])) {
				/* Pop from operator stack */
				pop(&op, &ct);
				/* Insert to Queue */
				enqueue(resf, resr, ct);
		        }

			push(&op, tokenData);
			break;
		case '(':
			balanced++;
			push(&op, tokenData);
			break;
		case ')':
			while (!isempty(op) && top(op)[0] != '(') {
				pop(&op, &ct);
				enqueue(resf, resr ,ct);
			}

			pop(&op, &ct);
			balanced--;
			break;
		default:
			/* Enqueue operands */
			enqueue(resf, resr, tokenData);
		}

		token = strtok(NULL, " ");;
	}

	while (!isempty(op)) {
		/* Put remaining elements into the queue */
		pop(&op, &ct);
		enqueue(resf , resr, ct);
	}

	if (balanced != 0) {
		fprintf(stderr, "infix2postfix: Unbalanced expression\n");
		return -1;
	}

	return 0;
}

/* Evaluates Postfix Expression
 * Failure if out parameter holds -1
 */
maxuint_t eval(queue **front, queue **rear, int *out)
{
	stack *est = NULL;
	Data ansdata, arg, raw_a, raw_b;
	*out = 0;

	if (*front == NULL)	/* If Queue is Empty */
		return 0;

	if (*front == *rear) {		/* If only one element in the queue */
		short s = 0;
		dequeue(front, rear, &ansdata);
		return unitconv(ansdata, &s, out);
	}

	while (*front != NULL && *rear != NULL) {
		dequeue(front, rear, &arg);

		if (strlen(arg.p) == 1 && !isdigit(arg.p[0])){ /* Check if arg is an operator */
			pop(&est, &raw_b);			/* Pop data from stack */
			pop(&est, &raw_a);

			maxuint_t b = unitconv(raw_b, &raw_b.unit, out);	/* Convert to integer */
			if (*out == -1)
				return 0;
			maxuint_t a = unitconv(raw_a, &raw_a.unit, out);
			if (*out == -1)
				return 0;

			maxuint_t c = 0;	/* Result data */
			Data raw_c;

			switch (arg.p[0]) {
			case '+':
				if (raw_a.unit && raw_b.unit) { /* Check if both are units */
				  	c = a + b;
					raw_c.unit = 1;
				} else {
					fprintf(stderr, "eval: Unit mismatch in +\n");
					*out = -1;
					emptystack(&est);
					cleanqueue(front,rear);
					return 0;
				}
				break;
			case '-':
				if (raw_a.unit && raw_b.unit) { /* Check if both are units */
					c = a - b;
					raw_c.unit = 1;
				} else {
					fprintf(stderr, "eval: Unit mismatch in -\n");
					*out = -1;
					emptystack(&est);
					cleanqueue(front,rear);
					return 0;
				}
				break;
			case '*':
				if (!raw_a.unit || !raw_b.unit) { /* Check if only one is unit */
					c = a * b;
					raw_c.unit = 1;
				} else {
					fprintf(stderr, "eval: Unit mismatch in *\n");
					*out = -1;
					emptystack(&est);
					cleanqueue(front,rear);
					return 0;
				}
				break;
			case '/':
				if (raw_a.unit && !raw_b.unit) { /* Check if only the dividend is unit */
					c = a / b;
					raw_c.unit = 1;
				} else {
					fprintf(stderr, "eval: Unit mismatch in /\n");
					*out = -1;
					emptystack(&est);
					cleanqueue(front,rear);
					return 0;
				}
				break;
			}

			strcpy(raw_c.p, getstr_u128(c, uint_buf));	/* Convert to string */
			push(&est, raw_c);				/* Put in stack */

		} else
			push(&est, arg);
	}

	pop(&est, &ansdata);
	return strtoull(ansdata.p, NULL, 0);	/* Convert string to integer */
}

/* Check if a char is operator or not */
int isoperator(char c)
{
	switch (c) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '(':
		case ')': return 1;
		default: return 0;
	}
}

/* Check if valid storage arithmetic expression */
int checkexp(char *exp)
{
	while (*exp) {
		if (*exp == 'b' || *exp == 'B')
			return 1;

		exp++;
	}

	return 0;
}

/* Trim all whitespace from both ends */
char *strstrip(char *s)
{
	if (!s || !*s)
		return s;

	size_t len = strlen(s) - 1;

	while (len != 0 && isspace(s[len]))
		len--;
	s[len + 1] = '\0';

	while (*s && isspace(*s))
		s++;

	return s;
}

/* Replace consecutive inner whitespaces with a single space */
void removeinnerspaces(char *s)
{
	char *p = s;

	while (*s != '\0') {
		if (!isspace(*s))
			*p++ = *s;

		s++;
	}

	*p = '\0';
}

/* Make the expression compatible with parsing by
 * inserting/removing space between arguments
 */
char *fixexpr(char *exp)
{
	strstrip(exp);
	removeinnerspaces(exp);

	if (!checkexp(exp))
		return NULL;

	int i = 0, j = 0;
	char *parsed = (char*)malloc(2 * strlen(exp) * sizeof(char));

	while (exp[i] != '\0') {
		if ((isdigit(exp[i]) && isoperator(exp[i + 1])) ||
		    (isoperator(exp[i]) && (isdigit(exp[i + 1]) || isoperator(exp[i + 1]))) ||
		    (isalpha(exp[i]) && isoperator(exp[i + 1]))) {
			parsed[j++] = exp[i];
			parsed[j++] = ' ';
			parsed[j] = exp[i + 1];
		} else
			parsed[j++] = exp[i];

		i++;
	}

	if (parsed[j])
		parsed[++j] = '\0';

	return parsed;
}

int convertunit(char *value, char *unit, ulong sectorsize)
{
	int count = sizeof(units)/sizeof(*units);
	maxuint_t bytes = 0, lba = 0, offset = 0;

	while (count-- > 0)
		if (!strcmp(units[count], strtolower(unit)))
			break;

	if (count == -1) {
		fprintf(stderr, "No matching unit\n");
		return -1;
	}

	fprintf(stdout, "\033[1mUNIT CONVERSION\033[0m\n");

	switch (count) {
	case 0:
		bytes = convertbyte(value);
		break;
	case 1:
		bytes = convertkib(value);
		break;
	case 2:
		bytes = convertmib(value);
		break;
	case 3:
		bytes = convertgib(value);
		break;
	case 4:
		bytes = converttib(value);
		break;
	case 5:
		bytes = convertkb(value);
		break;
	case 6:
		bytes = convertmb(value);
		break;
	case 7:
		bytes = convertgb(value);
		break;
	case 8:
		bytes = converttb(value);
		break;
	default:
		fprintf(stderr, "Unknown unit\n");
		return -1;
	}

	fprintf(stdout, "\n    ADDRESS\n\tdec: %s\n\thex: ", getstr_u128(bytes, uint_buf));
	printhex_u128(bytes);

	/* Calculate LBA and offset */
	lba = bytes / sectorsize;
	offset = bytes % sectorsize;

	fprintf(stdout, "\n\n    LBA:OFFSET\n\tsector size: 0x%lx\n", sectorsize);
	/* We use a global buffer, so print decimal lba first, then offset */
	fprintf(stdout, "\n\tdec: %s:", getstr_u128(lba, uint_buf));
	fprintf(stdout, "%s\n\thex: ", getstr_u128(offset, uint_buf));
	printhex_u128(lba);
	fprintf(stdout, ":");
	printhex_u128(offset);
	fprintf(stdout, "\n");

	return 0;
}

int evaluate(char *exp)
{
	char *expr = fixexpr(exp);	 /* Make parsing compatible */
	if (expr == NULL)
		return -1;

	maxuint_t bytes = 0;
	int ret = 0;

	queue *front = NULL, *rear = NULL;
	ret = infix2postfix(expr, &front, &rear);
	free(expr);
	if (ret == -1)
		return -1;

	bytes = eval(&front, &rear, &ret);  /* Evaluate Expression */
	if (ret == -1)
		return -1;

	fprintf(stdout, "\033[1mRESULT\033[0m\n");

	convertbyte(getstr_u128(bytes, uint_buf));

	fprintf(stdout, "\n    ADDRESS\n\tdec: %s\n\thex: ", getstr_u128(bytes, uint_buf));
	printhex_u128(bytes);
	fprintf(stdout, "\n");

	return 0;
}

int main(int argc, char **argv)
{
	int opt = 0;
	ulong sectorsize = SECTOR_SIZE;

	opterr = 0;

	while ((opt = getopt(argc, argv, "c:f:hs:")) != -1) {
		switch (opt) {
		case 'c':
			if (*optarg == '-') {
				fprintf(stderr, "N must be >= 0\n");
				return 1;
			}

			fprintf(stdout, "\033[1mBASE CONVERSION\033[0m\n");
			maxuint_t val = strtoull_b(optarg);

			fprintf(stdout, "\tbin: ");
			binprint(val);
			fprintf(stdout, "\n\tdec: %s\n\thex: ", getstr_u128(val, uint_buf));
			printhex_u128(val);
			fprintf(stdout, "\n\n");
			break;
		case 'f':
			if (tolower(*optarg) == 'c') {
				maxuint_t lba = 0;
				if (chs2lba(optarg + 1, &lba)) {
					fprintf(stdout, "\tLBA: (dec) %s, (hex) ",
						getstr_u128(lba, uint_buf));
					printhex_u128(lba);
					fprintf(stdout, "\n\n");
				} else
					fprintf(stderr, "Invalid input\n");
			} else if (tolower(*optarg) == 'l') {
				t_chs chs;
				if (lba2chs(optarg + 1, &chs))
					fprintf(stdout, "\tCHS: (dec) %lu %lu %lu, (hex) 0x%lx 0x%lx 0x%lx\n\n",
						chs.c, chs.h, chs.s, chs.c, chs.h, chs.s);
				else
					fprintf(stderr, "Invalid input\n");
			} else
				fprintf(stderr, "Invalid input\n");
			break;
		case 's':
			if (*optarg == '-') {
				fprintf(stderr, "sector size must be +ve\n");
				return 1;
			}
			sectorsize = strtoul_b(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return -1;
		}
	}

	if (argc == 1 && optind == 1) {
		usage();
		return -1;
	}

	/* Conversion */
	if (argc - optind == 2)
		if (convertunit(argv[optind], argv[optind + 1], sectorsize) == -1) {
			usage();
			return -1;
		}

	/*Arithmetic Operation*/
	if (argc - optind == 1)
		if (evaluate(argv[1]) == -1) {
			usage();
			return -1;
		}

	return 0;
}
