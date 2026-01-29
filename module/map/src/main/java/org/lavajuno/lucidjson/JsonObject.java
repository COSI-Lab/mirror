package org.lavajuno.lucidjson;

import org.lavajuno.lucidjson.util.Index;
import org.lavajuno.lucidjson.util.Pair;
import org.lavajuno.lucidjson.error.JsonParseException;

import java.util.TreeMap;
import java.util.Collection;
import java.util.Set;

/**
 * Represents a JSON object.
 * Provides functionality for accessing and modifying its values.
 */
@SuppressWarnings("unused")
public class JsonObject extends JsonEntity {
    private final TreeMap<String, JsonEntity> values;

    /**
     * Constructs an empty JsonObject.
     */
    public JsonObject() { values = new TreeMap<>(); }

    /**
     * Constructs a JsonObject from the given map.
     * @param values Values to initialize map with
     */
    public JsonObject(TreeMap<String, JsonEntity> values) { this.values = values; }

    /**
     * Constructs a JsonObject by parsing the input.
     * @param text JSON to parse
     * @param i Index of next character to parse
     * @throws JsonParseException If an error is encountered while parsing the input
     */
    protected JsonObject(String text, Index i) throws JsonParseException {
        values = parseValues(text, i);
    }

    /**
     * Deserializes a JSON object from a String.
     * @param text Input string
     * @return Deserialized JSON object
     * @throws JsonParseException if parsing fails;
     */
    public static JsonObject from(String text) throws JsonParseException {
        Index i = new Index(0);
        return new JsonObject(text, i);
    }

    /**
     * @param text JSON to parse
     * @param i Index of next character to parse
     * @return Key-value map created from the input
     * @throws JsonParseException If an error is encountered while parsing the input
     */
    private static TreeMap<String, JsonEntity> parseValues(String text, Index i) throws JsonParseException {
        TreeMap<String, JsonEntity> values = new TreeMap<>();
        skipSpace(text, i);
        if(text.charAt(i.pos) != '{') {
            throw new JsonParseException(text, i.pos, "Parsing object, expected a '{'.");
        }
        i.pos++;
        if(i.pos >= text.length()) {
            // Handle end of input after opening {
            throw new JsonParseException(text, i.pos, "Parsing object, reached end of input.");
        }
        if(text.charAt(i.pos) == '}') {
            // Handle empty objects
            i.pos++;
            return new TreeMap<>();
        }
        skipSpace(text, i);
        // Parse this JsonObject's values
        while(i.pos < text.length()) {
            Pair<String, JsonEntity> p = parsePair(text, i);
            values.put(p.first, p.second);
            skipSpace(text, i);
            if(text.charAt(i.pos) == '}') {
                // Object close
                i.pos++;
                break;
            }
            if(text.charAt(i.pos) != ',') {
                // Not the last item, but no comma
                throw new JsonParseException(text , i.pos, "Parsing object, expected a ','.");
            }
            i.pos++;
        }

        return values;
    }

    /**
     * Gets a JsonEntity with a given key.
     * @param key Key of the target JsonEntity
     * @return JsonEntity with the specified key, or null if it does not exist
     */
    public JsonEntity get(String key) { return values.get(key); }

    /**
     * Puts a JsonEntity under a given key. Will overwrite the previous
     * entity if it already exists.
     * @param key Key of the target JsonEntity
     * @param value New value for the target JsonEntity
     */
    public void put(String key, JsonEntity value) { values.put(key, value); }

    /**
     * Removes the JsonEntity at the specified key.
     * @param key Key of the JsonEntity to remove
     */
    public void remove(String key) { values.remove(key); }

    /**
     * Clears this JsonObject's map.
     */
    public void clear() { values.clear(); }

    /**
     * Gets a collection of all the keys in this JsonObject
     * @return This JsonObject's keys
     */
    public Collection<String> keys() { return values.keySet(); }

    /**
     * Gets a collection of all the values in this JsonObject
     * @return This JsonObject's values
     */
    public Collection<JsonEntity> values() { return values.values(); }

    /**
     * Gets ths size of this JsonObject
     * @return The number of entities contained by this JsonObject
     */
    public int size() { return values.size(); }

    @Override
    protected String toJsonString(int indent) {
        StringBuilder sb = new StringBuilder();
        String pad_elem = " ".repeat(indent + 4);
        String pad_close = " ".repeat(indent);
        sb.append("{\n");
        Set<String> keys = values.keySet();
        int i = 0;
        for(String j : keys) {
            i++;
            sb.append(pad_elem).append("\"").append(j).append("\": ");
            sb.append(values.get(j).toJsonString(indent + 4));
            if(i < keys.size()) { sb.append(","); }
            sb.append("\n");
        }
        sb.append(pad_close).append("}");
        return sb.toString();
    }

    @Override
    public String toJsonString() {
        StringBuilder sb = new StringBuilder();
        sb.append("{");
        Set<String> keys = values.keySet();
        int i = 0;
        for(String j : keys) {
            i++;
            sb.append("\"").append(j).append("\":").append(values.get(j).toJsonString());
            if(i < keys.size()) { sb.append(","); }
        }
        sb.append("}");
        return sb.toString();
    }
}
