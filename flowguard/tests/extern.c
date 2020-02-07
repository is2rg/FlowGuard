typedef struct _IO_FILE FILE;

extern int fprintf(FILE * __restrict __stream,
                   char const * __restrict __format, ...);

extern FILE *stderr;

static void sigcleanup(int signo)
{
    fprintf(stderr, "signal %d\n", signo);
    return;
}
