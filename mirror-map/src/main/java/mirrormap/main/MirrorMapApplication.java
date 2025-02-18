package mirrormap.main;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.text.ParseException;

import com.maxmind.geoip2.exception.GeoIp2Exception;
import mirrormap.geoip.GeoIPDatabase;
import mirrormap.geoip.GeoIPUpdater;
import mirrormap.io.WebsocketFrame;
import mirrormap.log.Log;
import mirrormap.websocket.WebsocketController;
import mirrormap.websocket.WebsocketServer;
import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

public class MirrorMapApplication {

    public static void main(String[] args){
        // Configure logging
        Log log = Log.getInstance();
        log.configure("mirrorlog", 4001, "Map");

        try (ZContext context = new ZContext()) {
            // Start GeoIP database updater
            log.info("Starting GeoIP database updater...");
            Thread geoIPUpdater = new Thread(new GeoIPUpdater());
            geoIPUpdater.start();

            // Connect to metrics engine
            log.info("Connecting to metrics engine...");
            ZMQ.Socket socket = context.createSocket(SocketType.SUB);
            socket.connect("tcp://mirror-metrics:8081");
            socket.subscribe("");

            // Start websocket server
            log.info("Starting websocket server...");
            WebsocketServer websocketServer = new WebsocketServer(8080);
            websocketServer.start();

            // Get GeoIP database handle
            GeoIPDatabase geoIP = GeoIPDatabase.getInstance();

            // Get websocket controller handle
            WebsocketController websocketController = WebsocketController.getInstance();

            while(true) {
                String msg = new String(socket.recv(), StandardCharsets.UTF_8);
                String[] projectIp = msg.split(":", 2);
                if(projectIp.length == 2) {
                    try {
                        StringBuilder sb = new StringBuilder();
                        double[] latlong = geoIP.getLatLong(projectIp[1]);
                        sb.append(projectIp[0]).append('\n');
                        sb.append(latlong[0]).append('\n');
                        sb.append(latlong[1]);
                        byte[] data = sb.toString().getBytes(StandardCharsets.UTF_8);
                        websocketController.broadcast(
                                new WebsocketFrame((byte) 0x1, data)
                        );
                    } catch (GeoIp2Exception ignored) {
                        log.error("Could not get location for IP " + projectIp[1] + " (GeoIp2Exception)");
                    } catch (UnknownHostException ignored) {
                        log.error("Could not get location for IP " + projectIp[1] + " (UnknownHostException)");
                    }
                }
            }
        } catch(IOException e) {
            log.fatal("IOException while initializing map backend.");
            log.debug(e.toString());
        } catch(ParseException e) {
            log.fatal("ParseException while initializing map backend.");
            log.debug(e.toString());
        }
    }
}