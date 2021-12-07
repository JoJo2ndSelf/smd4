#ifndef INCLUDED_GETOPT_PORT_H
#define INCLUDED_GETOPT_PORT_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef SMD_API
#define SMD_API
#endif

#define gop_no_argument (0)
#define gop_required_argument (1)
#define gop_optional_argument (2)

struct gop_option {
  char const* name, *usage;
  int has_arg;
  int* flag;
  int val;
  int short_name;
};

struct gop_ctx {
  struct gop_option const* longopts;
  char** argv;
  char const* optarg, *optcursor;
  int argc, optind, opterr, optopt, longindex;
};

SMD_API void gop_init(struct gop_ctx* ctx, int argc, char** argv, struct gop_option const* longopts);
SMD_API int gop_next(struct gop_ctx* ctx);

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_GETOPT_PORT_H
