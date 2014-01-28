#pragma once

//#ifndef ASSERT
//#define ASSERT(condition) if (!(condition)) ::DebugBreak();
//#endif

namespace trace {

enum _
{
	quiet = 0,
	fatal = 1,
	error = 2,
	warning = 3,
	info = 4,	
	verbose = 5,
	debug = 6	
};

class Writer;

void writeLine(int32_t,const std::string&);
void addWriter(const std::string&, const std::shared_ptr<Writer>&);	
void removeWriter(const std::string&);
void clearWriters();

class Writer
{
public:
	virtual void writeLine(int32_t,const std::string&)=0;
};

#define ___LOG_DEBUG(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::debug, ##__VA_ARGS__)
#define ___LOG_VERBOSE(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::verbose, ##__VA_ARGS__)
#define ___LOG_INFO(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::info, ##__VA_ARGS__)
#define ___LOG_WARN(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::warning, ##__VA_ARGS__)
#define ___LOG_ERROR(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::error, ##__VA_ARGS__)
#define ___LOG_FATAL(clsname, ...) \
	clsname(__FILE__, __LINE__, trace::fatal, ##__VA_ARGS__)

#define ___LOG_WRAP_DEBUG \
  ___LOG_DEBUG(trace::__call)
#define ___LOG_WRAP_VERBOSE \
  ___LOG_VERBOSE(trace::__call)
#define ___LOG_WRAP_INFO \
  ___LOG_INFO(trace::__call)
#define ___LOG_WRAP_WARN \
  ___LOG_WARN(trace::__call)
#define ___LOG_WRAP_ERR \
  ___LOG_ERROR(trace::__call)
#define ___LOG_WRAP_FATAL \
  ___LOG_FATAL(trace::__call)

#define LOG(severity) ___LOG_WRAP_ ## severity.stream()

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : __voidify() & LOG(severity)

#define CHECK(condition) \
  LOG_IF(FATAL, !(condition)) << "check failed: (" #condition ") "

class __call
{
public:
	__call(const char* file, int32_t line, int32_t severity)
		: _severity(severity)
	{
		init(file, line);
	}

	~__call();

	std::ostream& stream() { return _stream; }

private:

	void init(const char*, int32_t);

	std::ostringstream	_stream;
	int32_t _severity;
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class __voidify
{
public:
   __voidify() { }
  
   // This has to be an operator with a precedence lower than << but
   // higher than ?:
   void operator&(std::ostream&) { }
};

}; // trace