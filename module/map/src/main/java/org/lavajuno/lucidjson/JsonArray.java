package org.lavajuno.lucidjson;

import org.lavajuno.lucidjson.util.Index;
import org.lavajuno.lucidjson.error.JsonParseException;

import java.util.ArrayList;

/**
 * Represents a JSON array.
 * Provides functionality for accessing and modifying its values.
 */
@SuppressWarnings("unused")
public class JsonArray extends JsonEntity {
    private final ArrayList<JsonEntity> values;

    /**
     * Constructs an empty JsonArray.
     */
    public JsonArray() { values = new ArrayList<>(); }

    /**
     * Constructs a JsonArray from the given vector of elements.
     * @param values Values to initialize array with
     */
    public JsonArray(ArrayList<JsonEntity> values) { this.values = values; }

    /**
     * Constructs a JsonArray by parsing the input.
     * @param text JSON to parse
     * @param i Index of next character to parse
     * @throws JsonParseException If an error is encountered while parsing the input
     */
    protected JsonArray(String text, Index i) throws JsonParseException {
        values = parseValues(text, i);
    }

    /**
     * Deserializes a JSON array from a String.
     * @param text Input string
     * @return Deserialized JSON array
     * @throws JsonParseException if parsing fails;
     */
    public static JsonArray from(String text) throws JsonParseException {
        Index i = new Index(0);
        return new JsonArray(text, i);
    }

    /**
     * @param text JSON to parse
     * @param i Index of next character to parse
     * @return Vector created from the input
     * @throws JsonParseException If an error is encountered while parsing the input
     */
    private static ArrayList<JsonEntity> parseValues(String text, Index i) throws JsonParseException {
        ArrayList<JsonEntity> values = new ArrayList<>();
        skipSpace(text, i);
        if(text.charAt(i.pos) != '[') {
            throw new JsonParseException(text, i.pos, "Parsing array, expected a '['.");
        }
        i.pos++;
        if(i.pos >= text.length()) {
            // Handle end of input after opening {
            throw new JsonParseException(text, i.pos, "Parsing array, reached end of input.");
        }
        if(text.charAt(i.pos) == ']') {
            // Handle empty arrays
            i.pos++;
            return new ArrayList<>();
        }
        skipSpace(text, i);
        // Parse this JsonArray's values
        while(i.pos < text.length()) {
            values.add(parseEntity(text, i));
            skipSpace(text, i);
            if(text.charAt(i.pos) == ']') {
                i.pos++;
                break;
            }
            if(text.charAt(i.pos) != ',') {
                throw new JsonParseException(text , i.pos, "Parsing array, expected a ','.");
            }
            i.pos++;
        }

        return values;
    }

    /**
     * @param index Index of the target JsonEntity
     * @return JsonEntity at the given index
     * @throws IndexOutOfBoundsException If the index is larger than the size of the array
     */
    public JsonEntity get(int index) throws IndexOutOfBoundsException {
        return values.get(index);
    }

    /**
     * @param index Index of the target JsonEntity
     * @param value New value for the target JsonEntity
     * @throws IndexOutOfBoundsException If the index is larger than the size of the array
     */
    public void set(int index, JsonEntity value) throws IndexOutOfBoundsException {
        values.set(index, value);
    }

    /**
     * @param value JsonEntity to be added to this JsonArray
     */
    public void add(JsonEntity value) { values.add(value); }

    /**
     * @param index Index of the JsonEntity to remove
     * @throws IndexOutOfBoundsException If the index is larger than the size of the array
     */
    public void remove(int index) throws IndexOutOfBoundsException {
        values.remove(index);
    }

    /**
     * Clears this JsonArray
     */
    public void clear() { values.clear(); }

    /**
     * @return The number of entities contained by this JsonArray
     */
    public int size() { return values.size(); }

    /**
     * @return This JsonArray's elements
     */
    public ArrayList<JsonEntity> values() { return values; }

    @Override
    protected String toJsonString(int indent) {
        StringBuilder sb = new StringBuilder();
        String pad_elem = " ".repeat(indent + 4);
        String pad_close = " ".repeat(indent);
        sb.append("[\n");
        for(int i = 0; i < values.size(); i++) {
            sb.append(pad_elem).append(values.get(i).toJsonString(indent + 4));
            if(i < values.size() - 1) { sb.append(","); }
            sb.append("\n");
        }
        sb.append(pad_close).append("]");
        return sb.toString();
    }

    @Override
    public String toJsonString() {
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for(int i = 0; i < values.size() - 1; i++) {
            sb.append(values.get(i).toJsonString()).append(",\n");
        }
        if(!values.isEmpty()) {
            sb.append(values.get(values.size() - 1));
        }
        sb.append("]");
        return sb.toString();
    }
}
