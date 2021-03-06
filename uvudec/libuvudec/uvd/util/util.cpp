/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include "uvd/util/util.h"
#include "uvd/util/debug.h"
#include "uvd/util/error.h"
#include "uvd/core/uvd.h"
#include <linux/limits.h>

/* Get the name and args as a string */
uv_err_t uvd_parse_func(const char *text, char **name, char **content);

uv_err_t parseFunc(const std::string &text, std::string &name, std::string &content)
{
	char *nameTemp = NULL;
	char *contentTemp = NULL;

	if( UV_FAILED(uvd_parse_func(text.c_str(), &nameTemp, &contentTemp)) )
	{
		return UV_ERR_GENERAL;
	}

	uv_assert_ret(nameTemp);
	name = nameTemp;
	free(nameTemp);
	nameTemp = NULL;

	uv_assert_ret(contentTemp);
	content = contentTemp;
	free(contentTemp);
	contentTemp = NULL;
	
	return UV_ERR_OK;
}


uv_err_t getTempFile(std::string &sFile)
{
	char szBuffer[PATH_MAX] = UV_TEMP_PREFIX;
	int fd = 0;
	
	fd = mkstemp(szBuffer);
	if( fd < 0 )
	{
		return UV_DEBUG(UV_ERR_GENERAL);
	}
	close(fd);
	sFile = szBuffer;
	return UV_ERR_OK;
}

std::string escapeArg(const std::string &sIn)
{
	std::string sRet;
	for( std::string::size_type i = 0; i < sIn.size(); ++i )
	{
		if( !((sIn[i] >= 'A' && sIn[i] <= 'Z')
				|| (sIn[i] >= 'a' && sIn[i] <= 'z')
				|| (sIn[i] >= '0' && sIn[i] <= '9')
				|| sIn[i] == '/'
				|| sIn[i] == '.'
				|| sIn[i] == '_'
				|| sIn[i] == '-'
				) )
		{
		  	sRet += '\\';
		}
		sRet += sIn[i];
	}
	return sRet;
}

uv_err_t UVDExecuteToFile(const std::string &sCommand,
		const std::vector<std::string> &args,
		int &rcProcess,
		const std::string *stdOutIn,
		const std::string *stdErrIn)
{
	uv_err_t rc = UV_ERR_GENERAL;
	std::string stdOut;
	std::string stdErr;
	std::string sExec;
	
	UV_ENTER();

	if( stdOutIn )
	{
		stdOut = *stdOutIn;
	}
	if( stdErrIn )
	{
		stdErr = *stdErrIn;
	}
	
	sExec = escapeArg(sCommand);
	for( std::vector<std::string>::size_type i = 0; i < args.size(); ++i )
	{
		sExec += " ";
		sExec += escapeArg(args[i]);
	}
	if( stdOut.empty() )
	{
		stdOut = "/dev/null";
	}
	sExec += " 1>";
	sExec += stdOut;	
	if( stdErr.empty() )
	{
		stdErr = "/dev/null";
	}
	sExec += " 2>";
	sExec += stdErr;	
	
	printf_debug("Executing: %s\n", sExec.c_str());
	rcProcess = system(sExec.c_str());
	printf_debug("Ret: %d\n", rcProcess);
	uv_assert(!(rcProcess & 0xFF));
	rcProcess /= 256;
	rc = UV_ERR_OK;

error:
	return UV_DEBUG(rc);
}

uv_err_t UVDExecuteToText(const std::string &sCommand,
		const std::vector<std::string> &args,
		int &rcProcess,
		std::string *stdOut,
		std::string *stdErr)
{
	uv_err_t rc = UV_ERR_GENERAL;
	static std::string stdOutFile;
	static std::string stdErrFile;
	std::string stdOutFileLocal;
	std::string stdErrFileLocal;
	
	UV_ENTER();
	
	if( stdOutFile.empty() )
	{
		uv_assert_err(getTempFile(stdOutFile));
	}
	if( stdErrFile.empty() )
	{
		uv_assert_err(getTempFile(stdErrFile));
	}
	
	if( stdOut )
	{
		stdOutFileLocal = stdOutFile;		
	}
	if( stdErr )
	{
		if( stdOut == stdErr )
		{
			stdErrFileLocal = stdOutFile;
			stdErr = NULL;
		}
		else
		{
			stdErrFileLocal = stdErrFile;
		}
	}
	
	uv_assert_err(UVDExecuteToFile(sCommand,
			args,
			rcProcess,
			&stdOutFileLocal,
			&stdErrFileLocal));
	
	if( stdOut && UV_FAILED(readFile(stdOutFile, *stdOut)) )
	{
		goto error;
	}
	if( stdErr && UV_FAILED(readFile(stdErrFile, *stdErr)) )
	{
		goto error;
	}
	
	rc = UV_ERR_OK;
	
error:
	
	if( stdOut )
	{
		if( UV_FAILED(deleteFile(stdOutFileLocal)) && UV_SUCCEEDED(rc) )
		{
			rc = UV_ERR_GENERAL;
		}
	}
	if( stdErr )
	{
		if( UV_FAILED(deleteFile(stdErrFileLocal)) && UV_SUCCEEDED(rc) )
		{
			rc = UV_ERR_GENERAL;
		}
	}
	
	return UV_DEBUG(rc);
}

uv_err_t uvdPreprocessLine(const std::string &lineIn, std::string &lineOut)
{
	/*
	Skip comments, empty 
	Must be at beginning, some assemblers require # chars
	*/
	if( lineIn.empty() )
	{
		lineOut = "";
		return UV_ERR_BLANK;
	}
	if( lineIn.c_str()[0] == '#' )
	{
		lineOut = "";
		return UV_ERR_BLANK;
	}
	
	lineOut = lineIn;
	
	return UV_ERR_OK;
}

uv_err_t uvdParseLine(const std::string &line, std::string &key, std::string &value)
{
	uv_err_t rc = UV_ERR_GENERAL;
	std::string::size_type equalsPos = 0;

	/*
	Skip comments, empty 
	Must be at beginning, some assemblers require # chars
	*/
	if( line.empty() )
	{
		rc = UV_ERR_BLANK;
		goto error;	
	}
	if( line.c_str()[0] == '#' )
	{
		rc = UV_ERR_BLANK;
		goto error;	
	}

	/* Double check we aren't a section */
	uv_assert(line[0] != '.');

	/* Setup key/value pairing */
	equalsPos = line.find("=");
	if( equalsPos == std::string::npos )
	{
		UV_ERR(rc);
		goto error;
	}
	//Should have at least one char for key
	if( equalsPos < 1 )
	{
		UV_ERR(rc);
		goto error;
	}
	key = line.substr(0, equalsPos);
	//Skip the equals sign, make key comparisons easier
	//Value can be empty
	++equalsPos;
	if( equalsPos >= line.size() )
	{
		value = "";
	}
	else
	{
		value = line.substr(equalsPos);
	}

	printf_debug("key: %s, value: %s\n", key.c_str(), value.c_str());
	rc = UV_ERR_OK;
	
error:
	return UV_DEBUG(rc);
}

uv_err_t uvd_parse_func(const char *text, char **name, char **content)
{
	uv_err_t rc = UV_ERR_GENERAL;
	char *loc = NULL;
	char *copy = NULL;
	char *end = NULL;
	
	uv_assert(text);
	uv_assert(name);
	uv_assert(content);

	//printf_debug("uvd_parse_func, in: <%s>\n", text);

	copy = strdup(text);
	uv_assert_all(copy);

	loc = strstr(copy, "(");
	if( !loc )
	{
		free(copy);
		return UV_ERR_GENERAL;
	}
	loc[0] = 0;
	++loc;
	
	//Find last )
	end = loc;
	for( ;; )
	{
		char *temp = NULL;
		
		temp = strstr(end, ")");
		if( temp == NULL )
		{
			//Did we find at least one?
			if( end == loc )
			{
				return UV_ERR_GENERAL;
			}
			break;
		}
		end = temp + 1;
	}
	end[0] = 0;
	
	*name = strdup(copy);
	uv_assert_all(*name);

	*content = strdup(loc);
	uv_assert_all(*content);
		
	free(copy);
	
	rc = UV_ERR_OK;
	
error:
	return rc;
}

std::vector<std::string> charPtrArrayToVector(char *const *argv, int argc)
{
	std::vector<std::string> ret;
	for( int i = 0; i < argc; ++i )
	{
		ret.push_back(argv[i]);
	}
	return ret;
}

uv_err_t getArguments(const std::string &in, std::vector<std::string> &out)
{
	std::string cur;
	int parenCount = 0;
	
	UV_ENTER();
	
	for( std::string::size_type i = 0; ; ++i )
	{
		char c  = 0;
		if( i < in.size() )
		{
			c = in[i];
		}
		
		if( c == '(' )
		{
			++parenCount;
			cur += c;
		}
		else if( c == ')' )
		{
			if( parenCount == 0 )
			{
				return UV_DEBUG(UV_ERR_GENERAL);
			}
			--parenCount;
			cur += c;
		}
		//Nested function argument?
		else if( c == ',' && parenCount > 0 )
		{
			cur += c;
		}
		//End of argument?
		else if( c == ',' || i >= in.size() )
		{
			//Current level argument
			if( cur.empty() )
			{
				return UV_DEBUG(UV_ERR_GENERAL);
			}
			out.push_back(cur);
			cur = "";
			
			//Done?
			if( i >= in.size() )
			{
				break;
			}
		}
		//Otherwise add to current
		else
		{
			cur += c;
		}
	}

	printf_debug("Separated: %s, parts: %d\n", in.c_str(), out.size());
	
	return UV_ERR_OK;
}

uint64_t getTimingMicroseconds(void)
{
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv, &tz);
	tm=localtime(&tv.tv_sec);
	return ((tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec) * 1000000 + tv.tv_usec;
}

uv_err_t isCSymbol(const std::string &in)
{
	//[a-zA-Z_][z-zA-Z0-9_].

	if( in.empty() )
	{
		return UV_ERR_GENERAL;
	}
	
	//[a-zA-Z_]
	if( !((in[0] >= 'a' && in[0] <= 'z')
			|| (in[0] >= 'A' && in[0] <= 'Z')
			|| in[0] == '_') )
	{
		return UV_ERR_GENERAL;
	}
	
	//[z-zA-Z0-9_].
	for( std::string::size_type i = 1; i < in.size(); ++i )
	{
		if( !((in[0] >= 'a' && in[0] <= 'z')
				|| (in[0] >= 'A' && in[0] <= 'Z')
				|| (in[0] >= '0' && in[0] <= '9')
				|| in[0] == '_') )
		{
			return UV_ERR_GENERAL;
		}
	}

	return UV_ERR_OK;
}

uv_err_t isConfigIdentifier(const std::string &in)
{
	return isCSymbol(in);
}

uint32_t g_bytesPerRow = 16;
uint32_t g_bytesPerHalfRow = 8;

static unsigned int hexdumpHalfRow(const uint8_t *data, size_t size, uint32_t start)
{
	uint32_t col = 0;

	for( ; col < g_bytesPerHalfRow && start + col < size; ++col )
	{
		uint32_t index = start + col;
		uint8_t c = data[index];
		
		printf("%.2X ", (unsigned int)c);
		fflush(stdout);
	}

	//pad remaining
	while( col < g_bytesPerHalfRow )
	{
		printf("   ");
		fflush(stdout);
		++col;
	}

	//End pad
	printf(" ");
	fflush(stdout);

	return start + g_bytesPerHalfRow;
}

void UVDHexdumpCore(const uint8_t *data, size_t size, const std::string &prefix)
{
	/*
	[mcmaster@gespenst icd2prog-0.3.0]$ hexdump -C /bin/ls |head
	00000000  7f 45 4c 46 01 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
	00000010  02 00 03 00 01 00 00 00  f0 99 04 08 34 00 00 00  |............4...|
	00017380  00 00 00 00 01 00 00 00  00 00 00 00              |............|
	*/

	size_t pos = 0;
	while( pos < size )
	{
		uint32_t row_start = pos;
		uint32_t i = 0;

		printf("%s", prefix.c_str());
		fflush(stdout);

		pos = hexdumpHalfRow(data, size, pos);
		pos = hexdumpHalfRow(data, size, pos);

		printf("|");
		fflush(stdout);

		//Char view
		for( i = row_start; i < row_start + g_bytesPerRow && i < size; ++i )
		{
			char c = data[i];
			if( isprint(c) )
			{
				printf("%c", c);
				fflush(stdout);
			}
			else
			{
				printf("%c", '.');
				fflush(stdout);
			}
		} 
		for( ; i < row_start + g_bytesPerRow; ++i )
		{
			printf(" ");
			fflush(stdout);
		}

		printf("|\n");
		fflush(stdout);
	}
	fflush(stdout);
}

void UVDHexdump(const uint8_t *data, size_t size)
{
	UVDHexdumpCore(data, size, "");
}

uv_err_t UVDPrintToStdoutStringCallback(const std::string &s, void *user)
{
	printf("%s", s.c_str());
	return UV_ERR_OK;
}

uv_err_t UVDPrintToFileStringCallback(const std::string &s, void *user)
{
	fprintf((FILE *)user, "%s", s.c_str());
	return UV_ERR_OK;
}

uv_err_t UVDPrintToStringStringCallback(const std::string &cur, void *user)
{
	std::string &output = *(std::string *)user;
	
	output += cur;
	return UV_ERR_OK;
}

