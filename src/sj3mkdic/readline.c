/*
 * Copyright (c) 1991-1994  Sony Corporation
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL SONY CORPORATION BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of Sony Corporation
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from Sony Corporation.
 *
 */

/*
 * $SonyRCSfile: readline.c,v $  
 * $SonyRevision: 1.1 $ 
 * $SonyDate: 1994/06/03 08:00:44 $
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

#include "sj_euc.h"
#include "sj3_dict_const.h"

#include "sj3mkdic.h"

static	int	yomi[MAXYOMILENGTH + 1];
static	int	kanji[MAXKANJILENGTH + 1];
static	int	hinsi[MAXHINSINUMBER + 1];
static	int	atr[MAXATRNUMBER + 1];

#define Isblank(c)  ((c) == 0x20 || (c) == 0x09 || (c) == 0xa1a1)
#define Isillegal(c)    ((c) < 0x20)


static void
error(char *s)
{
	if (s)
		fprintf(stderr, "%s\n", s);
	mark_file(stderr);

	exit(1);
}



static int
readchar()
{
	int	c1;
	int	c2;

	c1 = getch();
	if (c1 == EOF)
		return EOF;

	if (c1 != SS3) {
		if (c1 & 0x80) {
			c2 = getch();
			if (c2 == EOF)
				error(NULL);
			c1 = ((c1 & 0xff) << 8) + (c2 & 0xff);
		}
	} else {
		c2 = getch();
		if (c2 == EOF)
			error(NULL);
		c1 = ((c1 & 0xff) << 8) + (c2 & 0xff);
		c2 = getch();
		if (c2 == EOF)
			error(NULL);
		c1 = ((c1 & 0xffff) << 8) + (c2 & 0xff);
	}
	return c1;
}



static int
skip_blank()
{
	int	c;

	c = readchar();
	while (Isblank(c))
		c = readchar();

	return c;
}



static int
readhinsi()
{
	static int	c = 0;
	int	i;
	unsigned char	hinsi[128];

retry:
	if (c == '\n') {
		c = 0;
		return 0;
	}

	c = skip_blank();
	if (c == '\n') {
		c = 0;
		return 0;
	}

	for (i = 0 ; !Isblank(c) ; ) {
		if (c == '\n' || c == ':')
			break;
		if (Isillegal(c))
			error(ILLEGALFORMAT);
                if (c > 0xffffff) {
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 24) & 0xff);
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 16) & 0xff);
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 8) & 0xff);
		} else if (c > 0xffff) {
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 16) & 0xff);
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 8) & 0xff);
		} else if (c > 0xff) {
			if (i >= 126)
				error(TOOLONGHINSI);
			hinsi[i++] = ((c >> 8) & 0xff);
		}
		if (i >= 127)
			error(TOOLONGHINSI);
		hinsi[i++] = (c & 0xff);
		c = readchar();
	}

	if (i == 0)
		return 0;

	if (hinsi[0] == '[' && hinsi[i - 1] == ']') {
		hinsi[i - 1] = 0;
		i = cnvhinsi(hinsi + 1);
		if (i > 0) {
			fprintf(stderr, "\311\312\273\354 \"%s\" \244\313\263\347\270\314\244\254\244\304\244\244\244\306\244\244\244\353\n",
				hinsi + 1);
		} else if (!i) {
			fprintf(stderr, "\"%s\" \244\254\245\263\241\274\245\311\262\275\244\307\244\255\244\336\244\273\244\363\n",
				hinsi + 1);
			goto retry;
		}
	}
	else {
		hinsi[i] = 0;
		i = cnvhinsi(hinsi);
		if (!i) {
			fprintf(stderr, "\"%s\" \244\254\245\263\241\274\245\311\262\275\244\307\244\255\244\336\244\273\244\363\n", hinsi);
			goto retry;
		}
	}

	return i;
}



int *
readline()
{
	int	c;
	int	i;
	int	j;

	for ( ; ; ) {
		mark_file(NULL);

		c = skip_blank();

		if (c == '#') {
			while (c != '\n')
				c = skip_blank();
		}

		if (c == EOF)
			return NULL;

		if (c != '\n')
			break;
	}

	for (i = 0 ; !Isblank(c) ; ) {
		if (Isillegal(c))
			error(ILLEGALFORMAT);
		if (i >= MAXYOMILENGTH)
			error(TOOLONGYOMI);
		yomi[i++] = c;
		c = readchar();
	}
	yomi[i] = 0;
	if (i == 0)
		error(NOYOMISTRING);

	c = skip_blank();

	if (c != '"') {
		for (i = 0 ; !Isblank(c) ; ) {
			if (Isillegal(c))
				error(ILLEGALFORMAT);
			if (i >= MAXKANJILENGTH)
				error(TOOLONGKANJI);
			kanji[i++] = c;
			c = readchar();
		}
	} else {
		c = readchar();
		for (i = 0 ; c != '"' ; ) {
			if (Isillegal(c))
				error(ILLEGALFORMAT);
			if (i >= MAXKANJILENGTH)
				error(TOOLONGKANJI);
			kanji[i++] = c;
			c = readchar();
		}
	}
	kanji[i] = 0;
	if (i == 0)
		error(NOKANJISTRING);

	i = j = 0;
	while (c = readhinsi()) {
		if (c < 0) {
			if (i >= MAXATRNUMBER)
				error(TOOMANYATR);
			atr[i++] = -c;
		}
		else {
			if (j >= MAXHINSINUMBER)
				error(TOOMANYHINSI);
			hinsi[j++] = c;
		}
	}
	atr[i] = 0;
	if (j == 0)
		hinsi[j++] = HINSITANKAN;
	hinsi[j] = 0;

	return yomi;
}



void
setline(void (*func)(int *, int *, int, int *))
{
	int	i;

	for (i = 0 ; hinsi[i] ; i++)
		(*func)(yomi, kanji, hinsi[i], atr);
}
