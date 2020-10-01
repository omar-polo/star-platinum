/* -*- mode: fundamental; indent-tabs-mode: t; -*- */
%{

/*
 * Copyright (c) 2020 Omar Polo <op@omarpolo.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>

#include "star-platinum.h"

/*
 * #define YYDEBUG 1
 * int yydebug = 1;
 */

#define SPECIAL(X) ((struct action){.type = (ASPECIAL), .special = (X) })
#define FAKE_KEY(K) ((struct action){.type = (AFAKE), .send_key = (K) })
#define EXEC(S) ((struct action){.type = (AEXEC), .str = (S) })

%}
 
%union {
	struct key key;
	char *str;
	struct action action;
	struct match *match;
	struct rule *rule;
	struct group *group;
}

%token TMATCH TCLASS TALL
%token TON TDO
%token TTOGGLE TACTIVATE TDEACTIVATE TIGNORE TEXEC
%token TERR

%token <key>	TKEY
%token <str>	TSTRING

%type <action>	action
%type <match>	matches match
%type <rule>	keys key
%type <group>	groups group

%%

groups		: /* empty */		{ $$ = NULL; }
		| groups group		{ $2->next = $1; config = $$ = $2; }
		| error '\n'
		;

group		: matches keys		{ $$ = new_group($1, $2); }
		;

matches		: /* empty */		{ $$ = NULL; }
		| matches '\n'		{ $$ = $1; }
		| matches match '\n'	{ $2->next = $1; $$ = $2; }
		;

match		: TMATCH TALL		{ $$ = new_match(MANY, NULL); }
		| TMATCH TCLASS TSTRING { $$ = new_match(MCLASS, $3); }
		;

keys		: /* empty */		{ $$ = NULL; }
		| keys '\n'		{ $$ = $1; }
		| keys key '\n'		{ $2->next = $1; $$ = $2; }
		;

key		: TON TKEY TDO action	{ $$ = new_rule($2, $4); }
		;

action		: TKEY			{ $$ = FAKE_KEY($1); }
		| TTOGGLE		{ $$ = SPECIAL(ATOGGLE); }
		| TACTIVATE		{ $$ = SPECIAL(AACTIVATE); }
		| TDEACTIVATE		{ $$ = SPECIAL(ADEACTIVATE); }
		| TIGNORE		{ $$ = SPECIAL(AIGNORE); }
		| TEXEC TSTRING		{ $$ = EXEC($2); }
		;
