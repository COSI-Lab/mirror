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

	static {
		PUB_SOCKET.bind("tcp://*:27887");
		REP_SOCKET.bind("tcp://*:27886");
	}

	public static void main(String[] args) {
		new Thread(MirrorJsonValidator::sendUpdates).start();
		new Thread(MirrorJsonValidator::answerRequests).start();
	}

	public static void answerRequests() {
		while (!Thread.currentThread().isInterrupted()) {
			REP_SOCKET.recv(); // Block until a message is received

			if (!ValidateMirrorJson()) {
				System.out.println("Problems were found in mirrors.json, but replying to initial request anyway");
			}

			try {
				SendZMQ(REP_SOCKET);
			} catch (IOException e) {
				System.out.println("Got IOException reading mirrors.json when responding to initial request");
			}
		}
	}

	public static void sendUpdates() {
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

			if (ValidateMirrorJson()) { // Only send if valid
				try {
					SendZMQ(PUB_SOCKET);
				} catch (IOException e) {
					System.out.println("Failed to read mirrors.json from file");
				}
			}
		}
	}

	/**
	 * Validates mirrors.json against mirrors.schema.json.
	 *
	 * @return true if mirrorsJson is a valid json matching schema, false otherwise
	 */
	private static boolean ValidateMirrorJson() {

		JsonNode mirrorsJsonNode;
		try {
			mirrorsJsonNode = new JsonMapper().readTree(MIRRORS_JSON);
		} catch (JsonParseException e) {
			System.out.println("mirrors.json contains invalid json");
			return false;
		} catch (IOException e) {
			System.out.println("Failed to read mirrors.json from file");
			return false;
		}

		Set<ValidationMessage> errors = SCHEMA.validate(mirrorsJsonNode);
		if (errors.isEmpty()) {
			System.out.println("mirrors.json is valid!");
			return true;
		} else {
			System.out.println("Errors were found in mirrors.json:");
			System.out.println(errors);
			return false;
		}
	}

	/**
	 * Sends mirrors.json as string via ZMQ
	 */
	private static void SendZMQ(ZMQ.Socket socket) throws IOException {
		if (socket.getSocketType().equals(SocketType.PUB)) { PUB_SOCKET.sendMore("Config"); }
		socket.send(Files.readString(MIRRORS_JSON.toPath()));
	}
}