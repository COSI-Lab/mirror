package org.lavajuno.lucidjson;

import org.lavajuno.lucidjson.util.Index;
import org.lavajuno.lucidjson.util.StringUtils;
import org.lavajuno.lucidjson.error.JsonParseException;


/**
 * Represents a JSON string value.
 * Provides functionality for getting and setting the value.
 * Handles escaping strings automatically.
 */
@SuppressWarnings("unused")
public class JsonString extends JsonEntity {
    private String value;

    /**
     * Constructs a JsonString with a value of "".
     */
    public JsonString() { value = ""; }

    /**
     * Constructs a JsonString with the given value.
     * @param value Value for the new JsonString
     */
    public JsonString(String value) { this.value = value; }

    /**
     * Constructs a JsonString by parsing the input.
     * @param text JSON string to parse
     */
    protected JsonString(String text, Index i) throws JsonParseException {
        skipSpace(text, i);
        if(text.charAt(i.pos) != '"') {
            throw new JsonParseException(text, i.pos, "Parsing string, expected a '\"'.");
        }
        i.pos++;
        int begin = i.pos;
        while(i.pos < text.length()) {
            if(text.charAt(i.pos) == '"' && text.charAt(i.pos - 1) != '\\') {
                break;
            }
            i.pos++;
        }
        if(i.pos == text.length()) {
            throw new JsonParseException(text, i.pos, "Parsing string, reached end of input.");
        }
        value = StringUtils.unescape(text.substring(begin, i.pos));
        i.pos++;
    }

    /**
     * Gets the value of this JsonString.
     * @return Value of this JsonString
     */
    public String value() { return value; }

    /**
     * Sets the value of this JsonString
     * @param value Value of this JsonString
     */
    public void setValue(String value) { this.value = value; }

    @Override
    public String toJsonString() {
        return "\"" + StringUtils.escape(value) + "\"";
    }

    @Override
    protected String toJsonString(int indent) { return this.toJsonString(); }
}
