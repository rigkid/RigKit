#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/json.h"
#include "ecs/MEcs.h"

struct ImFontAtlas;

// Forward declarations
namespace rigkit {
class RigKitEngine;
class MWindow;
class Progress;
class UndoStack;
} // namespace rigkit

namespace rigkit {

// Notification types
enum class NotificationType { Info, Success, Warning, Error };

/**
 * @brief Host seam for Rig + UI (no UI toolkit in core).
 * @details Surfaces edit ECS/schemas through this seam. Default UI pack is
 * rigImGui. Show/headless/embedded hosts omit UI and remain Rig-compliant.
 * @see docs/contract/ui.md
 * @see https://github.com/rigkid/RigWorks/blob/main/docs/ui.md
 */
class IMui {
  public:
	using FileDialogCallback = std::function<void(const std::string& path)>;

	virtual ~IMui() = default;

	// Core UI hooks
	virtual void init() = 0;
	virtual void shutdown() = 0;
	virtual void handleInput() = 0;
	virtual void render() = 0;
	virtual void setRigKitEngine(RigKitEngine* engine) = 0;

	/**
	 * @brief Present / chrome id for this fulfillment (e.g. "imgui", "tui").
	 * @details Host may swap fulfillments via RigKitEngine::requestUiChrome.
	 * Not an ImGuiStyle theme - a compositor / toolkit choice.
	 */
	virtual const char* chromeId() const { return "imgui"; }

	// Engine access
	virtual RigKitEngine* getRigKitEngine() const = 0;

	// Window visibility management
	virtual void setWindowVisibility(const std::string& windowName, bool visible) = 0;
	virtual void setWindowVisibilityAll(bool visible) = 0;
	virtual bool getWindowVisibility(const std::string& windowName) const = 0;
	virtual std::vector<std::string> getAllWindowNames() const = 0;

	// Window management
	virtual class MWindow* getWindowManager() = 0;
	virtual const class MWindow* getWindowManager() const = 0;

	/**
	 * @brief When true, dock central node shows the GL framebuffer (plot bed).
	 * @details Default off so non-plot apps keep a solid dock fill. rigImGui
	 * maps this to ImGuiDockNodeFlags_PassthruCentralNode.
	 */
	virtual void setDockPassthroughCentral(bool enabled) { (void)enabled; }
	virtual bool dockPassthroughCentral() const { return false; }

	/**
	 * @brief Window-client rect of the empty central dock (GL bed) when valid.
	 * @details Origin is the window client top-left (same space as GLFW cursor).
	 * One-frame lag is normal: dock layout updates during UI render after Draw.
	 * @return false if unavailable (caller falls back to full window).
	 */
	virtual bool centralViewRect(float& outX, float& outY, float& outW, float& outH) const {
		(void)outX;
		(void)outY;
		(void)outW;
		(void)outH;
		return false;
	}

	/**
	 * @brief Bind app/document undo history for Edit menu and shortcuts.
	 * @details Stack lives outside UI (app or pack). Null clears the bind.
	 */
	virtual void setUndoStack(UndoStack* stack) { (void)stack; }
	virtual UndoStack* undoStack() const { return nullptr; }

	/**
	 * @brief Enter/leave Edit Mode (author panels visible when ON).
	 * @details Transient UI state. Whether the feature exists at all is the
	 * engine's call - RigKitEngine::enableEditMode(bool), off by default - so
	 * this is a no-op for apps that never opted in.
	 */
	virtual void setEditMode(bool enabled) { (void)enabled; }
	virtual bool editMode() const { return false; }

	/**
	 * @brief Request an open-file path (async; callback on confirm).
	 * @param filters Extension filters for the fulfillment (e.g. ".rig", ".svg").
	 *        Fulfillments always also offer `.*` (all files) in the type combo.
	 */
	virtual void openFileDialog(const std::string& title, std::vector<std::string> filters,
								FileDialogCallback onSelected) {
		(void)title;
		(void)filters;
		(void)onSelected;
	}

	/** @brief Request a save-file path (async; callback on confirm). */
	virtual void saveFileDialog(const std::string& title, std::vector<std::string> filters,
								FileDialogCallback onSelected) {
		(void)title;
		(void)filters;
		(void)onSelected;
	}

	/**
	 * @brief Register a File menu row above Export (e.g. New / Open / Save).
	 * @param label Menu label (e.g. "Open...").
	 * @param action Invoked when the item is chosen.
	 * @param shortcut Optional chord shown in the menu (display only unless the
	 *        pack also binds it on the fulfillment shortcut manager).
	 */
	virtual void registerFileAction(const std::string& label, std::function<void()> action,
									const std::string& shortcut = {}) {
		(void)label;
		(void)action;
		(void)shortcut;
	}

	/**
	 * @brief Register a File menu submenu (e.g. Open to Resource to ...).
	 * @details @p drawContents runs inside an open `BeginMenu` - emit `MenuItem`s /
	 * nested menus. Drawn each frame the parent File menu is open so lists can be live.
	 */
	virtual void registerFileSubmenu(const std::string& label, std::function<void()> drawContents) {
		(void)label;
		(void)drawContents;
	}

	/**
	 * @brief Register an app-menu row (product actions; Preferences / Quit stay host-owned).
	 * @param label Menu label (e.g. "Import Options...").
	 * @param action Invoked when the item is chosen.
	 * @param shortcut Optional chord shown in the menu (display only unless also bound).
	 */
	virtual void registerAppAction(const std::string& label, std::function<void()> action,
								   const std::string& shortcut = {}) {
		(void)label;
		(void)action;
		(void)shortcut;
	}

	/**
	 * @brief Register an app-menu submenu (e.g. Import to Resource...).
	 * @details @p drawContents runs inside an open `BeginMenu` - emit `MenuItem`s /
	 * nested menus. Drawn each frame the parent app menu is open so lists can be live.
	 */
	virtual void registerAppSubmenu(const std::string& label, std::function<void()> drawContents) {
		(void)label;
		(void)drawContents;
	}

	/**
	 * @brief Record a successfully opened/saved document path for File to Open Recent.
	 * @details Newest first; duplicates move to the front. No-op when path is empty.
	 */
	virtual void noteRecentFile(const std::string& path) { (void)path; }

	/** @brief Clear the Open Recent list. */
	virtual void clearRecentFiles() {}

	/**
	 * @brief Handler invoked when the user picks a File to Open Recent entry.
	 * @details Apps/packs that own document load register here (e.g. load .rig).
	 */
	virtual void setRecentFileOpenHandler(std::function<void(const std::string& path)> handler) {
		(void)handler;
	}

	/**
	 * @brief Register a File to Export menu row (packs add formats; host owns PNG).
	 * @param label Menu label (e.g. "SVG...").
	 * @param action Invoked when the item is chosen.
	 */
	virtual void registerExportAction(const std::string& label, std::function<void()> action) {
		(void)label;
		(void)action;
	}

	/**
	 * @brief Register a Tools menu row (packs replace default gizmo entries when non-empty).
	 * @param id Stable id (e.g. "tool.selection").
	 * @param label Menu label.
	 * @param shortcut Optional chord shown in the menu (display only unless also bound).
	 * @param isActive Optional; when set, menu item shows a check mark.
	 * @param action Invoked when the item is chosen.
	 */
	virtual void registerToolAction(const std::string& id, const std::string& label,
									const std::string& shortcut, std::function<bool()> isActive,
									std::function<void()> action) {
		(void)id;
		(void)label;
		(void)shortcut;
		(void)isActive;
		(void)action;
	}

	/**
	 * @brief Register an Edit menu row after Undo/Redo (e.g. Delete).
	 * @param label Menu label (e.g. "Delete").
	 * @param shortcut Optional chord shown in the menu (display only unless also bound).
	 * @param isEnabled Optional; when set, disables the item when false.
	 * @param action Invoked when the item is chosen.
	 */
	virtual void registerEditAction(const std::string& label, const std::string& shortcut,
									std::function<bool()> isEnabled, std::function<void()> action) {
		(void)label;
		(void)shortcut;
		(void)isEnabled;
		(void)action;
	}

	/**
	 * @brief Register an Edit menu submenu (e.g. Rotate to 90 / 180).
	 * @details @p drawContents runs inside an open `BeginMenu`. @p isEnabled
	 * disables the submenu when set and false.
	 */
	virtual void registerEditSubmenu(const std::string& label, std::function<void()> drawContents,
									 std::function<bool()> isEnabled = {}) {
		(void)label;
		(void)drawContents;
		(void)isEnabled;
	}

	/**
	 * @brief Register a View menu submenu (e.g. Camera to Top / Left).
	 * @details @p drawContents runs inside an open `BeginMenu`.
	 */
	virtual void registerViewSubmenu(const std::string& label, std::function<void()> drawContents) {
		(void)label;
		(void)drawContents;
	}

	/** @brief Register a View menu row (e.g. Pages). */
	virtual void registerViewAction(const std::string& label, std::function<void()> action,
									const std::string& shortcut = {}) {
		(void)label;
		(void)action;
		(void)shortcut;
	}

	/**
	 * @brief Bind a named keyboard shortcut (Shortcuts panel + fulfillment input).
	 * @param id Stable id (e.g. "view.reload"). Re-register replaces the action.
	 * @param label Display name (e.g. "Reload").
	 * @param chord Menu-style chord (e.g. "Ctrl+R"). Empty leaves the key unbound.
	 * @param action Invoked when the chord is pressed.
	 */
	virtual void registerShortcut(const std::string& id, const std::string& label,
								  const std::string& chord, std::function<void()> action) {
		(void)id;
		(void)label;
		(void)chord;
		(void)action;
	}

	/** @brief Gizmo / tool op for Tools menu (Select V, Move W, Rotate E, Scale R). */
	enum class GizmoOp { Select, Translate, Rotate, Scale };
	virtual void setGizmoOp(GizmoOp op) { (void)op; }
	virtual GizmoOp gizmoOp() const { return GizmoOp::Select; }

	/**
	 * @brief Extra fonts for packs (e.g. monospace code editor).
	 * @details Called during fulfillment font load / reload, before the atlas is
	 * used. Pass nullptr to clear. Multiple packs may register; order is FIFO.
	 */
	virtual void registerFontAtlasHook(std::function<void(ImFontAtlas& atlas)> hook) { (void)hook; }

	/**
	 * @brief Extra advance (pixels at `sizePx`) for a chrome label pair.
	 * @details 0 = no kern. Bind VarFont `GetKernTablePairPx` /
	 * `GetGposPairExtraPx` here. Absent = TTF `kern` table when present.
	 */
	using ChromeKernFn = float (*)(unsigned left, unsigned right, float sizePx, void* user);

	/** @brief When true, chrome labels apply pair kerning (default on). */
	virtual void setChromeKerning(bool enabled) { (void)enabled; }
	virtual bool chromeKerning() const { return false; }
	virtual void setChromeKernFn(ChromeKernFn fn, void* user) {
		(void)fn;
		(void)user;
	}
	/** @brief Loaded `kern` pairs, or 0 when using a custom fn / none. */
	virtual int chromeKernPairCount() const { return 0; }

	/**
	 * @brief Optional About dialog intro (Help to About). Empty = default blurb + app.json.
	 * @details Header is always "RigKit" plus `rigkit::version()`. Empty intro keeps the
	 * host blurb, then app name / version / description / license. Pass a non-empty
	 * string to replace the intro block (not the RigKit header).
	 */
	virtual void setAboutIntro(std::string text) { (void)text; }
	virtual const std::string& aboutIntro() const {
		static const std::string empty;
		return empty;
	}

	/**
	 * @brief Progress chrome (status bar or floating). Null when UI is detached.
	 * @details Thread-safe begin / tick / finish. Show mode to nullptr (Rig only).
	 */
	virtual Progress* progress() { return nullptr; }
};

} // namespace rigkit
