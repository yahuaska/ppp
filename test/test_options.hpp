//
// Created by ringo on 17.09.18.
//

#ifndef PPP_TEST_OPTIONS_HPP
#define PPP_TEST_OPTIONS_HPP

#include "options.hpp"

int vslprintf(char *buf, int buflen, char *fmt, va_list args)
{
	return 0;
}

/*
 * fatal - log an error message and die horribly.
 */
void
fatal __V((char * fmt, ...))
{
}

size_t strlcpy(char*, char const*, unsigned long) {return 0;}

void
option_error(char * fmt, ...)
{
	va_list args;
	char buf[1024];

#if defined(__STDC__)
	va_start(args, fmt);
#else
	char *fmt;
    va_start(args);
    fmt = va_arg(args, char *);
#endif
	vslprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	fprintf(stderr, "%s: %s\n", progname, buf);
}

void warn(char*, ...){}

/*
 * Read a word from a file.
 * Words are delimited by white-space or by quotes (" or ').
 * Quotes, white-space and \ may be escaped with \.
 * \<newline> is ignored.
 */
int
getword(FILE *, char *, int *, char *)
{
	return 0;
}

void novm(char*) {}

#endif //PPP_TEST_OPTIONS_HPP
