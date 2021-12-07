#ifndef SMD_UNITY_BUILD

#include "getopt.h"

#include <stddef.h>
#include <string.h>


//char* optarg;
//int optopt;
/* The variable optind [...] shall be initialized to 1 by the system. */
//int optind = 1;
//int opterr;

//static char* optcursor = NULL;

static struct gop_option const* opt_search(struct gop_ctx* ctx, int optchar) {
  int i = 0;
  for (struct gop_option const* o=ctx->longopts; o->name; ++o, ++i) {
    if (o->short_name == optchar) {
      return o;
    }
  }
  return NULL;
}

/* Implemented based on [1] and [2] for optional arguments.
   optopt is handled FreeBSD-style, per [3].
   Other GNU and FreeBSD extensions are purely accidental. 
   
[1] http://pubs.opengroup.org/onlinepubs/000095399/functions/getopt.html
[2] http://www.kernel.org/doc/man-pages/online/pages/man3/getopt.3.html
[3] http://www.freebsd.org/cgi/man.cgi?query=getopt&sektion=3&manpath=FreeBSD+9.0-RELEASE
*/
static int single(struct gop_ctx* ctx) {
  int optchar = -1;
  struct gop_option const* optdecl = NULL;
  int argc = ctx->argc;
  char** argv = ctx->argv;

  ctx->optarg = NULL;
  ctx->opterr = 0;
  ctx->optopt = 0;

  /* Unspecified, but we need it to avoid overrunning the argv bounds. */
  if (ctx->optind >= argc)
    goto no_more_optchars;

  /* If, when getopt() is called argv[optind] is a null pointer, getopt() 
     shall return -1 without changing optind. */
  if (argv[ctx->optind] == NULL)
    goto no_more_optchars;

  /* If, when getopt() is called *argv[optind]  is not the character '-', 
     getopt() shall return -1 without changing optind. */
  if (*argv[ctx->optind] != '-')
    goto no_more_optchars;

  /* If, when getopt() is called argv[optind] points to the string "-", 
     getopt() shall return -1 without changing optind. */
  if (strcmp(argv[ctx->optind], "-") == 0)
    goto no_more_optchars;

  /* If, when getopt() is called argv[optind] points to the string "--", 
     getopt() shall return -1 after incrementing optind. */
  if (strcmp(argv[ctx->optind], "--") == 0) {
    ++ctx->optind;
    goto no_more_optchars;
  }

  if (ctx->optcursor == NULL || *ctx->optcursor == '\0')
    ctx->optcursor = argv[ctx->optind] + 1;

  optchar = *ctx->optcursor;

  /* FreeBSD: The variable optopt saves the last known option character 
     returned by getopt(). */
  ctx->optopt = optchar;

  /* The getopt() function shall return the next option character (if one is 
     found) from argv that matches a character in optstring, if there is 
     one that matches. */
  //optdecl = strchr(optstring, optchar);
  optdecl = opt_search(ctx, optchar);
  if (optdecl) {
    /* [I]f a character is followed by a colon, the option takes an
       argument. */
    if (optdecl->has_arg != gop_no_argument) {
      ctx->optarg = ++ctx->optcursor;
      if (*ctx->optarg == '\0') {
        /* GNU extension: Two colons mean an option takes an
           optional arg; if there is text in the current argv-element 
           (i.e., in the same word as the option name itself, for example, 
           "-oarg"), then it is returned in optarg, otherwise optarg is set
           to zero. */
        if (optdecl->has_arg != gop_optional_argument) {
          /* If the option was the last character in the string pointed to by
             an element of argv, then optarg shall contain the next element
             of argv, and optind shall be incremented by 2. If the resulting
             value of optind is greater than argc, this indicates a missing 
             option-argument, and getopt() shall return an error indication.

             Otherwise, optarg shall point to the string following the
             option character in that element of argv, and optind shall be
             incremented by 1.
          */
          if (++ctx->optind < argc) {
            ctx->optarg = argv[ctx->optind];
          } else {
            /* If it detects a missing option-argument, it shall return the 
               colon character ( ':' ) if the first character of optstring
               was a colon, or a question-mark character ( '?' ) otherwise.
            */
            ctx->optarg = NULL;
            optchar = (optdecl->has_arg != gop_no_argument) ? ':' : '?';
          }
        } else {
          ctx->optarg = NULL;
        }
      }

      ctx->optcursor = NULL;
    }
  } else {
    /* If getopt() encounters an option character that is not contained in 
       optstring, it shall return the question-mark ( '?' ) character. */
    optchar = '?';
  }

  if (ctx->optcursor == NULL || *++ctx->optcursor == '\0')
    ++ctx->optind;

  return optchar;

no_more_optchars:
  ctx->optcursor = NULL;
  return -1;
}

void gop_init(struct gop_ctx* ctx, int argc, char** argv, const struct gop_option* longopts) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->optind = 1;
  ctx->longopts = longopts;
  ctx->argc = argc;
  ctx->argv = argv;
}

/* Implementation based on [1].

[1] http://www.kernel.org/doc/man-pages/online/pages/man3/getopt.3.html
*/
int gop_next(/*int argc, char* const argv[], 
  struct gop_option const* longopts, int* longindex*/
  struct gop_ctx* ctx) {
  struct gop_option const* o = ctx->longopts;
  struct gop_option const* match = NULL;
  int num_matches = 0;
  size_t argument_name_length = 0;
  char const* current_argument = NULL;
  int retval = -1;
  int argc = ctx->argc;
  char** argv = ctx->argv;

  ctx->optarg = NULL;
  ctx->optopt = 0;

  if (ctx->optind >= argc)
    return -1;

  if (strlen(argv[ctx->optind]) < 3 || strncmp(argv[ctx->optind], "--", 2) != 0)
    return single(ctx);

  /* It's an option; starts with -- and is longer than two chars. */
  current_argument = argv[ctx->optind] + 2;
  argument_name_length = strcspn(current_argument, "=");
  for (; o->name; ++o) {
    if (strncmp(o->name, current_argument, argument_name_length) == 0) {
      match = o;
      ++num_matches;
    }
  }

  if (num_matches == 1) {
    /* set to the index of the long option relative to longopts. */
    ctx->longindex = (int)(match - ctx->longopts);

    /* If flag is not NULL set the external flag to value */
    if (match->flag)
      *(match->flag) = match->val;

    retval = match->short_name;

    if (match->has_arg != gop_no_argument) {
      ctx->optarg = strchr(argv[ctx->optind], '=');
      if (ctx->optarg != NULL)
        ++ctx->optarg;

      if (match->has_arg == gop_required_argument) {
        /* Only scan the next argv for required arguments. Behavior is not
           specified, but has been observed with Ubuntu and Mac OSX. */
        if (ctx->optarg == NULL && ++ctx->optind < argc) {
          ctx->optarg = argv[ctx->optind];
        }

        if (ctx->optarg == NULL)
          retval = ':';
      }
    } else if (strchr(argv[ctx->optind], '=')) {
      /* An argument was provided to a non-argument option. 
         I haven't seen this specified explicitly, but both GNU and BSD-based
         implementations show this behavior.
      */
      retval = '?';
    }
  } else {
    /* Unknown option or ambiguous match. */
    retval = '?';
  }

  ++ctx->optind;
  return retval;
}

#endif
