#pragma once

// std Includes
#include <mutex>
#include <string>
#include <thread>


namespace mirror {

    /**
     * Singleton class to be used when logging in projects associated with the Clarkson Open Source Institute's Mirror
     */
    class Logger {
    public: // Functions
        // Deleted
        /**
         * Copy Constructor - Deleted
         */
        Logger(Logger &) = delete;

        /**
         * Move Constructor - Deleted
         */
        Logger(Logger &&) = delete;

        /**
         * Copy Assignment Operator - Deleted
         *
         * @return Nothing
         */
        Logger &operator=(const Logger &) = delete;

        /**
         * Move Assignment Operator - Deleted
         * @return Nothing
         */
        Logger &operator=(const Logger &&) = delete;

        // Instance Fetch Method
        /**
         * Method used to fetch a pointer to the static instance of the Logger class
         *
         * @return Pointer to the Loggers static instance
         */
        static Logger *getInstance();

        // Logging Methods

        /**
         * Prints a debug event, but does not send it
         * to the log server.
         *
         * @param logMessage Information to be logged
         */
        [[maybe_unused]] void debug(const std::string &logMessage);

        /**
         * Sends information to be logged to the log server
         *
         * @param logMessage Information to be logged
         */
        [[maybe_unused]] void info(const std::string &logMessage);

        /**
         * Sends a warning message to the log server
         *
         * @param logMessage Information to be logged
         */
        [[maybe_unused]]void warn(const std::string &logMessage);

        /**
         * Sends an error message to the log server
         *
         * @param logMessage Information to be logged
         */
        [[maybe_unused]] void error(const std::string &logMessage);

        /**
         * Sends a fatal message to the log server
         *
         * @param logMessage Information to be logged
         */
        [[maybe_unused]] void fatal(const std::string &logMessage);

        // Configuration
        /**
         * Must be called before any log messages can be sent.
         * @param componentName Name of the component that the logger is being used in
         */
        void configure(const std::string &componentName);
        
        /**
         * Destroys the Logger.
         */
        inline void close() { Logger::getInstance()->~Logger(); }



    protected: // Functions
        /**
         * Default constructor.
         */
        inline Logger() : m_Configured(false) {}

        /**
         * Destructor for the Logger class. 
         */
        inline ~Logger() {}

    private: // Functions
        /**
         * Prints a log event to stdout.
         * @param level_tag Level tag ( ex. [ DEBUG ] ) to prepend message with
         * @param message Message to log
         */
        void printEvent(const char* level_tag, const std::string &message);

    private: // Data
        /**
         * Pointer to the instance of the Logger class
         */
        static Logger *s_Instance;

        /**
         * Mutex to prevent issues related to access of the Logger instance in a multi-threaded environment
         */
        static std::mutex s_AccessMutex;

        /**
         * False until `configure()` is run.
         *
         * If false, will cause program shutdown if an attempt is made to send a line to the log server
         */
        bool m_Configured;

        /**
         * Name of the component this class is instantiated in. Included in log messages
         */
        std::string m_ComponentName;


        /**
         * Available log levels, used to increase readability in the log methods
         */
        enum class LogLevels : int {
            /**
             * LogLevels::Info
             */
            Info,
            /**
             * LogLevels::Warn
             */
            Warn,
            /**
             * LogLevels::Error
             */
            Error,
            /**
             * LogLevels::Fatal
             */
            Fatal
        };

    }; // class Logger

} // namespace mirror
