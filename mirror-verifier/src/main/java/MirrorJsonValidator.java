import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.json.JsonMapper;
import com.networknt.schema.*;

import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.util.Set;
import java.util.concurrent.TimeUnit;

public class MirrorJsonValidator {

	private static final Path CONFIG_PATH = Paths.get("configs");
	private static final JsonSchema SCHEMA = JsonSchemaFactory.getInstance(SpecVersion.VersionFlag.V7)
			.getSchema(new File(CONFIG_PATH + "/mirrors.schema.json").getAbsoluteFile().toURI());
	private static final File MIRRORS_JSON = new File(CONFIG_PATH + "/mirrors.json");
	private static final ZContext CONTEXT = new ZContext();
	private static final ZMQ.Socket PUB_SOCKET = CONTEXT.createSocket(SocketType.PUB);
	private static final ZMQ.Socket REP_SOCKET = CONTEXT.createSocket(SocketType.REP);
	private static String LAST_VALID;

	static {
		PUB_SOCKET.bind("tcp://*:27887");
		REP_SOCKET.bind("tcp://*:27886");
	}

	public static void main(String[] args) {
		ValidateMirrorJson();
		new Thread(MirrorJsonValidator::sendUpdates).start();
		new Thread(MirrorJsonValidator::answerRequests).start();
	}

	private static void answerRequests() {
		while (!Thread.currentThread().isInterrupted()) {
			REP_SOCKET.recv(); // Block until a message is received

			if (LAST_VALID != null) SendZMQ(REP_SOCKET);
			else {
				System.out.println("Got request for mirrors.json, but file was missing or invalid on startup and has not been fixed");
				REP_SOCKET.send("{\"error\":true}");
			}
		}
	}

	private static void sendUpdates() {
		WatchService watcher;
		try {
			watcher = FileSystems.getDefault().newWatchService();
			CONFIG_PATH.register(watcher, StandardWatchEventKinds.ENTRY_MODIFY);
		} catch (IOException e) {
			throw new RuntimeException(e);
		}

		WatchKey key;
		while(!Thread.currentThread().isInterrupted()) {
			try {
				key = watcher.take(); // Wait for file to be changed
				TimeUnit.MILLISECONDS.sleep(50); // Prevents duplicate events (from updating metadata)
			} catch (InterruptedException e) {
				throw new RuntimeException(e);
			}

			key.pollEvents();
			key.reset();

			if (ValidateMirrorJson()) SendZMQ(PUB_SOCKET);
		}
	}

	/**
	 * Validates mirrors.json against mirrors.schema.json, and updates LAST_VALID if it is valid
	 * @return true if mirrorsJson is a valid json matching schema, false otherwise
	 */
	private static boolean ValidateMirrorJson() {

		JsonNode mirrorsJsonNode;
		String mirrorsJson;
		try {
			mirrorsJson = Files.readString(MIRRORS_JSON.toPath());
			mirrorsJsonNode = new JsonMapper().readTree(mirrorsJson);
		} catch (JsonParseException e) {
			System.out.println("mirrors.json contains invalid json");
			return false;
		} catch (IOException e) {
			System.out.println("Failed to read mirrors.json from file");
			return false;
		}

		if (LAST_VALID != null && LAST_VALID.equals(mirrorsJson)) return false;

		Set<ValidationMessage> errors = SCHEMA.validate(mirrorsJsonNode);
		if (errors.isEmpty()) {
			System.out.println("mirrors.json is valid!");
			LAST_VALID = mirrorsJson;
			return true;
		} else {
			System.out.println("Errors were found in mirrors.json:");
			System.out.println(errors);
			return false;
		}
	}

	/**
	 * Sends mirrors.json as string via ZMQ
	 * @param socket socket to send on
	 */
	private static void SendZMQ(ZMQ.Socket socket) {
		if (socket.getSocketType().equals(SocketType.PUB)) socket.sendMore("Config");
		socket.send(LAST_VALID);
	}
}