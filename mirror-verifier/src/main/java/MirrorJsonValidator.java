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

	private static final JsonSchema SCHEMA = JsonSchemaFactory.getInstance(SpecVersion.VersionFlag.V7)
			.getSchema(new File("config/mirrors.schema.json").getAbsoluteFile().toURI());
	private static final Path CONFIG_PATH = Paths.get("config");
	private static final File MIRRORS_JSON = new File(CONFIG_PATH + "/mirrors.json");

	public static void main(String[] args) {

		WatchService watcher;
		try {
			watcher = FileSystems.getDefault().newWatchService();
			CONFIG_PATH.register(watcher, StandardWatchEventKinds.ENTRY_MODIFY);
		} catch (IOException e) {
			throw new RuntimeException(e);
		}

		WatchKey key;
		while(true) {
			try {
				key = watcher.take(); // Wait for file to be changed
			} catch (InterruptedException e) {
				throw new RuntimeException(e);
			}

			try {
				TimeUnit.MILLISECONDS.sleep(50); // Prevents duplicate events (from updating metadata)
			} catch (InterruptedException e) {
				throw new RuntimeException(e);
			}
			key.pollEvents();
			key.reset();

			if (ValidateMirrorJson()) { // Only send if valid
				try {
					SendZMQ(Files.readString(MIRRORS_JSON.toPath()));
				} catch (IOException e) {
					System.out.println("IOException reading mirrors.json");
					throw new RuntimeException(e);
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

	private static final ZMQ.Socket SOCKET = new ZContext().createSocket(SocketType.PUB);
	static {
		SOCKET.bind("tcp://*:27887");
	}

	/**
	 * Sends new mirrors.json as string via ZMQ, topic "Config", port 27887
	 */
	private static void SendZMQ(String mirrorsJson) {
		SOCKET.sendMore("Config");
		SOCKET.send(mirrorsJson.getBytes(ZMQ.CHARSET));
	}
}