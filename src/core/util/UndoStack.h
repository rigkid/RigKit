#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace rigkit {

/**
 * @brief Command undo/redo - no UI types.
 * @details Apps and packs push records when an edit completes. `rigImGui`
 * binds Edit menu / shortcuts when an `UndoStack*` is set on `IMui`.
 */
struct UndoRecord {
	std::string label;
	std::function<void()> undo;
	std::function<void()> redo;
};

class UndoStack {
  public:
	void push(UndoRecord rec);
	void push(std::string label, std::function<void()> undoFn, std::function<void()> redoFn);

	template <typename T, typename Apply>
	void pushSnapshot(std::string label, T before, T after, Apply apply) {
		push(
			std::move(label), [before, apply] { apply(before); }, [after, apply] { apply(after); });
	}

	bool canUndo() const { return !m_undo.empty(); }
	bool canRedo() const { return !m_redo.empty(); }

	void undo();
	void redo();
	void clear();

	/** @brief Mark the current tip as the last saved state (clears dirty). */
	void markClean();
	/** @brief True when the tip differs from the last `markClean` / clear. */
	bool isDirty() const { return m_dirty; }

	const std::string& undoLabel() const { return m_undoLabel; }
	const std::string& redoLabel() const { return m_redoLabel; }

	void setLimit(std::size_t maxRecords);
	std::size_t limit() const { return m_limit; }

  private:
	void trimUndo();
	void invalidateRedo();

	std::vector<UndoRecord> m_undo;
	std::vector<UndoRecord> m_redo;
	std::string m_undoLabel;
	std::string m_redoLabel;
	std::size_t m_limit = 100;
	bool m_dirty = false;
	/// Undo stack size at last markClean. `kNoCleanDepth` when that tip is no longer reachable
	/// (its redo branch was discarded, or trimmed off the front).
	static constexpr std::size_t kNoCleanDepth = static_cast<std::size_t>(-1);
	std::size_t m_cleanDepth = 0;
};

} // namespace rigkit
