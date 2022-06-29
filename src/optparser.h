#ifndef __CUTEST_OPTPARSER_H__
#define __CUTEST_OPTPARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cutest_optparse_argtype
{
    CUTEST_OPTPARSE_NONE,
    CUTEST_OPTPARSE_REQUIRED,
    CUTEST_OPTPARSE_OPTIONAL,
} cutest_optparse_argtype_t;

typedef struct cutest_optparse_long_opt
{
    const char*                 longname;
    int                         shortname;
    cutest_optparse_argtype_t   argtype;
} cutest_optparse_long_opt_t;

typedef struct cutest_optparse
{
    char**                      argv;
    int                         permute;
    int                         optind;
    int                         optopt;
    char*                       optarg;
    char                        errmsg[64];
    size_t                      subopt;
} cutest_optparse_t;

void cutest_optparse_init(cutest_optparse_t *options, char **argv);
int cutest_optparse_long(cutest_optparse_t *options,
    const cutest_optparse_long_opt_t *longopts, int *longindex);

#ifdef __cplusplus
}
#endif

#endif
