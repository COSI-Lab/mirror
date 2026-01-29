package org.lavajuno.lucidjson.util;

/**
 * A pair of any two types.
 * @param <A> First item's type
 * @param <B> Second item's type
 */
public class Pair<A, B> {
    /**
     * First item of this Pair
     */
    public A first;

    /**
     * Second item of this Pair
     */
    public B second;

    /**
     * Constructs a Pair.
     * @param first First item of this Pair
     * @param second Second item of this Pair
     */
    public Pair(A first, B second) { this.first = first; this.second = second; }
}
