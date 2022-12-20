#ifndef __LOGGER_H__
#define __LOGGER_H__

/*
 * Copyright 2013 Matthew Lai
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <ctime>
#include <cstdint>

// convenience macros
#define LOG_DEBUG(...) logger.Log(Logger::eLevel::DEBUG, __FILE__ ":" LOGGER_STR(__LINE__) ": " __VA_ARGS__)
#define LOG_INFO(...) logger.Log(Logger::eLevel::INFO, __FILE__ ":" LOGGER_STR(__LINE__) ": " __VA_ARGS__)
#define LOG_WARN(...) logger.Log(Logger::eLevel::WARN, __FILE__ ":" LOGGER_STR(__LINE__) ": " __VA_ARGS__)
#define LOG_ERROR(...) logger.Log(Logger::eLevel::ERROR, __FILE__ ":" LOGGER_STR(__LINE__) ": " __VA_ARGS__)
#define LOG_FATAL(...) logger.Log(Logger::eLevel::FATAL, __FILE__ ":" LOGGER_STR(__LINE__) ": " __VA_ARGS__)

#define LOGGER_STR1(x) #x
#define LOGGER_STR(x) LOGGER_STR1(x)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26110)
#endif

template <typename T, typename... Params>
std::string FormatString(std::string format, T param, Params... remaining_params);

// base case
std::string FormatString(std::string format);

namespace Logger
{
	enum class eLevel
	{
		DEBUG = 0,
		INFO = 1,
		WARN = 2,
		ERROR = 3,
		FATAL = 4,
		NUM_LOG_LEVELS,
		
		/* special values */
		DISABLE
	};
	
	class ILogDestination
	{
	public:
		virtual void Log(eLevel level, std::string message) = 0;
	};
	
	class RateLimiter
	{
	public:
		/// default constructed RateLimiter is no-op
		RateLimiter() :
			m_tokenRate(0.0f),
			m_numTicksPerPeriod(0),
			m_tokens(0.0f),
			m_disabled(true)
		{}
	
		/// Tick() will block to limit firing rate to num per t milliseconds
		RateLimiter(std::chrono::milliseconds t, uint32_t num) :
			m_tokenRate(static_cast<float>(num) / t.count()), 
			m_numTicksPerPeriod(num), 
			m_tokens(static_cast<float>(num)), 
			m_disabled(false) 
			{}
		
		/// returns whether Tick() blocked (exceeding rate limit)
		bool Tick();
	private:
		float m_tokenRate; // how many tokens per millisecond
		std::chrono::system_clock::time_point m_lastTick;
		uint32_t m_numTicksPerPeriod;
		float m_tokens;
		bool m_disabled;
	};
	
	inline bool RateLimiter::Tick()
	{
		if (m_disabled)
		{
			return false;
		}
	
		// this is the token bucket algorithm
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		
		std::chrono::milliseconds time_since_last_tick = 
			std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTick);
		
		m_tokens += m_tokenRate * time_since_last_tick.count();
		
		if (m_tokens > m_numTicksPerPeriod)
		{
			m_tokens = static_cast<float>(m_numTicksPerPeriod);
		}
		
		bool blocked = false;
		
		if (m_tokens < 1.0f)
		{
			blocked = true;
			
			float deficit = 1.0f - m_tokens;
			
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(deficit / m_tokenRate)));
			
			m_tokens = 1.0f;
		}
		
		m_lastTick = std::chrono::system_clock::now();
		m_tokens -= 1.0f;
		return blocked;
	}
		
	class Logger
	{
	public:
		Logger(std::string prefix = "") : m_stdErrLevel(eLevel::WARN), m_stdOutLevel(eLevel::DISABLE), m_prefix(prefix) {}
		
		/// Sets log file for one log level. All log messages at or above the log level
		/// will be appended to the file. 
		/// To have the logger automatically generate a filename based on current date and time,
		/// pass an empty filename, and specify a prefix for the automatically generated file (eg.
		/// directory). Prefix is ignored if a filename is specified.
		std::string SetLogFile(eLevel level, std::string filename = "", std::string autogen_prefix = "");
		
		/// Closes and stops logging to the log file associated with the specified level
		void UnsetLogFile(eLevel level) { std::lock_guard<std::mutex> lock(m_mutex); m_logfiles.erase(level); }
		
		/// Sets the minimum log level for stderr, or disables logging to stderr
		/// default WARN
		void LogToStdErrLevel(eLevel level) { std::lock_guard<std::mutex> lock(m_mutex); m_stdErrLevel = level; }
		
		/// Sets the minimum log level for stdout, or disables logging to stdout
		/// default DISABLE
		void LogToStdOutLevel(eLevel level) { std::lock_guard<std::mutex> lock(m_mutex); m_stdOutLevel = level; }
		
		/// Log a message
		template <typename... Params>
		std::string Log(eLevel level, std::string format, Params... parameters);
		
		/// Have Logger limit message rate to num per t milliseconds
		void SetRateLimit(uint32_t t_ms, uint32_t num) 
			{ SetRateLimit(std::chrono::milliseconds(t_ms), num); }
		void SetRateLimit(std::chrono::milliseconds t, uint32_t num) 
			{ std::lock_guard<std::mutex> lock(m_mutex); m_rateLimiter = RateLimiter(t, num); }
			
		void RegisterDestination(std::shared_ptr<ILogDestination> dest) { m_destinations.insert(dest); }
		void RemoveDestination(std::shared_ptr<ILogDestination> dest) { m_destinations.erase(dest); }
		
		Logger &operator=(const Logger&) = delete;
		Logger(const Logger&) = delete;

	private:
		std::string GenerateLogFilename_(eLevel level, std::string prefix = "");
	
		std::string GenerateMessageWithPrefixSuffix_(eLevel level, std::string message);
		
		bool ShouldLog_(eLevel message_level, eLevel target_level) 
			{ return target_level != eLevel::DISABLE && target_level <= message_level; }
		
		// this is a hacky way to avoid having to put them in their own cpp file
		static std::mutex& GetStdOutMutex_() { static std::mutex std_out_mutex; return std_out_mutex; }
		static std::mutex& GetStdErrMutex_() { static std::mutex std_err_mutex; return std_err_mutex; }
		
		std::string LevelToString_(eLevel level);
	
		std::map<eLevel, std::string> m_logfiles;
		
		eLevel m_stdErrLevel;
		eLevel m_stdOutLevel;
		
		std::string m_prefix;
		
		RateLimiter m_rateLimiter;
		
		std::set<std::shared_ptr<ILogDestination> > m_destinations;
		
		std::mutex m_mutex;
	};
	
	inline std::string Logger::SetLogFile(eLevel level, std::string filename, std::string autogen_prefix) 
	{
		std::lock_guard<std::mutex> lock(m_mutex);
	
		if (filename == "")
		{
			filename = GenerateLogFilename_(level, autogen_prefix);
		}
		
		m_logfiles[level] = filename;
		return filename;
	}
	
	template <typename... Params>
	inline std::string Logger::Log(eLevel level, std::string format, Params... params)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		bool throttled = m_rateLimiter.Tick();
		
		std::string message = FormatString(format, params...);
		
		if (throttled)
		{
			message.append(" (throttled)");
		}
		
		std::string message_with_prefix_suffix = GenerateMessageWithPrefixSuffix_(level, message);

		if (ShouldLog_(level, m_stdErrLevel))
		{
			std::lock_guard<std::mutex> lock(GetStdErrMutex_());

			int colour_code = 39; // FG_DEFAULT

			if (level == eLevel::WARN) {
				colour_code = 33; // Yellow
			} else if (level == eLevel::ERROR || level == eLevel::FATAL) {
				colour_code = 31; // Red
			}
		
			std::cerr << "\033[" << colour_code << "m" << message_with_prefix_suffix << "\033[0m";
			std::cerr.flush();
		}
		
		if (ShouldLog_(level, m_stdOutLevel))
		{
			std::lock_guard<std::mutex> lock(GetStdOutMutex_());
			
			std::cout << message_with_prefix_suffix;
			std::cout.flush();
		}
		
		for (const auto &x : m_logfiles)
		{
			if (ShouldLog_(level, x.first))
			{
				std::ofstream of(x.second, std::ios::app);
				
				if (of.good())
				{			
					of << message_with_prefix_suffix;
					of.flush();
				}
				
				if (!of.good())
				{
					// is there a better way to deal with this?
					std::cerr << "Failed writing to log file " << x.second << std::endl;
				}
			}
		}
		
		for (const auto &x : m_destinations)
		{
			x->Log(level, message_with_prefix_suffix);
		}
		
		return message_with_prefix_suffix;
	}
	
	inline std::string Logger::GenerateLogFilename_(eLevel level, std::string prefix) 
	{ 
		time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		tm *timeinfo = localtime(&t);
		
		char time_str[100];
		strftime(time_str, 100, "%Y-%m-%d_%H-%M-%S", timeinfo);
		
		return prefix + time_str + "_" + LevelToString_(level) + ".log";
	}
	
	inline std::string Logger::GenerateMessageWithPrefixSuffix_(eLevel level, std::string message)
	{
		std::string ret;
		
		switch (level)
		{
		case eLevel::DEBUG:
			ret.append("[D]");
			break;
		case eLevel::INFO:
			ret.append("[I]");
			break;
		case eLevel::WARN:
			ret.append("[W]");
			break;
		case eLevel::ERROR:
			ret.append("[E]");
			break;
		case eLevel::FATAL:
			ret.append("[F]");
			break;
		default:
			ret.append("[U]");
			break;
		}
		
		std::stringstream formatted;
		
		// use strftime to get up to second, then manually add milliseconds
		// yes, this is epic ugly
		
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		
		time_t t = std::chrono::system_clock::to_time_t(now);
		tm *timeinfo = localtime(&t);
		
		char buf[100];
		strftime(buf, 100, "%Y-%m-%d %X", timeinfo);
		uint64_t ms = (now.time_since_epoch() / std::chrono::milliseconds(1)) % 1000;
		
		formatted << "[" << buf << "." << std::setw(3) << std::setfill('0') << ms << "]" ;
		
		ret.append(formatted.str());
		
		if (m_prefix != "")
		{
			ret.append(m_prefix);
		}
		
		ret.append(" ");
		ret.append(message);
		
		if (ret[ret.size() - 1] != '\n')
		{
			ret.push_back('\n');
		}
		
		return ret;
	}
	
	inline std::string Logger::LevelToString_(eLevel level)
	{
		switch (level)
		{
		case eLevel::DEBUG:
			return "DEBUG";
		case eLevel::INFO:
			return "INFO";
		case eLevel::WARN:
			return "WARN";
		case eLevel::ERROR:
			return "ERROR";
		case eLevel::FATAL:
			return "FATAL";
		default:
			return "UNKNOWN_LOG_LEVEL";
		}
	}
}

template <typename T, typename... Params>
inline std::string FormatString(std::string format, T param, Params... remaining_params)
{
	std::string ret;

	for (size_t i = 0; i < format.size(); ++i)
	{
		if (format[i] == '%')
		{
			// if we see a %, we throw it away, unless the next character is also %
			if(i != (format.size() - 1) && format[i+1] == '%')
			{
				ret.push_back('%');
				++i; // skip the next %
			}
			else
			{
				std::stringstream ss;
				ss << std::fixed << std::setprecision(2) << param;
				ret.append(ss.str());
				
				if (i != (format.size() - 1))
				{
					ret.append(FormatString(format.substr(i + 1), remaining_params...));
				}
				
				break;
			}
		}
		else
		{				
			ret.push_back(format[i]);
		}
	}
	
	return ret;
}

// base case
inline std::string FormatString(std::string format)
{
	return format;
}

extern Logger::Logger logger;

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // __LOGGER_H__

