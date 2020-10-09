/**
 * @file re_trace.h RE_TRACE helpers
 * JSON traces (chrome://tracing)
 */

typedef enum {
	RE_TRACE_ARG_NONE,
	RE_TRACE_ARG_INT,
	RE_TRACE_ARG_STRING_CONST,
	RE_TRACE_ARG_STRING_COPY,
} re_trace_arg_type;


int re_trace_init(const char *json_file);
int re_trace_close(void);
int re_trace_flush(void);
void re_trace_event(const char *cat, const char *name, char ph, void *id,
		   re_trace_arg_type arg_type, const char *arg_name,
		   void *arg_value);

#if !defined(RELEASE) && !defined(RE_TRACE_ENABLED)
#define RE_TRACE_ENABLED 1
#endif

#ifdef RE_TRACE_ENABLED

#define RE_TRACE_BEGIN(c, n) \
	re_trace_event(c, n, 'B', 0, RE_TRACE_ARG_NONE, NULL, NULL)
#define RE_TRACE_END(c, n) \
	re_trace_event(c, n, 'E', 0, RE_TRACE_ARG_NONE, NULL, NULL)
#define RE_TRACE_INSTANT(c, n) \
	re_trace_event(c, n, 'I', 0, RE_TRACE_ARG_NONE, NULL, NULL)
#define RE_TRACE_COUNTER(c, n, val) \
	re_trace_event(c, n, 'C', 0, RE_TRACE_ARG_INT, \
	n, (void *)(intptr_t)val)

#else

#define RE_TRACE_BEGIN(c, n)
#define RE_TRACE_END(c, n)
#define RE_TRACE_INSTANT(c, n)
#define RE_TRACE_COUNTER(c, n, val)

#endif