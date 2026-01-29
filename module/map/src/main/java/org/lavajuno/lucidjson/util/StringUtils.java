package org.lavajuno.lucidjson.util;

import org.lavajuno.lucidjson.error.JsonParseException;

/**
 * StringUtils provides helper functions for working with strings
 * that are used by other classes.
 */
public class StringUtils {
    /**
     * Escapes a string.
     * Currently does not support unicode escapes (uXXXX)
     * @param s String that may contain characters that need to be escaped
     * @return Escaped string
     */
    public static String escape(String s) {
        StringBuilder sb = new StringBuilder();
        for(int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch(c) {
                case '"':
                    sb.append("\\\"");
                    break;
                case '\\':
                    sb.append("\\\\");
                    break;
                case '\b':
                    sb.append("\\b");
                    break;
                case '\f':
                    sb.append("\\f");
                    break;
                case '\n':
                    sb.append("\\n");
                    break;
                case '\r':
                    sb.append("\\r");
                    break;
                case '\t':
                    sb.append("\\t");
                    break;
                default:
                    sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * Unescapes a string.
     * Currently does not support unicode escapes (uXXXX)
     * @param s String that may contain escape sequences
     * @return Unescaped strings
     * @throws JsonParseException If an invalid escape sequence is found
     */
    public static String unescape(String s) throws JsonParseException {
        StringBuilder sb = new StringBuilder();
        for(int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if(c == '\\' && i < s.length() - 1) {
                // Escape sequence
                switch(s.charAt(i + 1)) {
                    case '"':
                        sb.append('"');
                        break;
                    case '\\':
                        sb.append('\\');
                        break;
                    case 'b':
                        sb.append('\b');
                        break;
                    case 'f':
                        sb.append('\f');
                        break;
                    case 'n':
                        sb.append('\n');
                        break;
                    case 'r':
                        sb.append('\r');
                        break;
                    case 't':
                        sb.append("\t");
                        break;
                    default:
                        throw new JsonParseException(s, i, "Invalid escape sequence");
                }
                i++;
            } else {
                // Not part of escape sequence
                sb.append(c);
            }
        }
        return sb.toString();
    }
}
