#ifndef _LOG_
#define _LOG_
#include <time.h>

typedef enum {
	INFO,
	ERROR,
	WARNING,
	DETAIL,
	DEBUG
} ELOGLEVEL;

class CLog
{
	public:
		CLog();

		void LOG(ELOGLEVEL level, const char* fmt, ...);

	public:
		static void setTTY(const char* ttyname);
		static void openLogFile();
		static const char* getTTYName();
		static void getCindir();
		static char s_cindir[64];

	protected:
	    const char* getClassName();

	protected:
		static ELOGLEVEL s_level;

	private:
		char* getTime()
		{
			struct tm *ptr;
			time_t t;
			time(&t);
			ptr=localtime(&t);
			strftime(m_asctime, 100, "%H:%M:%S", ptr);
			return m_asctime;
		};

	private:
		char m_asctime[32];

	private:
		static char s_ttyName[64];
		static int s_ttyfd;
		static int s_filefd;
};

#endif
/* vi:set ts=4 sw=4 noet: */
