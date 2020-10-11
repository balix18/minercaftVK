#include "logger.h"
namespace fs = std::filesystem;

namespace ezpz {
    std::string replaceString(std::string subject, const std::string& search, const std::string& replace) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
             subject.replace(pos, search.length(), replace);
             pos += replace.length();
        }
        return subject;
    }

    std::string replaceTime(std::string subject, std::tm* t) {
        auto year = std::to_string(t->tm_year + 1900);
        auto month = std::to_string(t->tm_mon + 1);
        auto day = std::to_string(t->tm_mday);
        auto hour = std::to_string(t->tm_hour);
        auto minute = std::to_string(t->tm_min);
        auto second = std::to_string(t->tm_sec);

        subject = replaceString(subject, "{{Year}}", year);
        subject = replaceString(subject, "{{Month}}", month);
        subject = replaceString(subject, "{{Day}}", day);
        subject = replaceString(subject, "{{Hour}}", hour);
        subject = replaceString(subject, "{{Minute}}", minute);
        subject = replaceString(subject, "{{Second}}", second);

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

    std::string Logger::Init(const std::string& path)
	{
        memoryLog = std::make_unique<LogOutputMemory>(); 

        std::ifstream file{path};

        auto tt = std::time(nullptr);
        auto t = std::localtime(&tt);
        startTime = t;

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
                desc.hasUserTag = desc.pathFormat.find("{{Usertag}}", 0) != std::string::npos;
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
                        case 'I':
                            desc.channels |= LogLevel::INFO;
                            break;
                        case 'E':
                            desc.channels |= LogLevel::ERROR;
                            break;
                        case 'S':
                            desc.channels |= LogLevel::SERVER;
                            break;
                        case 'D':
                            desc.channels |= LogLevel::DEBUG;
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
                    desc.hasUserTag |= desc.lineFormat.find("{{Usertag}}", 0) != std::string::npos;
                } else {
                    error << "Line " << lineCount << ": key is not recognized.\n";
                    continue;
                }
            }
        }
        finishDesc();
        return error.str();
	}

    void LogOutputStream::Log(LogLevel logLevel, int tick, const std::string& data) {
        if (HasLogLevelBit(desc.channels, logLevel)) {
            auto out = desc.lineFormat;

            auto tt = std::time(nullptr);
            auto t = std::localtime(&tt);
            out = replaceTime(out, t);
            out = replaceString(out, "{{Level}}", toString(logLevel));
            out = replaceString(out, "{{Tick}}", std::to_string(tick));
            out = replaceString(out, "{{Message}}", data);
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
        memoryLog->Log(logLevel, tick, str);
        for (auto& it : outputLogs) {
            it->Log(logLevel, tick, str);
        }
	}

    void Logger::SetUsertag(std::string tag)
    {
        usertag = std::move(tag);

        for (auto& it : outputDescriptors) {
            it.pathFormat = replaceString(it.pathFormat, "{{Usertag}}", usertag);
            it.lineFormat = replaceString(it.lineFormat, "{{Usertag}}", usertag);
            AddOutputChannel(std::make_unique<LogOutputFile>(it));
        }
        outputDescriptors.clear();
    }

    void Logger::AddOutputChannel(std::unique_ptr<ILogOutput> pOutput)
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
		switch (logLevel)
		{
		case LogLevel::DEBUG:
			return "DEBUG";
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::ERROR:
			return "ERROR";
        case LogLevel::SERVER:
            return "SERVER";
		default:
			return "UNKNOWN";
		}
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
