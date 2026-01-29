package mirrormap.util;

/**
 * A mutable index that can be passed between functions and classes.
 */
public class Index {
    /**
     * Index position
     */
    public int pos;

    /**
     * Constructs an Index.
     * @param pos Initial index position
     */
    public Index(int pos) { this.pos = pos; }

    /**
     * Constructs an Index with an initial position of 0.
     */
    public Index() { this(0); }
}
