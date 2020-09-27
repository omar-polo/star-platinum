# star platinum

`star-platinum` is a key binding manager for x11.  It acts primarily
as a translator: the canonical example is to Emacs-ify various
programs by customizing how some key are handled.

Check out the [manpage](star-platinum.1) for more information.

## Building

`star-platinum` depends on `xlib` and needs a C compiler, `yacc` and
`lex` to compile.  With that in place, it's as easy as

    make

Configuration for the build process can be found in `config.mk`, but
you usually don't need to modify it: passing the variables to make
should be enough.  For instance, to build with `gcc`

    make CC=gcc

## FAQ

 - *something something* bison *something something*

   `star-platinum` should build with GNU `bison` and flex, but I've
   still not tried.  `bison` has some defaults different from `yacc`
   IIRC, so additional flags may be needed.

 - the name is a jojo reference?

   Sort of.  I was listening to 「sono chi kioku」, the fourth opening,
   while I was playing with the idea of translating the key.  Given
   that I'm generally bad at naming things...
