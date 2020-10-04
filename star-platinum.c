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

#include <err.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xutil.h>

#ifndef __OpenBSD__
# define pledge(a, b) 0
#endif

extern FILE *yyin;
char *fname;

extern int yylineno;
int yyparse(void);

struct group *config;
int goterror = 0;

Display *d;

int ignored_modifiers[] = {
	0,			/* no modifiers */
	LockMask,		/* caps lock */
	Mod2Mask,		/* num lock */
	Mod3Mask,		/* scroll lock */
	Mod5Mask,		/* ? */
};

#define IGNORE_MODIFIERS (LockMask | Mod2Mask | Mod3Mask | Mod5Mask)

__attribute__((__format__ (__printf__, 1, 2)))
void
yyerror(const char *fmt, ...)
{
	va_list ap;
	size_t l;

	va_start(ap, fmt);

	goterror = 1;
	fprintf(stderr, "%s:%d ", fname, yylineno);
	vfprintf(stderr, fmt, ap);

	l = strlen(fmt);
	if (l > 0 && fmt[l-1] != '\n')
		fprintf(stderr, "\n");

	va_end(ap);
}

char *
find_config()
{
	char *home, *xdg, *t;
	int free_xdg = 0;

	home = getenv("HOME");
	xdg = getenv("XDG_CONFIG_HOME");

	if (home == NULL && xdg == NULL)
		return NULL;

	/* file in home directory >>>>>> XDG shit */
	if (home != NULL) {
		if (asprintf(&t, "%s/.star-platinum.conf", home) == -1)
			return NULL;
		if (access(t, R_OK) == 0)
			return t;

		free(t);
		/* continue to search */
	}

	/* sanitize XDG_CONFIG_HOME */
	if (xdg == NULL) {
		free_xdg = 1;
		if (asprintf(&xdg, "%s/.config", home) == -1)
			return NULL;
	}

	if (asprintf(&t, "%s/star-platinum.conf", xdg) == -1)
		goto err;

	if (access(t, R_OK) == 0)
		return t;
	free(t);

err:
	if (free_xdg)
		free(xdg);

	return NULL;
}

int
main(int argc, char **argv)
{
	int status = 0, dump_config = 0, conftest = 0, ch;
	struct group *g;
	struct rule *r;
	XEvent e;
	Window root;

	signal(SIGCHLD, SIG_IGN); /* don't allow zombies */

	fname = NULL;
	while ((ch = getopt(argc, argv, "c:dn")) != -1) {
		switch (ch) {
		case 'c':
			free(fname);
			if ((fname = strdup(optarg)) == NULL)
				err(1, "strdup");
			break;

		case 'd':
			dump_config = 1;
			break;

		case 'n':
			conftest = 1;
			break;

		default:
			fprintf(stderr, "USAGE: %s [-dn] [-c conf]\n", *argv);
			return 1;
		}
	}

	if (fname == NULL) {
		if ((fname = find_config()) == NULL)
			errx(1, "can't find a configuration file");
	}

	if ((yyin = fopen(fname, "r")) == NULL)
		err(1, "cannot open %s", fname);
	yyparse();
	fclose(yyin);

	free(fname);
	fname = NULL;

	if (goterror)
		return 1;

	if (dump_config)
		printgroup(config);

	if (conftest) {
		recfree_group(config);
		return 0;
	}

	if ((d = XOpenDisplay(NULL)) == NULL) {
		recfree_group(config);
		return 1;
	}

	XSetErrorHandler(&error_handler);

	root = DefaultRootWindow(d);

	/* grab all the keys */
	for (g = config; g != NULL; g = g->next)
		for (r = g->rules; r != NULL; r = r->next)
			grabkey(r->key);

	XSelectInput(d, root, KeyPressMask);
	XFlush(d);

	pledge("stdio proc exec", NULL);

	while (1) {
		XNextEvent(d, &e);
		switch (e.type) {
		case KeyRelease:
		case KeyPress:
			process_event(config, (XKeyEvent*)&e);
			break;

		default:
			printf("Unknown event %d\n", e.type);
			break;
		}
	}

	XCloseDisplay(d);
	recfree_group(config);
	return status;
}


/* xlib */

int
error_handler(Display *d, XErrorEvent *e)
{
	fprintf(stderr, "Xlib error %d\n", e->type);
	return 1;
}

/* TODO: it should grab ALL POSSIBLE COMBINATIONS of `ignored_modifiers`! */
void
grabkey(struct key k)
{
	static size_t len = sizeof(ignored_modifiers)/sizeof(int);
	size_t i;
	Window root;

	root = DefaultRootWindow(d);

	/* printf("Grabbing "); printkey(k); printf("\n"); */
	for (i = 0; i < len; ++i) {
		XGrabKey(d, XKeysymToKeycode(d, k.key),
		    k.modifier | ignored_modifiers[i],
		    root, False, GrabModeAsync, GrabModeAsync);
	}
}

KeySym
keycode_to_keysym(unsigned int kc)
{
	/* group 0 (?). shift level is 0 because we don't want it*/
	return XkbKeycodeToKeysym(d, kc, 0, 0);
}

Window
focused_window()
{
	Window w;
	int revert_to;

	/* one can use (at least) three way to obtain the current
	 * focused window using xlib:
	 *
	 * - XQueryTree : you traverse tre tree until you find the
	 *     window
	 *
	 * - looking at _NET_ACTIVE_WINDOW in the root window, but
	 *    depedns on the window manager to set it
	 *
	 * - using XGetInputFocus
	 *
	 * I don't know the pro/cons of these, but XGetInputFocus
	 * seems the easiest.
	 */
	XGetInputFocus(d, &w, &revert_to);
	return w;
}

void
send_fake(Window w, struct key k, XKeyEvent *original)
{
	XKeyEvent e;

	/*
	 * this needs to be hijacked. original->window is the root
	 * window (since we grabbed the key there) and we want to
	 * deliver the key to another window.
	 */
	e.window = w;

	/* this is the fake key */
	e.keycode = XKeysymToKeycode(d, k.key);
	e.state = k.modifier;

	/* the rest is just copying fields from the original event */
	e.type = original->type;
	e.display = original->display;
	e.root = original->root;
	e.subwindow = original->subwindow;
	e.time = original->time;
	e.same_screen = original->same_screen;
	e.x = original->x;
	e.y = original->y;
	e.x_root = original->x_root;
	e.y_root = original->y_root;

	XSendEvent(d, w, True, KeyPressMask, (XEvent*)&e);
	XFlush(d);
}

int
window_match_class(Window w, const char *class)
{
	XClassHint ch;
	int matched;

	if (!XGetClassHint(d, w, &ch)) {
		fprintf(stderr, "XGetClassHint failed\n");
		return 0;
	}

	matched = !strcmp(ch.res_class, class);

	XFree(ch.res_name);
	XFree(ch.res_class);

	return matched;
}


/* action */

void
do_action(struct action a, Window focused, XKeyEvent *original)
{
	switch (a.type) {
	case AFAKE:
		send_fake(focused, a.send_key, original);
		break;

	case ASPECIAL:
		switch (a.special) {
		case ATOGGLE:
		case AACTIVATE:
		case ADEACTIVATE:
			printf("TODO\n");
			break;

		case AIGNORE:
			break;

		default:
			/* unreachable */
			abort();
		}
		break;

	case AEXEC: {
		pid_t p;
		const char *sh;

		/* exec only on key press */
		if (original->type == KeyRelease)
			break;

		switch (p = fork()) {
		case -1:
			err(1, "fork");

		case 0:
			if ((sh = getenv("SHELL")) == NULL)
				sh = "/bin/sh";
			printf("before exec'ing %s -c '%s'\n", sh, a.str);
			execlp(sh, sh, "-c", a.str, NULL);
			err(1, "execlp");
		}

		break;
	}

	default:
		/* unreachable */
		abort();
	}
}

void
free_action(struct action a)
{
	if (a.type == AEXEC)
		free(a.str);
}


/* match */

struct match *
new_match(int prop, char *s)
{
	struct match *m;

	if ((m = calloc(1, sizeof(*m))) == NULL)
		err(1, "calloc");
	m->prop = prop;
	m->str = s;
	return m;
}

void
recfree_match(struct match *m)
{
	struct match *mt;

	if (m == NULL)
		return;

	mt = m->next;
	free(m->str);
	free(m);
	recfree_match(mt);
}

int
match_window(struct match *m, Window w)
{
	switch (m->prop) {
	case MANY:
		return 1;

	case MCLASS:
		return window_match_class(w, m->str);
		break;

	default:
		/* unreachable */
		abort();
	}
}


/* rule */

struct rule *
new_rule(struct key k, struct action a)
{
	struct rule *r;

	if ((r = calloc(1, sizeof(*r))) == NULL)
		err(1, "calloc");
	memcpy(&r->key, &k, sizeof(k));
	memcpy(&r->action, &a, sizeof(a));
	return r;
}

void
recfree_rule(struct rule *r)
{
	struct rule *rt;

	if (r == NULL)
		return;

	rt = r->next;
	free(r);
	recfree_rule(rt);
}

int
rule_matched(struct rule *r, struct key k)
{
	unsigned int m;

	m = k.modifier;
	m &= ~IGNORE_MODIFIERS; /* clear ignored modifiers */

	return r->key.modifier == m
		&& r->key.key == k.key;
}


/* group */

struct group *
new_group(struct match *matches, struct rule *rules)
{
	struct group *g;

	if ((g = calloc(1, sizeof(*g))) == NULL)
		err(1, "calloc");
	g->matches = matches;
	g->rules = rules;
	return g;
}

void
recfree_group(struct group *g)
{
	struct group *gt;

	if (g == NULL)
		return;

	gt = g->next;
	recfree_match(g->matches);
	recfree_rule(g->rules);
	free(g);
	recfree_group(gt);
}

void
process_event(struct group *g, XKeyEvent *e)
{
	Window focused;
	struct rule *r;
	struct key pressed = {
		.modifier = e->state,
		.key = keycode_to_keysym(e->keycode),
	};

	focused = focused_window();

	for (; g != NULL; g = g->next) {
		if (!group_match(g, focused))
			continue;

		for (r = g->rules; r != NULL; r = r->next) {
			if (rule_matched(r, pressed)) {
				do_action(r->action, focused, e);
				return;
			}
		}
	}

	send_fake(focused, pressed, e);
}

int
group_match(struct group *g, Window w)
{
	struct match *m;

	for (m = g->matches; m != NULL; m = m->next)
		if (match_window(m, w))
			return 1;
	return 0;
}


/* debug/dump stuff */

void
printkey(struct key k)
{
	if (k.modifier & ControlMask)
		printf("C-");
	if (k.modifier & ShiftMask)
		printf("S-");
	if (k.modifier & Mod1Mask)
		printf("M-");
	if (k.modifier & Mod4Mask)
		printf("s-");

	printf("%s", XKeysymToString(k.key));
}

void
printaction(struct action a)
{
	switch (a.type) {
	case ASPECIAL:
		switch (a.special) {
		case ATOGGLE: printf("toggle"); break;
		case AACTIVATE: printf("activate"); break;
		case ADEACTIVATE: printf("deactivate"); break;
		case AIGNORE: printf("ignore"); break;
		default: abort(); /* unreachable */
		}
		break;

	case AFAKE:
		printf("send key ");
		printkey(a.send_key);
		break;

	case AEXEC:
		printf("exec %s", a.str);
		break;

	default:
		/* unreachable */
		abort();
	}
}

void
printmatch(struct match *m)
{
	if (m == NULL) {
		printf("(null)");
		return;
	}
	printf("match ");
	switch (m->prop) {
	case MANY:
		printf("all");
		break;
	case MCLASS:
		printf("class %s", m->str);
		break;
	default:
		/* unreachable */
		abort();
	}

	if (m->next == NULL)
		printf("\n");
	else {
		printf(" ; ");
		printmatch(m->next);
	}
}

void
printrule(struct rule *r)
{
	if (r == NULL) {
		printf("(null)");
		return;
	}
	printf("on ");
	printkey(r->key);
	printf(" do ");
	printaction(r->action);
	printf("\n");

	if (r->next != NULL)
		printrule(r->next);
}

void
printgroup(struct group *g)
{
	if (g == NULL) {
		printf("(null)");
		return;
	}
	printmatch(g->matches);
	printf("\n");
	printrule(g->rules);
	printf("\n\n");

	if (g->next != NULL)
		printgroup(g->next);
}
