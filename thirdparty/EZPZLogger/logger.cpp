#include "logger.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;
using namespace std::chrono;

namespace ezpz {
    std::string replaceString(std::string subject, const std::string& search, const std::string& replace) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
             subject.replace(pos, search.length(), replace);
             pos += replace.length();
        }
        return subject;
    }

    // Based on: https://stackoverflow.com/questions/15957805/extract-year-month-day-etc-from-stdchronotime-point-in-c
    std::string replaceTime(std::string subject, system_clock::time_point tp) {
        typedef duration<int, std::ratio_multiply<hours::period, std::ratio<24> >::type> days;
        system_clock::duration d = tp.time_since_epoch();
        d -= duration_cast<days>(d);
        d -= duration_cast<hours>(d);
        d -= duration_cast<minutes>(d);
        d -= duration_cast<seconds>(d);

        milliseconds ms = duration_cast<milliseconds>(d);

        time_t tt = system_clock::to_time_t(tp);
        auto t = localtime(&tt);

        auto year = std::to_string(t->tm_year + 1900);
        auto month = std::to_string(t->tm_mon + 1);
        auto day = std::to_string(t->tm_mday);
        auto hour = std::to_string(t->tm_hour);
        auto minute = std::to_string(t->tm_min);
        auto second = std::to_string(t->tm_sec);
        auto msstr = std::to_string(ms.count());

        subject = replaceString(subject, "{{Year}}", year);
        subject = replaceString(subject, "{{Month}}", month);
        subject = replaceString(subject, "{{Day}}", day);
        subject = replaceString(subject, "{{Hour}}", hour);
        subject = replaceString(subject, "{{Minute}}", minute);
        subject = replaceString(subject, "{{Second}}", second);
        subject = replaceString(subject, "{{Ms}}", msstr);

        return subject;
    }

    std::vector<std::string> split(const std::string& str, char delim)
    {
        std::stringstream ss(str);
        std::string tmp;
        std::vector<std::string> tokens;
        while (getline(ss, tmp, delim))
        {
            tokens.push_back(tmp);
        }
        return tokens;
    }

    // trim from start (in place)
    void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    std::string DefaultToString::operator()(LogLevel logLevel)
    {
        switch (logLevel)
        {
            case LogLevel::FATAL:
                return "FATAL";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::WARN:
                return "WARNING";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::SERVER:
                return "SERVER";
            default:
                return "UNKNOWN";
        }
    }

    std::string ShortToString::operator()(LogLevel logLevel)
    {
        switch (logLevel)
        {
            case LogLevel::FATAL:
                return "FAT";
            case LogLevel::ERROR:
                return "ERR";
            case LogLevel::WARN:
                return "WRN";
            case LogLevel::INFO:
                return "INF";
            case LogLevel::DEBUG:
                return "DBG";
            case LogLevel::SERVER:
                return "SRV";
            default:
                return "UNK";
        }
    }

    std::string Logger::Init(const std::string& path)
	{
        memoryLog = std::make_unique<LogOutputMemory>(); 

        std::ifstream file{path};

        auto t = system_clock::now();

        OutputDescriptor desc;
        desc.Reset();
        bool descValid = false;

        auto finishDesc = [&](){
            if (descValid) {
                if (desc.hasUserTag) {
                    outputDescriptors.push_back(desc);
                } else {
                    if (desc.pathFormat == "cerr") {
                        outputLogs.push_back(std::make_unique<LogOutputStream>(desc, std::cerr));
                    } else if (desc.pathFormat == "cout") {
                        outputLogs.push_back(std::make_unique<LogOutputStream>(desc, std::cout));
                    } else {
                        outputLogs.push_back(std::make_unique<LogOutputFile>(desc));
                    }
                }

                desc.Reset();
            }
            descValid = true;
        };
       
        std::stringstream error;
        int lineCount = 0;
        std::string line = "";
        while (std::getline(file, line)) {
            lineCount++;
            trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            } else if (line.front() == '[') { // new output
                finishDesc();

                if (line.back() != ']') {
                    error << "Line " << lineCount << ": closing ] is missing.\n";
                    continue;
                }

                line = line.substr(1, line.size() - 2); // remove [ ]
                line = replaceTime(line, t);

                desc.pathFormat = line;
                desc.hasUserTag = desc.pathFormat.find("{$", 0) != std::string::npos;
            } else if (line.find('=', 0) == std::string::npos) {
                error << "Line " << lineCount << ": separator (=) is missing.\n";
                continue;
            } else {
                auto splitted = split(line, '=');
                if (splitted.size() != 2) {
                    error << "Line " << lineCount << ": multiple separators (=) are found.\n";
                    continue;
                }
                trim(splitted[0]);
                trim(splitted[1]);
                if (splitted[0] == "channels") {
                    desc.channels = 0;
                    for (auto c : splitted[1]) {
                        switch (c) {
                        case 'F':
                            desc.channels |= LogLevel::FATAL;
                            break;
                        case 'E':
                            desc.channels |= LogLevel::ERROR;
                            break;
                        case 'W':
                            desc.channels |= LogLevel::WARN;
                            break;
                        case 'I':
                            desc.channels |= LogLevel::INFO;
                            break;
                        case 'D':
                            desc.channels |= LogLevel::DEBUG;
                            break;
                        case 'S':
                            desc.channels |= LogLevel::SERVER;
                            break;
                        default:
                            error << "Line " << lineCount << ": '" << c << "' is ignored. Valid characters: I|E|S|D.\n";
                            break;
                        }
                    }
                } else if (splitted[0] == "stats") {
                    if (splitted[1] == "true") {
                        desc.bStats = true;
                    } else if (splitted[1] == "false") {
                        desc.bStats = false;
                    } else {
                        error << "Line " << lineCount << ": value must be 'true' or 'false'. Defaulting to 'true'.\n";
                        continue;
                    }
                } else if (splitted[0] == "version") {
                    if (splitted[1] == "true") {
                        desc.bVersion = true;
                    } else if (splitted[1] == "false") {
                        desc.bVersion = false;
                    } else {
                        error << "Line " << lineCount << ": value must be 'true' or 'false'. Defaulting to 'true'.\n";
                        continue;
                    }
                } else if (splitted[0] == "format") {
                    desc.lineFormat = splitted[1];
                    desc.hasUserTag |= desc.lineFormat.find("{$", 0) != std::string::npos;
                } else {
                    error << "Line " << lineCount << ": key is not recognized.\n";
                    continue;
                }
            }
        }
        finishDesc();
        return error.str();
	}

    void LogOutputStream::Log(LogLevel logLevel, int tick, std::thread::id threadId, const std::string& data) {
        if (HasLogLevelBit(desc.channels, logLevel)) {
            auto out = desc.lineFormat;

            std::stringstream tmp;
            tmp << threadId;

            auto t = system_clock::now();
            out = replaceTime(out, t);
            out = replaceString(out, "{{Level}}", theLogger.LogLevelToString(logLevel));
            out = replaceString(out, "{{Tick}}", std::to_string(tick));
            out = replaceString(out, "{{Message}}", data);
            out = replaceString(out, "{{ThreadId}}", tmp.str());
            os << out << std::endl;
        }
    }
    
    LogOutputFile::LogOutputFile(OutputDescriptor desc)
        : LogOutputStream{desc, file}
    {
        try {
            auto path = fs::path{desc.pathFormat};
            if (path.has_parent_path()) {
                fs::create_directories(path.remove_filename());
            }
            file.open(desc.pathFormat);
        } catch(std::exception& e) {
            std::cerr << "Could not create file: " << desc.pathFormat << std::endl << "Exception: " << e.what() << std::endl;
        }
    }

	void Logger::Log_Impl(LogLevel logLevel, std::string const& str)
	{
        auto id = std::this_thread::get_id();
        memoryLog->Log(logLevel, tick, id, str);
        for (auto& it : outputLogs) {
            it->Log(logLevel, tick, id, str);
        }
	}

    void Logger::SetUsertag_Impl(const std::string& tag, const std::string& rep)
    {
        std::vector<OutputDescriptor> tmp;
        for (auto& it : outputDescriptors) {
            it.pathFormat = replaceString(it.pathFormat, "{$" + tag + "$}", rep);
            it.lineFormat = replaceString(it.lineFormat, "{$" + tag + "$}", rep);

            it.hasUserTag = false;
            it.hasUserTag |= it.pathFormat.find("{$", 0) != std::string::npos;
            it.hasUserTag |= it.lineFormat.find("{$", 0) != std::string::npos;
            
            if (!it.hasUserTag) {
                AddOutputChannel_Impl(std::make_unique<LogOutputFile>(it));
            } else {
                tmp.push_back(it);
            }
        }
        outputDescriptors.clear();
        outputDescriptors = std::move(tmp);
    }

    void Logger::AddOutputChannel_Impl(std::unique_ptr<ILogOutput> pOutput)
    {
        memoryLog->WriteDataToOutput(*pOutput);
        outputLogs.push_back(std::move(pOutput));
    }

	void Logger::StartNextTick()
	{
		++tick;
	}

    std::string toString(LogLevel logLevel)
    {
        static DefaultToString toString;
        return toString(logLevel);
    }

	std::ostream& operator<<(std::ostream& os, LogLevel logLevel)
	{
        os << toString(logLevel);
        return os;
	}

    bool HasLogLevelBit(unsigned char bits, LogLevel logLevel) {
        return bits & (unsigned char)logLevel;
    }
}
