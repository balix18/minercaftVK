#pragma once
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <ostream>
#include <sstream>
#include <mutex>
#include <functional>
#include <thread>

#ifdef LOGGER_USING_FMT
#include <fmt/format.h>
#endif

namespace ezpz {
	enum LogLevel : unsigned char {
        FATAL = 1,
        ERROR = 2,
        WARN = 4,
		INFO = 8,
		DEBUG = 16,
        SERVER = 32,
        ALL = 255
	};
	std::ostream& operator<<(std::ostream& os, LogLevel logLevel);
    std::string toString(LogLevel level);
    bool HasLogLevelBit(unsigned char bits, LogLevel logLevel);

    class ILogOutput {
    public:
       virtual void Log(LogLevel level, int tick, std::thread::id threadId, const std::string& data) = 0;
       virtual void SetVersion(const std::string& strVersion) = 0;
       virtual void SetStats(const std::string& strStats) = 0;
       virtual ~ILogOutput() = default;
    };

    struct OutputDescriptor {
        std::string pathFormat, lineFormat;
        bool bVersion, bStats, hasUserTag;
        unsigned char channels;

        void Reset()
        {
            pathFormat = "communication.log";
            lineFormat = "{{Message}}";
            bVersion = true;
            bStats = true;
            hasUserTag = false;
            channels = LogLevel::ALL;
        }
    };

    class LogOutputStream : public ILogOutput {
    public:
        LogOutputStream(OutputDescriptor desc, std::ostream& os) : os{os}, desc{desc} {}
        ~LogOutputStream() = default;
        void Log(LogLevel logLevel, int tick, std::thread::id threadId, const std::string& data) override;
        void SetVersion(const std::string& strVersion) override {
            if (desc.bVersion) os << "[VERSION]\n" << strVersion << std::endl << "[\\VERSION]\n"; 
        }
        void SetStats(const std::string& strStats) override {
            if (desc.bStats) os << "[STATS]\n" << strStats << std::endl << "[\\STATS]\n"; 
        }

    private:
        OutputDescriptor desc;
        std::ostream& os;
    };

    class LogOutputFile : public LogOutputStream {
    public:
        LogOutputFile(OutputDescriptor desc);

    private:
        std::ofstream file;
    };

    class LogOutputMemory : public ILogOutput {
    public:
        ~LogOutputMemory() = default;
        void Log(LogLevel logLevel, int tick, std::thread::id threadId, const std::string& data) override {
            logs.push_back(LogData{logLevel, tick, threadId, data});
        }

        void SetVersion(const std::string& strVersion) override {
            this->strVersion = strVersion; 
        }

        void SetStats(const std::string& strStats) override {
            this->strStats = strStats; 
        }

        void WriteDataToOutput(ILogOutput& output) const {
            if (!strVersion.empty()) output.SetVersion(strVersion);
            for (auto& it : logs) {
                output.Log(it.level, it.tick, it.threadId, it.data);
            }
            if (!strStats.empty()) output.SetStats(strStats);
        }

    private:
        struct LogData {
            LogLevel level;
            int tick;
            std::thread::id threadId;
            std::string data;
        };

    private:
        std::vector<LogData> logs;
        std::string strStats, strVersion;
    };

    namespace impl {
        inline void log_impl(std::stringstream& ss) {}

        template <typename T>
        inline void log_impl(std::stringstream& ss, T&& val) {
            ss << val;
        }

        template <typename T, typename... Tail>
        inline void log_impl(std::stringstream& ss, T&& val, Tail&&... args) {
            ss << val;
            return log_impl(ss, args...);
        }
    }

    struct DefaultToString {
        std::string operator()(LogLevel logLevel);
    };

    struct ShortToString {
        std::string operator()(LogLevel logLevel);
    };

	class Logger {
	private:
		Logger() = default;
		~Logger() = default;
		Logger(Logger const&) = delete;
		Logger& operator=(Logger const&) = delete;
	public:
		static Logger& Instance() {
			static Logger instance;
			return instance;
		}

	private:
		std::unique_ptr<LogOutputMemory> memoryLog;
        std::vector<std::unique_ptr<ILogOutput>> outputLogs;
        std::vector<OutputDescriptor> outputDescriptors;
		int tick = -1;
        mutable std::mutex mutex;

	public:
        std::string Init(const std::string& path);
        
        void SetUsertag(const std::string& usertag, const std::string& replacement)
        {
#ifdef EZPZLOGGER_USE_LOCK
            std::lock_guard<std::mutex> _l{mutex};
#endif
            SetUsertag_Impl(usertag, replacement);
        }

        void AddOutputChannel(std::unique_ptr<ILogOutput> pOutput)
        {
#ifdef EZPZLOGGER_USE_LOCK
            std::lock_guard<std::mutex> _l{mutex};
#endif
            AddOutputChannel_Impl(std::move(pOutput));
        }

        std::function<std::string(LogLevel)> LogLevelToString = DefaultToString{};
        template <typename... Args> void Log(LogLevel logLevel, Args&&... args) {
#ifndef LOGGER_USING_FMT
            std::stringstream ss;
            impl::log_impl(ss, args...);

#ifdef EZPZLOGGER_USE_LOCK
            std::lock_guard<std::mutex> _l{mutex};
#endif
            Log_Impl(logLevel, ss.str());
#else
#ifdef EZPZLOGGER_USE_LOCK
            std::lock_guard<std::mutex> _l{mutex};
#endif
            Log_Impl(logLevel, fmt::format(args...));
#endif
        }

        template <typename... Args> void LogFatal(Args&&... args)
        {
            Log(LogLevel::FATAL, args...);
        }
        template <typename... Args> void LogError(Args&&... args) {
            Log(LogLevel::ERROR, args...);
        }
        template <typename... Args> void LogWarning(Args&&... args)
        {
            Log(LogLevel::WARN, args...);
        }
        template <typename... Args> void LogInfo(Args&&... args) {
            Log(LogLevel::INFO, args...);
        }
        template <typename... Args> void LogDebug(Args&&... args) {
            Log(LogLevel::DEBUG, args...);
        }
        template <typename... Args> void LogServer(Args&&... args) {
            Log(LogLevel::SERVER, args...);
        }
        template <typename T> void SetVersion(const T& version) {
            auto str = version.toString();
            memoryLog->SetVersion(str);
            for (auto& it : outputLogs) {
                it->SetVersion(str);
            }
        }
        template <typename T> void SetStats(const T& stats) {
            auto str = stats.toString();
            memoryLog->SetStats(str);
            for (auto& it : outputLogs) {
                it->SetStats(str);
            }
        }
		void StartNextTick();

        void SetToString(std::function<std::string(LogLevel)> formatter) {
            LogLevelToString = std::move(formatter);
        }

    private:
		void Log_Impl(LogLevel logLevel, std::string const& str);
        void AddOutputChannel_Impl(std::unique_ptr<ILogOutput> pOutput);
        void SetUsertag_Impl(const std::string& usertag, const std::string& replacement);
	};

	inline Logger& theLogger = Logger::Instance();
}
