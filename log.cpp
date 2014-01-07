#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

char CLog::s_ttyName[64] = {0};
char CLog::s_cindir[64] = {0};
int CLog::s_ttyfd = 0;
int CLog::s_filefd = -1;
ELOGLEVEL CLog::s_level = DETAIL;

static const char* LOGLEVELNAMELIST[]=
{
	"INFO",
	"ERROR",
	"WARNING",
	"DETAIL",
	"DEBUG"
};
const char* CLog::getClassName()
{
	return "Simualate";
}
void CLog::setTTY(const char* ttyname)
{
	int fd = open(ttyname, O_WRONLY);
	if(fd > 0) 
	{
		s_ttyfd = fd;
		strncpy(s_ttyName, ttyname, sizeof(s_ttyName));
	}
}

void CLog::openLogFile()
{
	char logfile[128];
	snprintf(logfile, sizeof(logfile), "%s/simulate.log", s_cindir);
	s_filefd = open(logfile, O_WRONLY|O_CREAT, 0664);
	if(s_filefd == -1)
		printf("Failed with open log file(%s): %s", logfile, strerror(errno));
	else
		printf("Open log file success\n");
}

void CLog::getCindir()
{
	char*p =  getenv("CINDIR");
	strncpy(s_cindir, p, sizeof(s_cindir));
}

const char* CLog::getTTYName()
{
	return s_ttyName;
}

CLog::CLog()
{
	getCindir();
	
	memset(m_asctime, 0, sizeof(m_asctime));
	openLogFile();
}

void CLog::LOG(ELOGLEVEL level, const char* fmt, ...)
{
	if(s_ttyfd < 0 || level > s_level || level < INFO || level > DEBUG)
		return;

	char temp[900];
	va_list args;
	va_start(args, fmt);
	vsnprintf(temp, sizeof(temp), fmt, args);
	va_end(args); 
	if(temp[strlen(temp)-1] == '\n')
		temp[strlen(temp)-1] = '\0';

	char buffer[900];
	const char* LOGFORMAT[] = {
		"\033[4;47;30m", //INFO
		"\033[5;47;31m", //ERROR
		"\033[1;47;31m", //WARNING
		"\033[0;47;34m", //DETAIL
		"\033[1;47;36m", //DEBUG
	};

	snprintf(buffer, sizeof(buffer),
			"[%s] %s%s\033[0m <%s> %s\n",
			getTime(),
			LOGFORMAT[level],
			LOGLEVELNAMELIST[level],
			getClassName(),
			temp);

	int t;
	t = write(s_ttyfd, buffer, strlen(buffer));
	if(s_filefd > 0 && (level <= WARNING)) {
		snprintf(buffer, sizeof(buffer),
				"[%s] %s\033[0m <%s> %s\n",
				getTime(),
				LOGLEVELNAMELIST[level],
				getClassName(),
				temp);

		t = write(s_filefd, buffer, strlen(buffer));
	}
}

/* vi:set ts=4 sw=4 noet: */
