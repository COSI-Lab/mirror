package mirrormap.log;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Singleton class handling logging to the console and an external log server.
 */
public class Log {
    private static final SimpleDateFormat LOG_DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    private static final int COMPONENT_PAD = 24;
    private static final String LOG_DEBUG = " \u001B[34m[ DEBUG ]\u001B[0m ";
    private static final String LOG_INFO = " \u001B[32m[ INFO ]\u001B[0m  ";
    private static final String LOG_WARN = " \u001B[33m[ WARN ]\u001B[0m  ";
    private static final String LOG_ERROR = " \u001B[31m[ ERROR ]\u001B[0m ";
    private static final String LOG_FATAL = " \u001B[31m[ FATAL ]\u001B[0m ";

    private static Log instance = null;
    private String log_host = "";
    private int log_port = 0;
    private String component_name = "";
    Socket socket;
    OutputStream clientOut;
    boolean configured;

    /**
     * Possible levels of a log event.
     */
    public enum Levels { INFO, WARN, ERROR, FATAL, DEBUG }

    /**
     * Gets the instance of Log.
     * @return Singleton instance of Log
     */
    public static synchronized Log getInstance() {
        if(instance == null) { instance = new Log(); }
        return instance;
    }

    /**
     * Configures and connects this instance of Log to the log server
     * @param log_host Log server hostname (ex. "mirrorlog" or "localhost")
     * @param log_port Log server port (ex. 4001)
     */
    public synchronized void configure(String log_host, int log_port, String component_name) {
        try {
            this.log_host = log_host;
            this.log_port = log_port;
            this.component_name = component_name;
            socket = new Socket(log_host, log_port);
            clientOut = socket.getOutputStream();
            configured = true;
            printToConsole(Levels.INFO, "Logger - Connected to log server.");
        } catch(IOException e) {
            configured = false;
            printToConsole(Levels.ERROR, "Logger - Could not connect to log server.");
            printToConsole(Levels.WARN, "Logger - Running in offline mode.");
        }
    }

    /**
     * Constructs a Log.
     */
    private Log() { configured = false; }

    /**
     * Logs an event with severity "DEBUG".
     * Debug log events are not sent to the log server.
     * @param message Message to log
     */
    public void debug(String message) { printToConsole(Levels.DEBUG, message); }

    /**
     * Logs an event with severity "INFO".
     * @param message Message to log
     */
    public void info(String message) { print(Levels.INFO, message); }

    /**
     * Logs an event with severity "WARN".
     * @param message Message to log
     */
    public void warn(String message) { print(Levels.WARN, message); }

    /**
     * Logs an event with severity "ERROR".
     * @param message Message to log
     */
    public void error(String message) { print(Levels.ERROR, message); }

    /**
     * Logs an event with severity "FATAL".
     * @param message Message to log
     */
    public void fatal(String message) { print(Levels.FATAL, message); }

    /**
     * Logs a message to the console and the log server.
     * @param level Severity of the log event
     * @param message Message to log
     */
    private synchronized void print(Levels level, String message) {
        printToConsole(level, message);
        if(configured) {
            StringBuilder sb = new StringBuilder();
            sb.append("@").append(component_name).append("@");
            sb.append(level.ordinal()).append(message).append("\n");
            try {
                clientOut.write(sb.toString().getBytes(StandardCharsets.UTF_8));
                clientOut.flush();
            } catch (IOException e) {
                printToConsole(Levels.INFO, "Logger - Reconnecting to log server...");
                try {
                    socket = new Socket(log_host, log_port);
                    clientOut = socket.getOutputStream();
                    clientOut.write(sb.toString().getBytes(StandardCharsets.UTF_8));
                    clientOut.flush();
                } catch(IOException f) {
                    printToConsole(Levels.ERROR, "Logger - Could not reconnect to log server.");
                }
            }
        }
    }

    /**
     * Prints a log event to the console.
     * @param level Severity of the log event
     * @param message Message to log
     */
    private void printToConsole(Levels level, String message) {
        StringBuilder sb = new StringBuilder();
        sb.append(LOG_DATE_FORMAT.format(new Date()));
        switch(level.ordinal()) {
            case 1 -> sb.append(LOG_WARN);
            case 2 -> sb.append(LOG_ERROR);
            case 3 -> sb.append(LOG_FATAL);
            case 4 -> sb.append(LOG_DEBUG);
            default -> sb.append(LOG_INFO);
        }
        sb.append(component_name);
        sb.append(" ".repeat(Math.max(0, COMPONENT_PAD - component_name.length())));
        sb.append(": ").append(message);
        System.out.println(sb);
    }
}