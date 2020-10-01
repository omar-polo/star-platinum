# star platinum

`star-platinum` is a key binding manager for x11.  It acts primarily
as a translator: the canonical example is to Emacs-ify various
programs by customizing how some key are handled.

Check out the [manpage](star-platinum.1) for more information.

## Building

`star-platinum` depends on `xlib` and needs a C compiler, `yacc` and
`lex` to compile.  With that in place, it's as easy as

    make

OpenBSD' `lex` and `yacc` were tested, as well as `bison` 3.3.2, GNU
gcc 4.2.1 and 8.4.0, and clang 10.0.1

Configuration for the build process can be found in `config.mk`, but
you usually don't need to modify it: passing the variables to make
should be enough.  For instance, to build with `gcc`

    make CC=gcc

`bison` can be used instead of `yacc` by changing the `YACC` variable

    make YACC=bison

Unless you are compiling on OpenBSD, you probably want to change the
default `CFLAGS` and `LDFLAGS`.

If `etags` is available, a `TAGS` file is created.  Note however that
`etags` is **not needed** for building: it's only a support tool used
to aid the development.

## FAQ

 - the name is a jojo reference?

   Sort of.  I was listening to 「sono chi kioku」, the fourth opening,
   while I was playing with the idea of translating the key.  Given
   that I'm generally bad at naming things...
