# Known gaps

Places the Contract is silent, ambiguous, or wrong. Found by implementing a fulfillment from the prose, not imagined — the honest price of a data framework ([why-no-code.md](why-no-code.md#the-honest-price)) is that nothing fails to build when the words are unclear.

Open gaps are tracked as GitHub issues labeled [`gap`](https://github.com/rigkid/RigWorks/labels/gap), not in this file. History of how a schema got its shape lives in git blame and closed issues — this page stays short enough to read in one sitting.

## How to file a gap

Implement a host from the prose alone. Every time you cannot answer a question without reading someone else's code, that is a gap:

1. Open an issue on [rigkid/RigWorks](https://github.com/rigkid/RigWorks/issues/new) titled `[GAP] <short name>`.
2. Apply the `gap` label.
3. State what you were implementing, what the prose left silent / ambiguous / wrong, who found it (host, pack, or tool — link it if public), and a candidate resolution if you have one.
4. Close the issue when a version resolves it (do not delete it) and name the version in the closing comment.
