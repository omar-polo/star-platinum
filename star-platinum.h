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

#ifndef STAR_PLATINUM_H
#define STAR_PLATINUM_H

/* XXX: why I need to define this to get XK_space/...?
 * is this a bad idea?  there is another way? */
#define XK_LATIN1
#define XK_MISCELLANY

#include <X11/Xlib.h>

#include <err.h>
#include <stdlib.h>

void			 yyerror(const char*, ...);

struct key {
	unsigned int modifier;
	KeySym key;
};

struct action {
#define ASPECIAL	1
#define AFAKE		2
#define AEXEC		3
	int type;
	union {
#define ATOGGLE		1
#define AACTIVATE	2
#define ADEACTIVATE	3
#define AIGNORE		4
		int special;
		struct key send_key;
		char *str;
	};
};

void			 do_action(struct action, Window, XKeyEvent*);
void			 free_action(struct action);

struct match {
#define MANY		1
#define MCLASS		2
	int prop;
	char *str;
	struct match *next;
};

struct match		*new_match(int, char*);
void			 recfree_match(struct match*);
int			 match_window(struct match*, Window);

struct rule {
	struct key key;
	struct action action;
	struct rule *next;
};

struct rule		*new_rule(struct key, struct action);
void			 recfree_rule(struct rule*);
int			 rule_matched(struct rule*, struct key);

struct group {
	struct match *matches;
	struct rule *rules;
	struct group *next;
};

extern struct group *config;

struct group		*new_group(struct match*, struct rule*);
void			 recfree_group(struct group*);
void			 process_event(struct group*, XKeyEvent*);
int			 group_match(struct group*, Window);

/* xlib-related */
int			 error_handler(Display*, XErrorEvent*);
void			 grabkey(struct key, Window);
KeySym			 keycode_to_keysym(unsigned int);
Window			 focused_window();
void			 send_fake(Window, struct key, XKeyEvent*);
int			 window_match_class(Window, const char*);

/* debugging */
void			 printkey(struct key);
void			 printaction(struct action);
void			 printmatch(struct match*);
void			 printrule(struct rule*);
void			 printgroup(struct group*);

#endif /* STAR_PLATINUM_H */
