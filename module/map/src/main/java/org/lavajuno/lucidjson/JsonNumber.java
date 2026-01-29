package org.lavajuno.lucidjson;

import org.lavajuno.lucidjson.util.Index;
import org.lavajuno.lucidjson.error.JsonParseException;

/**
 * Represents a JSON number value.
 * Provides functionality for getting and setting the value as an int, long, float, or double.
 */
@SuppressWarnings("unused")
public class JsonNumber extends JsonEntity {
    private String value;

    /**
     * Constructs a JsonNumber from an int.
     * @param value Value of this JsonNumber
     */
    public JsonNumber(int value) { this.value = Integer.toString(value); }

    /**
     * Constructs a JsonNumber from a long.
     * @param value Value of this JsonNumber
     */
    public JsonNumber(long value) { this.value = Long.toString(value); }

    /**
     * Constructs a JsonNumber from a float.
     * @param value Value of this JsonNumber
     */
    public JsonNumber(float value) { this.value = Float.toString(value); }

    /**
     * Constructs a JsonNumber from a double.
     * @param value Value of this JsonNumber
     */
    public JsonNumber(double value) { this.value = Double.toString(value); }

    /**
     * Constructs a JsonNumber from the input.
     * @param text JSON number
     * @param i Index of next character to parse
     */
    protected JsonNumber(String text, Index i) throws JsonParseException {
        int begin = i.pos;
        while(i.pos < text.length()) {
            if(!isNumber(text.charAt(i.pos))) {
                break;
            }
            i.pos++;
        }
        if(i.pos == text.length()) {
            throw new JsonParseException(text, i.pos, "Parsing number, reached end of input.");
        }
        this.value = text.substring(begin, i.pos);
    }

    /**
     * Gets the value of this JsonNumber.
     * @return Value of this JsonNumber as an int
     * @throws NumberFormatException If this JsonNumber cannot be parsed as an int
     */
    public int toInt() throws NumberFormatException { return Integer.parseInt(value); }

    /**
     * Gets the value of this JsonNumber.
     * @return Value of this JsonNumber as a long
     * @throws NumberFormatException If this JsonNumber cannot be parsed as a long
     */
    public long toLong() throws NumberFormatException { return Long.parseLong(value); }

    /**
     * Gets the value of this JsonNumber.
     * @return Value of this JsonNumber as a float
     * @throws NumberFormatException If this JsonNumber cannot be parsed as a float
     */
    public float toFloat() throws NumberFormatException { return Float.parseFloat(value); }

    /**
     * Gets the value of this JsonNumber.
     * @return Value of this JsonNumber as a double
     * @throws NumberFormatException If this JsonNumber cannot be parsed as a double
     */
    public double toDouble() throws NumberFormatException { return Double.parseDouble(value); }

    /**
     * Sets the value of this JsonNumber.
     * @param value Int value of this JsonNumber
     */
    public void setValue(int value) { this.value = Integer.toString(value); }

    /**
     * Sets the value of this JsonNumber.
     * @param value Long value of this JsonNumber
     */
    public void setValue(long value) { this.value = Long.toString(value); }

    /**
     * Sets the value of this JsonNumber.
     * @param value Float value of this JsonNumber
     */
    public void setValue(float value) { this.value = Float.toString(value); }

    /**
     * Sets the value of this JsonNumber.
     * @param value Double value of this JsonNumber
     */
    public void setValue(double value) { this.value = Double.toString(value); }

    /**
     * Returns true if the character is part of a valid JSON number
     * @param c Character to check
     * @return True if the character is part of a valid JSON number
     */
    private static boolean isNumber(char c) {
        return switch(c) {
            case '0', '1', '2', '3', '4', '5', '6',
                    '7', '8', '9', '.', '-', 'e', 'E' -> true;
            default -> false;
        };
    }

    @Override
    public String toJsonString() { return value; }

    @Override
    protected String toJsonString(int indent) { return this.toJsonString(); }
}
