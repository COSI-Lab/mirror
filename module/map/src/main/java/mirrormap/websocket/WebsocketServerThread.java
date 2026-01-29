package mirrormap.websocket;
import mirrormap.io.HttpRequest;
import mirrormap.io.HttpResponse;
import mirrormap.io.WebsocketFrame;
import mirrormap.log.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.ParseException;
import java.util.Base64;

public class WebsocketServerThread extends Thread {
    private final Socket socket;
    private final Log log;
    private InputStream in;
    private OutputStream out;
    public boolean active;

    /**
     * Constructs a WebsocketServerThread.
     * @param socket Socket to communicate with client over
     */
    public WebsocketServerThread(Socket socket) {
        this.socket = socket;
        log = Log.getInstance();
        active = true;
    }

    @Override
    public void run() {
        try {
            log.info("Opening new websocket connection...");

            // Get socket i/o handles
            in = socket.getInputStream();
            out = socket.getOutputStream();

            // Listen for websocket connection upgrade request
            HttpRequest req = new HttpRequest(in);
            String ws_key = req.getHeader("Sec-WebSocket-Key");

            // Send websocket upgrade response
            HttpResponse res = new HttpResponse("101 Switching Protocols");
            res.setHeader("Connection", "Upgrade");
            res.setHeader("Upgrade", "websocket");
            res.setHeader("Sec-WebSocket-Accept", getWsAccept(ws_key));
            out.write(res.toBytes());

            // Register this session with WebsocketController once the handshake has completed
            WebsocketController.getInstance().register(this);

            log.info("Opened new websocket connection.");

            // Respond to pings while the socket is open
            while(true) {
                try {
                    WebsocketFrame ping = new WebsocketFrame(in);
                    WebsocketFrame pong = new WebsocketFrame((byte) 0xA, ping.getPayload());
                    sendFrame(pong);
                } catch(IOException e) {
                    log.debug("IOException on websocket.");
                    e.printStackTrace(System.err);
                    break;
                }
            }

            log.info("Closing websocket connection...");

            socket.close();

            log.info("Closed websocket connection.");
            active = false;

        } catch(ParseException e) {
            log.error("ParseException on HTTP request from " + socket.getInetAddress().toString());
            active = false;
        } catch(NoSuchAlgorithmException e) {
            log.error("WebsocketServerThread threw a NoSuchAlgorithmException. This should not happen!");
            active = false;
        } catch(IOException e) {
            log.error("IOException communicating with" + socket.getInetAddress().toString());
            active = false;
        }
    }

    @Override
    public void interrupt() {
        log.info("Closing session...");
        active = false;
        try {
            socket.close();
        } catch(IOException e) {
            log.error("Failed to close session.");
        }
    }

    /**
     * Sends a frame to the client this WebsocketServerThread is connected to.
     * @param f Frame to send to the client
     */
    public synchronized void sendFrame(WebsocketFrame f) throws IOException {
        out.write(f.toBytes());
    }

    /**
     * Generates the key to send back to clients when they request a connection upgrade
     * @param ws_key Key the client sent us to hash
     * @return Key to send back to the client
     * @throws NoSuchAlgorithmException If MessageDigest not support SHA-1
     */
    private String getWsAccept(String ws_key) throws NoSuchAlgorithmException {
        byte[] linked = (ws_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").getBytes(StandardCharsets.UTF_8);
        return Base64.getEncoder().encodeToString(
                MessageDigest.getInstance("SHA-1").digest(linked)
        );
    }
}
