package mirrormap.io;
import mirrormap.util.Index;
import mirrormap.util.Pair;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.text.ParseException;
import java.util.Map;
import java.util.Scanner;
import java.util.TreeMap;

/**
 * Stores a single HTTP request. Constructed with an InputStream.
 */
public class HttpRequest {
    private final String request;
    private final Map<String, String> headers;

    /**
     * Constructs a HttpRequest by listening on the provided InputStream
     * and parsing the request.
     * @param in InputStream (usually a socket's input stream) to listen on
     * @throws ParseException If an error is encountered while parsing the request
     */
    public HttpRequest(InputStream in) throws ParseException {
        Scanner scanner = new Scanner(in, StandardCharsets.UTF_8);
        String raw_request = scanner.useDelimiter("\\r\\n\\r\\n").next();
        Index i = new Index();
        request = parseLine(raw_request, i);
        headers = new TreeMap<>();
        while(i.pos < raw_request.length()) {
            Pair<String, String> header = parseHeader(raw_request, i);
            headers.put(header.first, header.second);
        }
    }

    /**
     * Gets the header with the specified key.
     * @param key Key of the header to get
     * @return Header with the specified key, or null if it does not exist
     */
    public String getHeader(String key) { return headers.get(key); }

    /**
     * Gets the request line of this HttpRequest (ex. "GET / HTTP/1.1")
     * @return Request line of this HttpRequest
     */
    public String getRequest() { return request; }

    /**
     * Parses a single HTTP header.
     * @param s HTTP request as a String
     * @param i Index object
     * @return Single header as a key-value pair
     * @throws ParseException If an error is encountered while parsing the request
     */
    private static Pair<String, String> parseHeader(String s, Index i) throws ParseException {
        StringBuilder key = new StringBuilder();
        StringBuilder value = new StringBuilder();

        // Read key
        for(; i.pos < s.length(); i.pos++) {
            if(isDelimiter(s, i.pos)) {
                i.pos += 2;
                break;
            } else if(isLineBreak(s, i.pos)) {
                throw new ParseException("Parsing HTTP header - encountered line break", i.pos);
            }

            if(!isKeyCharacter(s.charAt(i.pos))) {
                throw new ParseException("Parsing HTTP header - invalid character in key.", i.pos);
            }
            key.append(s.charAt(i.pos));
        }

        // Read value
        for(; i.pos < s.length(); i.pos++) {
            if(isLineBreak(s, i.pos)) {
                i.pos += 2;
                break;
            }
            value.append(s.charAt(i.pos));

        }

        return new Pair<>(key.toString(), value.toString());
    }

    /**
     * Parses a single line from an HTTP request.
     * @param s HTTP request as a String
     * @param i Index object
     * @return Single line as a String
     * @throws ParseException If an error is encountered while parsing the request
     */
    private static String parseLine(String s, Index i) throws ParseException {
        StringBuilder sb = new StringBuilder();
        for(; i.pos < s.length(); i.pos++) {
            if(isLineBreak(s, i.pos)) {
                i.pos += 2;
                return sb.toString();
            }
            sb.append(s.charAt(i.pos));
        }
        throw new ParseException("Parsing HTTP request - reached end of input", i.pos);
    }

    /**
     * @param c Character to check
     * @return True if the character is a valid character for a HTTP header key
     */
    private static boolean isKeyCharacter(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    /**
     * @param s HTTP request as a String
     * @param i Index object
     * @return True if the index points to a delimiter
     */
    private static boolean isDelimiter(String s, int i) {
        if(i >= s.length() - 1) { return false; }
        return s.charAt(i) == ':' && s.charAt(i + 1) == ' ';
    }

    /**
     * @param s HTTP request as a String
     * @param i Index object
     * @return True if the index points to a line break
     */
    private static boolean isLineBreak(String s, int i) {
        if(i >= s.length() - 1) { return false; }
        return s.charAt(i) == '\r' && s.charAt(i + 1) == '\n';
    }
}
