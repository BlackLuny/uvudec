/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#include "uvd_arg_util.h"
#include "uvd_config.h"
#include "uvd_debug.h"
#include "uvd_log.h"
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef __GNUC__
#include <cxxabi.h>
#include <execinfo.h>
#endif

static const char *g_last_func = NULL;
static uint32_t g_debugTypeFlags = UVD_DEBUG_TYPE_NONE;
//<propertyForm, numeric flag>
static std::map<std::string, uint32_t> g_propertyFlagMap;

static void uvd_signal_handler(int sig)
{
	const char *sig_str = "UNKNOWN";
	switch( sig )
	{
	case SIGSEGV:
		sig_str = "SIGSEGV";
		break;
	case SIGFPE:
		sig_str = "SIGFPE";
		break;
	}
	
	printf("\n\nSEVERE ERROR\n");
	/*
	i before e, except after c... 
	...and you'll never be right, no matter what you say!
	*/
	printf("Received signal: %s\n", sig_str);
	//Yes, the stack is still intact
	UVD_PRINT_STACK();
	//exit() is not a "safe" function.  See man signal
	_exit(1);
}

static bool shouldPrintType(uint32_t type)
{
	//Any flags should be set?
	//Or is it more useful to do all?
	//usually we will have just one
	return type & g_debugTypeFlags;
}

uv_err_t UVDSetDebugFlag(uint32_t flag, uint32_t shouldSet)
{
	if( shouldSet )
	{
		g_debugTypeFlags |= flag;
	}
	else
	{
		g_debugTypeFlags &= ~flag;
	}
	return UV_ERR_OK;
}

static void printf_debug_prefix(FILE *logHandle, const char *prefix, const char *file, uint32_t line, const char *func)
{
	if( !logHandle )
	{
		logHandle = stdout;
	}
	if( prefix == NULL )
	{
		prefix = "";
	}
	fprintf(logHandle, "DEBUG %s(%s:%d): %s", file, func, line, prefix);
}

void printf_debug_core(uint32_t level, uint32_t type, const char *file, uint32_t line, const char *func, const char *format, ...)
{
	FILE *logHandle = g_log_handle;
	va_list ap;
	uint32_t verbose = 0;
	uint32_t set_level = 0;

	if( shouldPrintType(type) )
	{
		std::string typePrefix;
		
		//Keep logging before g_config initialized
		if( !logHandle )
		{
			logHandle = stdout;
		}
		if( g_config )
		{
			verbose = g_config->m_verbose;
			set_level = g_config->m_verbose_level;
		}
		else
		{
			verbose = 0;
			set_level = 0;
		}
	
		//Is logging disabled or are we at too high of a level?
		if( !verbose || level > set_level )
		{
			return;
		}
		
		if( g_config && g_config->m_modulePrefixes.find(type) != g_config->m_modulePrefixes.end() )
		{
			typePrefix = g_config->m_modulePrefixes[type];
			if( !typePrefix.empty() )
			{
				typePrefix += ": ";
			}
		}
		printf_debug_prefix(logHandle, typePrefix.c_str(), file, line, func);
	
		va_start(ap, format);
		vfprintf(logHandle, format, ap);
		fflush(logHandle);
		va_end(ap);
	}
}

void uv_enter(const char *file, uint32_t line, const char *func)
{
	printf_debug_level(UVD_DEBUG_VERBOSE, "ENTER: %s:%d::%s\n", file, line, func);
	g_last_func = func;
}

const char *get_last_func()
{
	return g_last_func;
}

static std::string argStringToDebugFlagProperty(const std::string &printPrefix)
{
	return std::string("debug.flag.") + printPrefix;
}

static std::string argStringToDebugFlagLongForm(const std::string &printPrefix)
{
	return std::string("debug-") + printPrefix;
}

static uv_err_t argParser(const UVDArgConfig *argConfig, std::vector<std::string> argumentArguments)
{
	UVDConfig *config = NULL;
	uint32_t typeFlag = 0;
	
	config = g_config;
	uv_assert_ret(config);
	uv_assert_ret(argConfig);

	//Map our property to a flag
	uv_assert_ret(g_propertyFlagMap.find(argConfig->m_propertyForm) != g_propertyFlagMap.end());
	typeFlag = g_propertyFlagMap[argConfig->m_propertyForm];
		
	if( argumentArguments.empty() )
	{
		uv_assert_err_ret(UVDSetDebugFlag(typeFlag, true));
	}
	else
	{
		std::string firstArg;

		firstArg = argumentArguments[0];
		uv_assert_err_ret(UVDSetDebugFlag(typeFlag, UVDArgToBool(firstArg)));
	}

	return UV_ERR_OK;
}

uv_err_t UVDRegisterTypePrefix(uvd_debug_flag_t typeFlag, const std::string &argName, const std::string &printPrefix)
{
	std::string propertyForm = argStringToDebugFlagProperty(argName);
	
	uv_assert_ret(g_config);
	uv_assert_ret(g_config->m_modulePrefixes.find(typeFlag) == g_config->m_modulePrefixes.end());
	g_config->m_modulePrefixes[typeFlag] = printPrefix;
	g_propertyFlagMap[propertyForm] = typeFlag;
	
	//static std::map<std::string, uint32_t> g_propertyFlagMap;
	
	uv_assert_err_ret(g_config->registerArgument(propertyForm,
			0, argStringToDebugFlagLongForm(argName),
			"set given debug flag",
			1,
			argParser,
			true));
	
	return UV_ERR_OK;
}

static uv_err_t initializeTypePrefixes()
{
	uv_assert_err_ret(UVDRegisterTypePrefix(UVD_DEBUG_TYPE_GENERAL, "general", ""));
	uv_assert_err_ret(UVDRegisterTypePrefix(UVD_DEBUG_TYPE_FLIRT, "flirt", "FLIRT"));
	
	return UV_ERR_OK;
}

uv_err_t UVDDebugInit()
{
	g_debugTypeFlags = UVD_DEBUG_TYPE_NONE;
	signal(SIGSEGV, uvd_signal_handler);
	signal(SIGFPE, uvd_signal_handler);
	uv_assert_err_ret(initializeTypePrefixes());
	return UV_ERR_OK;
}

uv_err_t UVDDebugDeinit()
{
	return UV_ERR_OK;
}

#ifdef __GNUC__
void uvd_print_trace(const char *file, int line, const char *function)
{
	/*
	Based on code from http://tombarta.wordpress.com/2008/08/01/c-stack-traces-with-gcc/
	*/
    const size_t max_depth = 100;
    size_t stack_depth;
    void *stack_addrs[max_depth];
    char **stack_strings;

    stack_depth = backtrace(stack_addrs, max_depth);
    stack_strings = backtrace_symbols(stack_addrs, stack_depth);

    printf("Call stack from %s:%d:\n", file, line);

    for (size_t i = 1; i < stack_depth; i++)
    {
        printf("    %s\n", stack_strings[i]);
    }
    free(stack_strings); // malloc()ed by backtrace_symbols
    fflush(stdout);
 #if 0
    const size_t max_depth = 100;
    size_t stack_depth;
    void *stack_addrs[max_depth];
    char **stack_strings;

    stack_depth = backtrace(stack_addrs, max_depth);
    stack_strings = backtrace_symbols(stack_addrs, stack_depth);

    printf("Call stack from %s @ %s:%d:\n", function, file, line);

	for (size_t i = 1; i < stack_depth; i++) {
		size_t sz = 200; // just a guess, template names will go much wider
		char *function = (char *)malloc(sz);
		char *begin = 0, *end = 0;
		// find the parentheses and address offset surrounding the mangled name
		for (char *j = stack_strings[i]; *j; ++j) {
		    if (*j == '(') {
		        begin = j;
		    }
		    else if (*j == '+') {
		        end = j;
		    }
		}
		if (begin && end) {
		    *begin++ = ' ';
		    *end = ' ';
		    // found our mangled name, now in [begin, end)

		    int status;
		    char *ret = abi::__cxa_demangle(begin, function, &sz, &status);
		    if (ret) {
		        // return value may be a realloc() of the input
		        function = ret;
		    }
		    else {
		        // demangling failed, just pretend it's a C function with no args
		        strncpy(function, begin, sz);
		        strncat(function, "()", sz);
		        function[sz-1] = ' ';
		    }
		    printf("    %s:%s\n", stack_strings[i], function);
		}
		else
		{
		    // didn't find the mangled name, just print the whole line
		    printf("    %s\n", stack_strings[i]);
		}
		free(function);
	}
    free(stack_strings); // malloc()ed by backtrace_symbols
    fflush(stdout);
#endif
}
#else
void uvd_print_trace(const char *file, int line, const char *function)
{
	printf_warn("stack trace unsupported\n");
}
#endif

