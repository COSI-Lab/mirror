package mirrormap.websocket;

import mirrormap.log.Log;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;

/**
 * WebsocketServer accepts connections and manages a pool of WebsocketServerThreads,
 * which it assigns newly opened connections to.
 */
public class WebsocketServer extends Thread {
    private final ServerSocket socket;
    private final ExecutorService threadPool;
    private final Log log;

    /**
     * Constructs a WebsocketServer.
     * @param port Port for the new WebsocketServer to listen on
     * @throws IOException If the server socket could not be created
     */
    public WebsocketServer(int port) throws IOException {
        log = Log.getInstance();
        log.info("Configuring websocket server...");
        threadPool = Executors.newCachedThreadPool();
        socket = new ServerSocket(port);
        Runtime.getRuntime().addShutdownHook(new Thread(this::interrupt));
        log.info("Configured websocket server.");
    }

    @Override
    public void run() {
        while(true) {
            try {
                threadPool.submit(new WebsocketServerThread(socket.accept()));
            } catch(IOException e) {
                log.error("IOexception on opening socket.");
                log.debug(e.toString());
            } catch(RejectedExecutionException e) {
                log.error("Failed to assign websocket session to thread pool.");
                log.debug(e.toString());
            }
        }
    }

    @Override
    public void interrupt() {
        try {
            log.info("Closing server socket...");
            socket.close();
            log.info("Closed server socket.");
        } catch(IOException e) {
            log.error("Failed to close server socket.");
            log.debug(e.toString());
        }
    }
}
