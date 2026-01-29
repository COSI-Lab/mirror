package org.lavajuno.lucidjson;

import org.lavajuno.lucidjson.error.JsonParseException;

/**
 * Classes can implement JsonSerializable to enable them
 * to be easily serialized/deserialized to/from JSON strings.
 * Only toJsonObject() and fromJsonObject() need to be implemented.
 */
@SuppressWarnings("unused")
public interface JsonSerializable {
    /**
     * Saves data from this instance into a new JSON object.
     * @return JSON object created from this instance
     */
    JsonObject toJsonObject();

    /**
     * Loads data from a JSON object into this instance.
     * @param o JSON object to load data from
     */
    void fromJsonObject(JsonObject o);

    /**
     * Serializes this instance to a JSON string.
     * @param pretty false to minify, true to use newlines and indents
     * @return This instance as a JSON string
     */
    default String toJsonString(boolean pretty) {
        return this.toJsonObject().toJsonString(pretty);
    }

    /**
     * Serializes this instance to a minified JSON string.
     * @return This object as a minified JSON string
     */
    default String toJsonString() {
        return this.toJsonObject().toJsonString(false);
    }

    /**
     * Deserializes a JSON string into this instance.
     * @param s JSON string to deserialize
     * @throws JsonParseException If an error is encountered while parsing the input
     */
    default void fromJsonString(String s) throws JsonParseException {
        this.fromJsonObject(JsonObject.from(s));
    }
}
