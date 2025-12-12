#ifndef TERMCOLOR_H
#define TERMCOLOR_H

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_END      "\x1b[0m"

#define TR_TAG    ANSI_COLOR_CYAN"[Traceroute]"ANSI_COLOR_END
#define DNS_TAG   ANSI_COLOR_BLUE"[DNS Lookup]"ANSI_COLOR_END
#define PROXY_TAG "[Proxy]"

#define ERR_TAG   ANSI_COLOR_RED"[Error]"ANSI_COLOR_END
#define WARN_TAG  ANSI_COLOR_YELLOW"[Warning]"ANSI_COLOR_END

#endif
