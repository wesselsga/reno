#include "core.h"
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

namespace trace {

// in debug mode; we'll use a writer 
// for OutputDebugString
#ifdef _DEBUG
class DebugWriter : public Writer
{
public:
	void writeLine(int32_t,const std::string&);
};
#endif

//
// write to std::cout
//
class StdWriter : public Writer
{
public:
	void writeLine(int32_t,const std::string&);
};

//
// manage collection of writers
//
class Log
{
public:
	Log();
	~Log(){}
	
	void writeLine(int32_t,const std::string&);
	
	void addWriter(const std::string&, const std::shared_ptr<Writer>&);	
	void removeWriter(const std::string&);
	void clearWriters();
		
private:	
	typedef std::map<std::string,std::shared_ptr<Writer> > writers;
	writers _writers;
};

// the one and only log object
std::shared_ptr<Log> _log(new Log());

void writeLine(int32_t severity, const std::string& msg)
{
	_log->writeLine(severity, msg);
}

//
// registers a new log writer with the given name
//
void addWriter(const std::string& key, const std::shared_ptr<Writer>& out)
{
	_log->addWriter(key, out);
}

//
// removes the log writer by name
//
void removeWriter(const std::string& key)
{
	_log->removeWriter(key);
}

//
// removes all log writers
//
void clearWriters()
{
	_log->clearWriters();
}

__call::~__call()
{
	_stream << std::endl; // auto-newline

	trace::writeLine(_severity,_stream.str().c_str());

	if (_severity == fatal)
   {
#ifdef _DEBUG

#ifdef _WIN32
		//ASSERT(0);
#endif
#else
		throw std::bad_exception(_stream.str().c_str());
#endif

   }
}

void __call::init(const char* file, int32_t line)
{
	const char* last_slash = strrchr(file, '/');
	if (!last_slash){
		last_slash = strrchr(file, '\\');
	}

	if (last_slash){
		file = last_slash + 1;
	}

	_stream << "[";

	switch (_severity)
	{
		case verbose: _stream << "V"; break;
		case info   : _stream << "I"; break;
		case warning: _stream << "W"; break;
		case error  : _stream << "E"; break;
		case debug  :	
		default     : _stream << "D";
			break;
	}

	//if (log_timestamp) 
	{
		uint32_t hours, mins, secs, ms;

#ifdef _WIN32
		SYSTEMTIME st;
		::GetLocalTime(&st);
		hours = st.wHour;
		mins = st.wMinute;
		secs = st.wSecond;
		ms = st.wMilliseconds;
#else
		struct timeval now;
		gettimeofday(&now, 0);
		struct tm* t = localtime(&now.tv_sec);
		hours = t->tm_hour;
		mins = t->tm_min;
		secs = t->tm_sec;
		ms = now.tv_usec / 1000;
#endif
    
		_stream << '|'
				<< std::setfill('0')
				<< std::setw(2) << hours
				<< ":" << std::setw(2) << mins
				<< ":" << std::setw(2) << secs
				<< ":" << std::setw(3) << ms;
	}

	_stream << "|~" 
				<< std::setfill('0') 
				<< std::setw(4) << uv_thread_self();
	//_stream << "|" << file << "(" << line << ")] ";
	_stream << "] ";
}


#ifdef _DEBUG
void DebugWriter::writeLine(int32_t,const std::string& msg)
{
#ifdef WIN32
	::OutputDebugStringA(msg.c_str());
#endif
}
#endif

void StdWriter::writeLine(int32_t,const std::string& msg)
{
	std::cout << msg;
}



Log::Log()
{
#ifdef _DEBUG
	std::shared_ptr<Writer> dbg(new DebugWriter());
	addWriter(":debug", dbg);
#endif

	std::shared_ptr<Writer> std(new StdWriter());
	addWriter(":stdout", std);
}

void Log::addWriter(const std::string& key, 
		const std::shared_ptr<Writer>& out)
{
	if (out.get())
	{
		removeWriter(key);
		_writers[key] = out;
	}
}

void Log::removeWriter(const std::string& key)
{
	writers::iterator i = _writers.find(key);
	if (i != _writers.end()){
		_writers.erase(i);
	}
}

void Log::clearWriters()
{
	_writers.clear();
}

//
// simply sends the message to all registered listeners
//
void Log::writeLine(int32_t severity, const std::string& msg)
{
	writers::const_iterator i;
	for (i = _writers.cbegin(); i != _writers.cend(); i++)
	{
		i->second->writeLine(severity, msg);
	}
}

}; // trace