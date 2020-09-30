/* -*- mode:fundamental; indent-tabs-mode: t; -*- */
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

#include "star-platinum.h"

#include <X11/Xlib.h>
#include <X11/keysymdef.h>

#include <err.h>
#include <string.h>

#include "y.tab.h"

int state  = 0;
KeySym key = 0;

%}

%x COMMENT
%x KEY

%%

"#"		{ BEGIN(COMMENT); }
<COMMENT>\n	{ yylineno++; BEGIN(INITIAL); }
<COMMENT>.	;

[ \t\r\v\f]+	;

"match"		return TMATCH;
"class"		return TCLASS;
"on"		return TON;
"do"		return TDO;
"toggle"	return TTOGGLE;
"activate"	return TACTIVATE;
"deactivate"	return TDEACTIVATE;
"ignore"	return TIGNORE;

"\n"		yylineno++; return '\n';
"*"		return '*';

[-a-zA-Z0-9]+	{
			char *ident;
			if ((ident = strdup(yytext)) == NULL)
				err(1, "strdup");
			yylval.str = ident;
			return TSTRING;
		}

"\""		{ BEGIN(KEY); }
<KEY>"\""	{
			BEGIN(INITIAL);

			yylval.key = (struct key){ state, key };
			state = 0;
			key = 0;
			return TKEY;
		}

<KEY>C-		{ state |= ControlMask; }
<KEY>S-		{ state |= ShiftMask; }
<KEY>M-		{ state |= Mod1Mask; }
<KEY>s-		{ state |= Mod4Mask; }

<KEY><[^>]+>	{
			char *c;

			if ((c = strdup(yytext)) == NULL)
                                err(1, "strdup");

			c++;			/* skip the < */
			c[strlen(c)-1] = '\0';	/* trim the > */

			if ((key = XStringToKeysym(c)) == NoSymbol)
				yyerror("invalid keysym %s", c);

			free(--c);
		}

<KEY>SPC	{ key = XK_space; }
<KEY>RET	{ key = XK_Return; }

<KEY>"("	{ key = XK_parenleft; }
<KEY>")"	{ key = XK_parenright; }

<KEY>.		{
			if ((key = XStringToKeysym(yytext)) == NoSymbol)
				yyerror("invalid key %s", yytext);
		}


%%

int
yywrap(void)
{
	return 1;
}
