#include "core/util/UndoStack.h"

namespace rigkit {

void UndoStack::push(UndoRecord rec) {
	invalidateRedo();
	if (m_cleanDepth != kNoCleanDepth && m_cleanDepth > m_undo.size()) {
		m_cleanDepth = kNoCleanDepth; // the saved tip lived in the redo branch just dropped
	}
	m_undo.push_back(std::move(rec));
	trimUndo();
	if (!m_undo.empty()) {
		m_undoLabel = m_undo.back().label;
	}
	m_redoLabel.clear();
	m_dirty = true;
}

void UndoStack::push(std::string label, std::function<void()> undoFn,
					 std::function<void()> redoFn) {
	UndoRecord rec;
	rec.label = std::move(label);
	rec.undo = std::move(undoFn);
	rec.redo = std::move(redoFn);
	push(std::move(rec));
}

void UndoStack::undo() {
	if (m_undo.empty()) {
		return;
	}
	UndoRecord rec = std::move(m_undo.back());
	m_undo.pop_back();
	if (rec.undo) {
		rec.undo();
	}
	m_redo.push_back(std::move(rec));
	m_redoLabel = m_redo.back().label;
	m_undoLabel = m_undo.empty() ? std::string{} : m_undo.back().label;
	m_dirty = m_undo.size() != m_cleanDepth;
}

void UndoStack::redo() {
	if (m_redo.empty()) {
		return;
	}
	UndoRecord rec = std::move(m_redo.back());
	m_redo.pop_back();
	if (rec.redo) {
		rec.redo();
	}
	m_undo.push_back(std::move(rec));
	m_undoLabel = m_undo.back().label;
	m_redoLabel = m_redo.empty() ? std::string{} : m_redo.back().label;
	m_dirty = m_undo.size() != m_cleanDepth;
}

void UndoStack::clear() {
	m_undo.clear();
	m_redo.clear();
	m_undoLabel.clear();
	m_redoLabel.clear();
	m_dirty = false;
	m_cleanDepth = 0;
}

void UndoStack::markClean() {
	m_cleanDepth = m_undo.size();
	m_dirty = false;
}

void UndoStack::setLimit(std::size_t maxRecords) {
	m_limit = maxRecords < 1 ? 1 : maxRecords;
	trimUndo();
}

void UndoStack::setByteLimit(std::size_t maxBytes) {
	m_byteLimit = maxBytes;
	trimUndo();
}

void UndoStack::trimUndo() {
	auto totalBytes = [this] {
		std::size_t n = 0;
		for (const auto& rec : m_undo) {
			n += rec.bytes;
		}
		return n;
	};
	while (m_undo.size() > m_limit ||
		   (m_byteLimit > 0 && m_undo.size() > 1 && totalBytes() > m_byteLimit)) {
		m_undo.erase(m_undo.begin());
		if (m_cleanDepth != kNoCleanDepth) {
			m_cleanDepth = (m_cleanDepth == 0) ? kNoCleanDepth : m_cleanDepth - 1;
		}
	}
	if (!m_undo.empty()) {
		m_undoLabel = m_undo.back().label;
	}
}

void UndoStack::invalidateRedo() {
	m_redo.clear();
	m_redoLabel.clear();
}

} // namespace rigkit
