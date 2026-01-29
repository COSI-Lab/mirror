package org.lavajuno.lucidjson.error;

import java.text.ParseException;

public class JsonParseException extends ParseException {
    public JsonParseException(String text, int pos, String cause) {
        super(getMessage(text, pos, cause), pos);
    }

    private static String getMessage(String text, int pos, String cause) {
        return "At index " + pos + " of input:\n-->" +
                text.substring(pos, Math.min(pos + 12, text.length())) +
                "\n" + cause;
    }
}
