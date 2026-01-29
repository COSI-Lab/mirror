package mirrormap.websocket;

import mirrormap.io.WebsocketFrame;
import mirrormap.log.Log;

import java.io.IOException;
import java.util.Vector;

/**
 * WebsocketController stores a list of active connections and provides
 * functionality to broadcast frames to every connected client.
 * WebsocketServerThreads will register themselves with the WebsocketController
 * upon connection, and deregister themselves when they disconnect or if an exception
 * is thrown.
 */
public class WebsocketController {
    private final Vector<WebsocketServerThread> connections;
    private static WebsocketController instance = null;
    private final Log log;

    /**
     * Private constructor for WebsocketController.
     */
    private WebsocketController() {
        connections = new Vector<>();
        log = Log.getInstance();
    }

    /**
     * Gets the instance of WebsocketController, creating it if it does not already exist.
     * @return WebsocketController instance
     */
    public static synchronized WebsocketController getInstance() {
        if(instance == null) { instance = new WebsocketController(); }
        return instance;
    }

    /**
     * Broadcasts a WebsocketFrame to every currently connected client.
     * If a client has disconnected, it will be removed from the list of active connections.
     * @param f WebsocketFrame to broadcast
     */
    public synchronized void broadcast(WebsocketFrame f) {
        for(int i = 0; i < connections.size(); i++) {
            if(!connections.get(i).active) {
                connections.remove(i);
                i--;
            } else {
                try {
                    connections.get(i).sendFrame(f);
                } catch(IOException ignored) {
                    log.warn("Failed to broadcast frame.");
                }
            }
        }
    }

    /**
     * Registers a WebsocketServerThread with this WebsocketController.
     * @param session WebsocketServerThread instance to register (usually "this")
     */
    public void register(WebsocketServerThread session) { connections.add(session); }
}
