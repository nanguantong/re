/**
 * @file re_trace.h RE_TRACE helpers
 * JSON traces (chrome://tracing)
 */

typedef enum {
	RE_TRACE_ARG_NONE,
	RE_TRACE_ARG_INT,
	MTR_ARG_TYPE_STRING_CONST,
	MTR_ARG_TYPE_STRING_COPY,
} re_trace_arg_type;


int re_trace_init(const char *json_file);
int re_trace_close(void);
int re_trace_flush(void);
void re_trace_event(const char *cat, const char *name, char ph, void *id);

#if !defined(RELEASE) && !defined(RE_TRACE_ENABLED)
#define RE_TRACE_ENABLED 1
#endif

#ifdef RE_TRACE_ENABLED

#define RE_TRACE_BEGIN(c, n) re_trace_event(c, n, 'B', 0)
#define RE_TRACE_END(c, n) re_trace_event(c, n, 'E', 0)

#else

#define RE_TRACE_BEGIN(c, n)
#define RE_TRACE_END(c, n)

#endif