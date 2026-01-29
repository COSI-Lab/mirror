package org.lavajuno.lucidjson.util;

/**
 * Stores the index of the next character to be parsed.
 * Passed between JsonEntities as the input is parsed
 * and they are constructed.
 */
public class Index {
    /**
     * Integer index of the next character be parsed.
     */
    public int pos;

    /**
     * Constructs an Index.
     * @param pos Initial character pointed to by this Index.
     */
    public Index(int pos) { this.pos = pos; }
}
