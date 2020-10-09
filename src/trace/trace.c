/**
 * @file trace.c RE_TRACE helpers
 * JSON traces (chrome://tracing)
 */
#include <stdio.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_trace.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_tmr.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#define TRACE_BUFFER_SIZE 1000000

struct trace_event {
	const char *name;
	const char *cat;
	void *id;
	uint64_t ts;
	int pid;
	uint32_t tid;
	char ph;
	re_trace_arg_type arg_type;
	const char *arg_name;
	union {
		const char *a_str;
		int a_int;
	} arg;
};

/** Trace configuration */
static struct {
	int process_id;
	FILE *f;               /**< Logfile                */
	int event_count;
	struct trace_event *event_buffer;
	struct trace_event *event_buffer_flush;
	bool tracing;
	bool flushing;
	pthread_mutex_t mutex; /**< Thread locking         */
} trace = {
	0,
	NULL,
	0,
	NULL,
	NULL,
	false,
	false,
};


static inline uint32_t get_thread_id(void)
{
#if defined(WIN32)
	return (uint32_t)GetCurrentThreadId();
#else
	return (unsigned long)pthread_self();
#endif
}


static inline int get_process_id(void)
{
#if defined(WIN32)
	return (int)GetCurrentProcessId();
#else
	return (int)getpid();
#endif
}


int re_trace_init(const char *json_file)
{
#ifndef RE_TRACE_ENABLED
	return 0;
#endif
	if (!json_file)
		return EINVAL;

	trace.event_buffer = mem_zalloc(
		TRACE_BUFFER_SIZE * sizeof(struct trace_event), NULL);
	if (!trace.event_buffer)
		return ENOMEM;

	trace.event_buffer_flush = mem_zalloc(
		TRACE_BUFFER_SIZE * sizeof(struct trace_event), NULL);
	if (!trace.event_buffer_flush) {
		trace.event_buffer = mem_deref(trace.event_buffer);
		return ENOMEM;
	}

	pthread_mutex_init(&trace.mutex, 0);

	trace.f = fopen(json_file, "w+");
	if (!trace.f)
		return errno;

	(void)re_fprintf(trace.f, "{\t\n\t\"traceEvents\": [\n");
	(void)fflush(trace.f);

	return 0;
}


int re_trace_close(void)
{
#ifndef RE_TRACE_ENABLED
	return 0;
#endif
	int err = 0;

	re_trace_flush();

	trace.event_buffer = mem_deref(trace.event_buffer);
	trace.event_buffer_flush = mem_deref(trace.event_buffer_flush);
	pthread_mutex_destroy(&trace.mutex);

	(void)re_fprintf(trace.f, "\n\t]\n}\n");
	if (trace.f)
		err = fclose(trace.f);

	if (err)
		return errno;

	trace.f = NULL;

	return 0;
}


int re_trace_flush(void)
{
#ifndef RE_TRACE_ENABLED
	return 0;
#endif
	int i, first_line;
	struct trace_event *event_tmp;
	struct trace_event *e;
	char json_arg[1024];

	pthread_mutex_lock(&trace.mutex);
	event_tmp = trace.event_buffer_flush;
	trace.event_buffer_flush = trace.event_buffer;
	trace.event_buffer = event_tmp;
	pthread_mutex_unlock(&trace.mutex);

	first_line = 1;
	for (i = 0; i < trace.event_count; i++)
	{
		e = &trace.event_buffer_flush[i];

		switch (e->arg_type)
		{
		case RE_TRACE_ARG_NONE:
			json_arg[0] = '\0';
			break;
		case RE_TRACE_ARG_INT:
			(void)re_snprintf(json_arg, sizeof(json_arg),
					", \"args\":{\"%s\":%i}",
					e->arg_name, e->arg.a_int);
			break;
		case RE_TRACE_ARG_STRING_CONST:
			(void)re_snprintf(json_arg, sizeof(json_arg),
					", \"args\":{\"%s\":\"%s\"}",
					e->arg_name, e->arg.a_str);
			break;
		case RE_TRACE_ARG_STRING_COPY:
			if (str_len(e->arg.a_str) > 300)
			{
				(void)re_snprintf(json_arg, sizeof(json_arg),
					", \"args\":{\"%s\":\"%.*s\"}",
					e->arg_name, 300, e->arg.a_str);
			}
			else
			{
				(void)re_snprintf(json_arg, sizeof(json_arg),
					", \"args\":{\"%s\":\"%s\"}",
					e->arg_name, e->arg.a_str);
			}
			break;
		}
		(void)re_fprintf(trace.f,
			"%s{\"cat\":\"%s\",\"pid\":%i,\"tid\":%lu,\"ts\":%llu,"
			"\"ph\":\"%c\",\"name\":\"%s\"%s}",
			first_line ? "" : ",\n",
			e->cat, e->pid, e->tid, e->ts, e->ph, e->name,
			str_len(json_arg) ? json_arg : "");
		first_line = 0;
	}

	(void)fflush(trace.f);
	return 0;
}


void re_trace_event(const char *cat, const char *name, char ph, void *id,
		   re_trace_arg_type arg_type, const char *arg_name,
		   void *arg_value)
{
#ifndef RE_TRACE_ENABLED
	return;
#endif
	struct trace_event *e;

	pthread_mutex_lock(&trace.mutex);
	if (!trace.f) {
		pthread_mutex_unlock(&trace.mutex);
		return;
	}
	e = &trace.event_buffer[trace.event_count];
	++trace.event_count;
	pthread_mutex_unlock(&trace.mutex);

	e->ts = tmr_jiffies_us();
	e->id = id;
	e->ph = ph;
	e->cat = cat;
	e->name = name;
	e->pid = get_process_id();
	e->tid = get_thread_id();
	e->arg_type = arg_type;
	e->arg_name = arg_name;

	switch (arg_type)
	{
	case RE_TRACE_ARG_NONE:
		break;
	case RE_TRACE_ARG_INT:
		e->arg.a_int = (int)(uintptr_t)arg_value;
		break;
	case RE_TRACE_ARG_STRING_CONST:
		e->arg.a_str = (const char *)arg_value;
		break;
	case RE_TRACE_ARG_STRING_COPY:
		str_dup((char **)&e->arg.a_str, (const char *)arg_value);
		break;
	}
}