#include "core/util/UndoStack.h"

namespace rigkit {

void UndoStack::push(UndoRecord rec) {
	invalidateRedo();
	m_undo.push_back(std::move(rec));
	trimUndo();
	if (!m_undo.empty()) {
		m_undoLabel = m_undo.back().label;
	}
	m_redoLabel.clear();
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
}

void UndoStack::clear() {
	m_undo.clear();
	m_redo.clear();
	m_undoLabel.clear();
	m_redoLabel.clear();
}

void UndoStack::setLimit(std::size_t maxRecords) {
	m_limit = maxRecords < 1 ? 1 : maxRecords;
	trimUndo();
}

void UndoStack::trimUndo() {
	while (m_undo.size() > m_limit) {
		m_undo.erase(m_undo.begin());
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
