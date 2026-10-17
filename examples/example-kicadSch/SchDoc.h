#pragma once

#include "core/util/View2D.h"

#include <memory>
#include <string>

class IRenderer;
struct SchSheet;
struct SchNav;

/**
 * @brief One KiCad schematic sheet, kept as the s-expression it was loaded from.
 * @details Symbol bodies come from the file's lib_symbols, then from KiCad's
 * sym-lib-table. Dragging a symbol snaps its anchor to the grid and moves every
 * wire end, junction, and label that sat on a pin with it, so a grid change
 * cannot drop a connection. A places a symbol from those same libraries.
 * Click a sheet box, or press H, to open another sheet in the same project.
 */
class SchDoc {
  public:
	SchDoc();
	~SchDoc();

	bool load(const std::string& path);
	bool save(const std::string& path);
	const std::string& path() const;
	const std::string& error() const;

	float pageW() const;
	float pageH() const;
	float grid() const;
	void setGrid(float mm);

	void draw(IRenderer& renderer, const rigkit::View2D& view);

	/// @param kx @param ky Sheet coordinates, Y up, origin at the bottom left.
	void press(float kx, float ky);
	void dragTo(float kx, float ky);
	void release();
	bool dragging() const;

	/// Move one connected symbol onto a coarser grid and check the wire still meets the pin.
	bool connectionsSurviveGridChange();
	/// True when Device:R can be read from an installed KiCad library, or when none is installed.
	bool probeInstalledLibraries();

	/// Library chooser. Type a nickname or Nickname:Symbol, Enter arms it, click the sheet to drop.
	void openChooser();
	void closeChooser();
	void cancelPlace();
	bool choosing() const;
	bool placingSymbol() const;
	void typeChar(char c);
	void backspace();
	void moveChoice(int delta);
	void confirmChoice();
	/// @return True when the click hit the chooser (or dismissed it) and must not drag a symbol.
	bool pressChooser(float sx, float sy);
	void aim(float kx, float ky);
	/// Drop another copy of the armed symbol at the cursor.
	bool placeAtCursor();
	/// Place from the chooser, including Device:R when KiCad is installed, and check the result.
	bool placeSmoke();

	/// Open a sheet file. The previous sheet stays in memory, so Backspace can return.
	bool openSheet(const std::string& path);
	bool goBack();
	void toggleSheets();
	bool sheetsOpen() const;
	void moveSheet(int delta);
	void confirmSheet();
	/// @return True when the click hit the sheet list and must not drag a symbol.
	bool pressSheets(float sx, float sy);
	/// True once after a sheet change, so the view can fit the new page.
	bool consumeRefit();
	/// Enter a child sheet of KiCad's complex_hierarchy demo and come back, when that demo is
	/// installed.
	bool hierarchySmoke();

  private:
	bool placeLibrarySymbol(const std::string& libId, float x, float y);
	bool showSheet(const std::string& path, bool push);
	void refreshSheetList();

	std::unique_ptr<SchSheet> m;
	std::unique_ptr<SchNav> n;
};
