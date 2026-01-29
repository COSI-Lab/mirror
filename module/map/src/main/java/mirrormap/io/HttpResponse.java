package mirrormap.io;

import java.nio.charset.StandardCharsets;
import java.security.InvalidParameterException;
import java.util.Map;
import java.util.TreeMap;

/**
 * Stores a single HTTP 1.1 response. Can be serialized to bytes.
 */
public class HttpResponse {
    private static final String HTTP_VERSION = "HTTP/1.1 ";

    private final String status;
    private final Map<String, String> headers;

    /**
     * Constructs a HttpResponse with the given status code
     * @param status Status code (ex. "404 Not Found")
     */
    public HttpResponse(String status) {
        this.status = status;
        this.headers = new TreeMap<>();
    }

    /**
     * Sets a header in this HttpResponse to the given value.
     * Will create it if it does not already exist.
     * @param key Key of the header to set
     * @param value Value for the header
     * @throws InvalidParameterException
     */
    public void setHeader(String key, String value) throws InvalidParameterException {
        if(!key.matches("^[A-Za-z0-9_-]+$")) {
            throw new InvalidParameterException("Invalid header.");
        }
        headers.put(key, value);
    }

    /**
     * Serializes this HttpResponse to bytes to be written to an OutputStream.
     * @return This HttpResponse as bytes
     */
    public byte[] toBytes() {
        StringBuilder sb = new StringBuilder();
        sb.append(HTTP_VERSION).append(status).append("\r\n");
        for(Map.Entry<String, String> i : headers.entrySet()) {
            sb.append(i.getKey()).append(": ").append(i.getValue()).append("\r\n");
        }
        sb.append("\r\n");
        return sb.toString().getBytes(StandardCharsets.UTF_8);
    }
}
