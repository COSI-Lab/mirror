// Header Being Defined
#include <mirror/logger.hpp>

// std includes
#include <mutex>
#include <string>
#include <thread>
#include <iomanip>
#include <iostream>

namespace mirror {

    // Static Member Initializations
    Logger *Logger::s_Instance = nullptr;
    std::mutex Logger::s_AccessMutex;

    /*
     * Start Of Public Functions
     */

    Logger *Logger::getInstance() {
        std::lock_guard<std::mutex> instanceLock(s_AccessMutex);

        // If no instance exists, one is created
        if (s_Instance == nullptr) {
            s_Instance = new Logger();
        }

        return s_Instance;
    }

    [[maybe_unused]] void Logger::debug(const std::string &logMessage) {
        printEvent(" \u001B[34m[ DEBUG ]\u001B[0m ", logMessage);
    }

    [[maybe_unused]] void Logger::info(const std::string &logMessage) {
        std::string lineToSend = "@" + std::to_string((int) LogLevels::Info) + logMessage;
        f_SendLine(lineToSend);
        printEvent(" \u001B[32m[ INFO ]\u001B[0m  ", logMessage);
    }

    [[maybe_unused]] void Logger::warn(const std::string &logMessage) {
        std::string lineToSend = "@" + std::to_string((int) LogLevels::Warn) + logMessage;
        f_SendLine(lineToSend);
        printEvent(" \u001B[33m[ WARN ]\u001B[0m  ", logMessage);
    }

    [[maybe_unused]] void Logger::error(const std::string &logMessage) {
        std::string lineToSend = "@" + std::to_string((int) LogLevels::Error) + logMessage;
        f_SendLine(lineToSend);
        printEvent(" \u001B[31m[ ERROR ]\u001B[0m ", logMessage);
    }

    [[maybe_unused]] void Logger::fatal(const std::string &logMessage) {
        std::string lineToSend = "@" + std::to_string((int) LogLevels::Fatal) + logMessage;
        f_SendLine(lineToSend);
        printEvent(" \u001B[31m[ FATAL ]\u001B[0m ", logMessage);
    }

    void Logger::configure(const std::string &componentName) {
        std::lock_guard<std::mutex> instanceLock(s_AccessMutex);

        m_ComponentName = componentName;
        m_Configured = true;
    }

    /*
     * Start Of Private Functions
     */

    void Logger::printEvent(const char* level_tag, const std::string &message) {
        std::time_t now = std::time(nullptr);
        auto time = *std::localtime(&now);
        std::cout << std::put_time(&time, "%Y-%m-%d %H:%M:%S");
        std::cout << level_tag << std::left << std::setw(20);
        std::cout << m_ComponentName << " : " << message << std::endl;
    }
} // namespace mirror