#include "UfoIo.h"

#include "CCode.h"
#include "CFontAnchor.h"
#include "CFontAvar.h"
#include "CFontAxis.h"
#include "CFontCell.h"
#include "CFontComponent.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CFontLayer.h"
#include "CName.h"
#include "CPath.h"
#include "CRelationship.h"
#include "FontFind.h"
#include "Glif.h"
#include "Plist.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <miniz.h>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace rigkit {
namespace ufo {
namespace {

std::string readText(const fs::path& path) {
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		return {};
	}
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}

bool writeText(const fs::path& path, const std::string& text) {
	fs::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	if (!out) {
		return false;
	}
	out << text;
	return static_cast<bool>(out);
}

std::string lowerExt(std::string s) {
	for (char& c : s) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

bool isUfoz(const std::string& path) {
	const auto ext = lowerExt(fs::path(path).extension().string());
	return ext == ".ufoz";
}

std::string glyphFileName(const std::string& name) {
	std::string out;
	for (char c : name) {
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-') {
			out += c;
		} else {
			out += '_';
		}
	}
	if (out.empty()) {
		out = "glyph";
	}
	return out + ".glif";
}

std::string glyphNameOf(MEcs& ecs, entt::entity e) {
	if (ecs.hasComponent<ecs::CName>(e) && !ecs.getComponent<ecs::CName>(e).stableId.empty()) {
		return ecs.getComponent<ecs::CName>(e).stableId;
	}
	return ecs.entityName(e);
}

std::vector<entt::entity> childrenOf(MEcs& ecs, entt::entity parent) {
	std::vector<entt::entity> out;
	for (auto e : ecs.registry().view<ecs::CRelationship>()) {
		if (ecs.registry().get<ecs::CRelationship>(e).parent == parent) {
			out.push_back(e);
		}
	}
	return out;
}

void destroyFontEntities(MEcs& ecs) {
	std::vector<entt::entity> kill;
	auto take = [&](auto view) {
		for (auto e : view) {
			kill.push_back(e);
		}
	};
	take(ecs.registry().view<ecs::CFontFace>());
	take(ecs.registry().view<ecs::CFontLayer>());
	take(ecs.registry().view<ecs::CFontGlyph>());
	take(ecs.registry().view<ecs::CFontComponent>());
	take(ecs.registry().view<ecs::CFontAnchor>());
	take(ecs.registry().view<ecs::CFontKern>());
	take(ecs.registry().view<ecs::CFontGroup>());
	take(ecs.registry().view<ecs::CFontAxis>());
	take(ecs.registry().view<ecs::CFontAvar>());
	take(ecs.registry().view<ecs::CFontCell>());
	for (auto e : ecs.registry().view<ecs::CCode>()) {
		if (ecs.registry().get<ecs::CCode>(e).language == "fea") {
			kill.push_back(e);
		}
	}
	std::sort(kill.begin(), kill.end());
	kill.erase(std::unique(kill.begin(), kill.end()), kill.end());
	for (auto e : kill) {
		if (ecs.registry().valid(e)) {
			ecs.destroyEntity(e);
		}
	}
}

void addName(MEcs& ecs, entt::entity e, const std::string& name, const std::string& stable) {
	ecs::CName n;
	n.name = name;
	n.stableId = stable.empty() ? name : stable;
	ecs.addComponent(e, n);
}

void parentTo(MEcs& ecs, entt::entity e, entt::entity parent) {
	ecs::CRelationship rel;
	rel.parent = parent;
	ecs.addComponent(e, rel);
}

UfoResult loadUfoDir(MEcs& ecs, const fs::path& root) {
	UfoResult result;
	const auto fontinfo = readText(root / "fontinfo.plist");
	Plist info;
	if (!fontinfo.empty()) {
		parsePlist(fontinfo, info);
	}

	destroyFontEntities(ecs);

	const std::string family = info.string("familyName", "Untitled");
	const std::string style = info.string("styleName", "Regular");
	auto face = ecs.createEntity(family + " " + style);
	ecs::CFontFace facePod;
	facePod.family = family;
	facePod.style = style;
	facePod.unitsPerEm = static_cast<float>(info.number("unitsPerEm", 1000));
	facePod.ascender = static_cast<float>(info.number("ascender", 800));
	facePod.descender = static_cast<float>(info.number("descender", -200));
	facePod.capHeight = static_cast<float>(info.number("capHeight", 700));
	facePod.xHeight = static_cast<float>(info.number("xHeight", 500));
	facePod.version = info.string("openTypeNameVersion", info.string("versionMajor"));
	ecs.addComponent(face, facePod);
	addName(ecs, face, family + " " + style, family + "-" + style);
	result.face = face;

	const auto feaPath = root / "features.fea";
	if (fs::exists(feaPath)) {
		auto fea = ecs.createEntity("features.fea");
		ecs::CCode code;
		code.name = "features.fea";
		code.language = "fea";
		code.text = readText(feaPath);
		ecs.addComponent(fea, code);
		parentTo(ecs, fea, face);
		ecs.getComponent<ecs::CFontFace>(face).features = "features.fea";
	}

	std::vector<std::pair<std::string, fs::path>> layers;
	Plist layerContents;
	if (parsePlist(readText(root / "layercontents.plist"), layerContents) &&
		layerContents.kind == Plist::Array) {
		for (const auto& row : layerContents.arr) {
			if (row.kind == Plist::Array && row.arr.size() >= 2 &&
				row.arr[0].kind == Plist::String && row.arr[1].kind == Plist::String) {
				layers.emplace_back(row.arr[0].s, root / row.arr[1].s);
			}
		}
	}
	if (layers.empty()) {
		layers.emplace_back("public.default", root / "glyphs");
	}

	for (size_t li = 0; li < layers.size(); ++li) {
		const auto& [layerName, layerDir] = layers[li];
		auto layer = ecs.createEntity(layerName);
		ecs::CFontLayer layerPod;
		layerPod.role = (li == 0) ? ecs::CFontLayer::Foreground : ecs::CFontLayer::Spare;
		if (layerName == "background" || layerName.find("background") != std::string::npos) {
			layerPod.role = ecs::CFontLayer::Background;
		}
		ecs.addComponent(layer, layerPod);
		addName(ecs, layer, layerName, layerName);
		parentTo(ecs, layer, face);

		Plist contents;
		parsePlist(readText(layerDir / "contents.plist"), contents);
		if (contents.kind != Plist::Dict) {
			continue;
		}
		for (const auto& [gname, fileVal] : contents.dict) {
			if (fileVal.kind != Plist::String) {
				continue;
			}
			Glif glif;
			if (!parseGlif(readText(layerDir / fileVal.s), glif)) {
				continue;
			}
			auto glyph = ecs.createEntity(gname);
			ecs::CFontGlyph g;
			g.width = glif.width;
			g.unicodes = glif.unicodes;
			ecs.addComponent(glyph, g);
			addName(ecs, glyph, gname, gname);
			parentTo(ecs, glyph, layer);
			if (!glif.path.empty()) {
				ecs.addComponent(glyph, glif.path);
			}
			for (const auto& comp : glif.components) {
				auto child = ecs.createEntity();
				ecs.addComponent(child, comp);
				parentTo(ecs, child, glyph);
			}
			for (const auto& an : glif.anchors) {
				auto child = ecs.createEntity(an.name);
				ecs::CFontAnchor a;
				a.point = {an.x, an.y};
				ecs.addComponent(child, a);
				addName(ecs, child, an.name, an.name);
				parentTo(ecs, child, glyph);
			}
		}
	}

	Plist groups;
	if (parsePlist(readText(root / "groups.plist"), groups) && groups.kind == Plist::Dict) {
		for (const auto& [name, members] : groups.dict) {
			auto e = ecs.createEntity(name);
			ecs::CFontGroup g;
			if (members.kind == Plist::Array) {
				for (const auto& m : members.arr) {
					if (m.kind == Plist::String) {
						g.members.push_back(m.s);
					}
				}
			}
			if (name.rfind("public.kern1.", 0) == 0) {
				g.side = ecs::CFontGroup::Left;
			} else if (name.rfind("public.kern2.", 0) == 0) {
				g.side = ecs::CFontGroup::Right;
			}
			ecs.addComponent(e, g);
			addName(ecs, e, name, name);
			parentTo(ecs, e, face);
		}
	}

	Plist kerning;
	if (parsePlist(readText(root / "kerning.plist"), kerning) && kerning.kind == Plist::Dict) {
		for (const auto& [left, rights] : kerning.dict) {
			if (rights.kind != Plist::Dict) {
				continue;
			}
			for (const auto& [right, val] : rights.dict) {
				auto e = ecs.createEntity();
				ecs::CFontKern k;
				k.left = left;
				k.right = right;
				k.value = static_cast<float>(val.n);
				ecs.addComponent(e, k);
				parentTo(ecs, e, face);
			}
		}
	}

	result.ok = true;
	return result;
}

bool saveUfoDir(MEcs& ecs, const fs::path& root, std::string* error) {
	entt::entity face = entt::null;
	for (auto e : ecs.registry().view<ecs::CFontFace>()) {
		face = e;
		break;
	}
	if (face == entt::null) {
		if (error) {
			*error = "no CFontFace";
		}
		return false;
	}
	const auto& facePod = ecs.getComponent<ecs::CFontFace>(face);

	std::error_code ec;
	fs::create_directories(root, ec);

	Plist meta;
	meta.kind = Plist::Dict;
	meta.dict["creator"] = Plist::str("com.rigkid.ufomat");
	meta.dict["formatVersion"] = Plist::integer(3);
	if (!writeText(root / "metainfo.plist", writePlist(meta))) {
		if (error) {
			*error = "write metainfo.plist failed";
		}
		return false;
	}

	Plist info;
	info.kind = Plist::Dict;
	info.dict["familyName"] = Plist::str(facePod.family);
	info.dict["styleName"] = Plist::str(facePod.style);
	info.dict["unitsPerEm"] = Plist::integer(static_cast<long long>(facePod.unitsPerEm));
	info.dict["ascender"] = Plist::real(facePod.ascender);
	info.dict["descender"] = Plist::real(facePod.descender);
	info.dict["capHeight"] = Plist::real(facePod.capHeight);
	info.dict["xHeight"] = Plist::real(facePod.xHeight);
	if (!facePod.version.empty()) {
		info.dict["openTypeNameVersion"] = Plist::str(facePod.version);
	}
	writeText(root / "fontinfo.plist", writePlist(info));

	if (!facePod.features.empty()) {
		if (auto fea = font::findNamed(ecs, facePod.features);
			fea != entt::null && ecs.hasComponent<ecs::CCode>(fea)) {
			writeText(root / "features.fea", ecs.getComponent<ecs::CCode>(fea).text);
		}
	}

	entt::entity layer = entt::null;
	std::string layerName = "public.default";
	for (auto e : childrenOf(ecs, face)) {
		if (ecs.hasComponent<ecs::CFontLayer>(e)) {
			layer = e;
			layerName = glyphNameOf(ecs, e);
			if (layerName.empty()) {
				layerName = "public.default";
			}
			break;
		}
	}

	Plist layerContents;
	layerContents.kind = Plist::Array;
	Plist layerRow;
	layerRow.kind = Plist::Array;
	layerRow.arr.push_back(Plist::str(layerName));
	layerRow.arr.push_back(Plist::str("glyphs"));
	layerContents.arr.push_back(std::move(layerRow));
	writeText(root / "layercontents.plist", writePlist(layerContents));

	const fs::path glyphsDir = root / "glyphs";
	fs::create_directories(glyphsDir, ec);
	Plist contents;
	contents.kind = Plist::Dict;

	const auto glyphParents =
		(layer != entt::null) ? childrenOf(ecs, layer) : childrenOf(ecs, face);
	for (auto e : glyphParents) {
		if (!ecs.hasComponent<ecs::CFontGlyph>(e)) {
			continue;
		}
		const auto& g = ecs.getComponent<ecs::CFontGlyph>(e);
		Glif glif;
		glif.name = glyphNameOf(ecs, e);
		glif.width = g.width;
		glif.unicodes = g.unicodes;
		if (ecs.hasComponent<ecs::CPath>(e)) {
			glif.path = ecs.getComponent<ecs::CPath>(e);
		}
		for (auto child : childrenOf(ecs, e)) {
			if (ecs.hasComponent<ecs::CFontComponent>(child)) {
				glif.components.push_back(ecs.getComponent<ecs::CFontComponent>(child));
			}
			if (ecs.hasComponent<ecs::CFontAnchor>(child)) {
				GlifAnchor an;
				an.name = glyphNameOf(ecs, child);
				an.x = ecs.getComponent<ecs::CFontAnchor>(child).point.x;
				an.y = ecs.getComponent<ecs::CFontAnchor>(child).point.y;
				glif.anchors.push_back(an);
			}
		}
		const auto file = glyphFileName(glif.name);
		contents.dict[glif.name] = Plist::str(file);
		writeText(glyphsDir / file, writeGlif(glif));
	}
	writeText(glyphsDir / "contents.plist", writePlist(contents));

	Plist groups;
	groups.kind = Plist::Dict;
	Plist kerning;
	kerning.kind = Plist::Dict;
	for (auto e : childrenOf(ecs, face)) {
		if (ecs.hasComponent<ecs::CFontGroup>(e)) {
			const auto& g = ecs.getComponent<ecs::CFontGroup>(e);
			Plist members;
			members.kind = Plist::Array;
			for (const auto& m : g.members) {
				members.arr.push_back(Plist::str(m));
			}
			groups.dict[glyphNameOf(ecs, e)] = std::move(members);
		}
		if (ecs.hasComponent<ecs::CFontKern>(e)) {
			const auto& k = ecs.getComponent<ecs::CFontKern>(e);
			auto& rights = kerning.dict[k.left];
			rights.kind = Plist::Dict;
			rights.dict[k.right] = Plist::real(k.value);
		}
	}
	if (!groups.dict.empty()) {
		writeText(root / "groups.plist", writePlist(groups));
	}
	if (!kerning.dict.empty()) {
		writeText(root / "kerning.plist", writePlist(kerning));
	}
	return true;
}

bool unzipTo(const fs::path& zipPath, const fs::path& dest, std::string* error) {
	mz_zip_archive zip{};
	if (!mz_zip_reader_init_file(&zip, zipPath.string().c_str(), 0)) {
		if (error) {
			*error = "open ufoz failed";
		}
		return false;
	}
	const mz_uint n = mz_zip_reader_get_num_files(&zip);
	for (mz_uint i = 0; i < n; ++i) {
		mz_zip_archive_file_stat st{};
		if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_is_directory) {
			continue;
		}
		const fs::path out = dest / st.m_filename;
		fs::create_directories(out.parent_path());
		if (!mz_zip_reader_extract_to_file(&zip, i, out.string().c_str(), 0)) {
			mz_zip_reader_end(&zip);
			if (error) {
				*error = "extract ufoz failed";
			}
			return false;
		}
	}
	mz_zip_reader_end(&zip);
	return true;
}

bool zipDir(const fs::path& src, const fs::path& zipPath, std::string* error) {
	mz_zip_archive zip{};
	if (!mz_zip_writer_init_file(&zip, zipPath.string().c_str(), 0)) {
		if (error) {
			*error = "create ufoz failed";
		}
		return false;
	}
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(src, ec);
		 it != fs::recursive_directory_iterator(); ++it) {
		if (!it->is_regular_file()) {
			continue;
		}
		const auto rel = fs::relative(it->path(), src).generic_string();
		if (!mz_zip_writer_add_file(&zip, rel.c_str(), it->path().string().c_str(), nullptr, 0,
									MZ_DEFAULT_COMPRESSION)) {
			mz_zip_writer_end(&zip);
			if (error) {
				*error = "zip add failed";
			}
			return false;
		}
	}
	if (!mz_zip_writer_finalize_archive(&zip)) {
		mz_zip_writer_end(&zip);
		if (error) {
			*error = "zip finalize failed";
		}
		return false;
	}
	mz_zip_writer_end(&zip);
	return true;
}

fs::path findUfoRoot(const fs::path& dir) {
	if (fs::exists(dir / "fontinfo.plist") || fs::exists(dir / "metainfo.plist")) {
		return dir;
	}
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(dir, ec);
		 it != fs::recursive_directory_iterator(); ++it) {
		if (it->path().filename() == "fontinfo.plist") {
			return it->path().parent_path();
		}
	}
	return dir;
}

} // namespace

UfoResult loadUfo(MEcs& ecs, const std::string& path) {
	UfoResult result;
	if (isUfoz(path)) {
		const auto tmp = fs::temp_directory_path() / "ufomat-ufoz";
		std::error_code ec;
		fs::remove_all(tmp, ec);
		fs::create_directories(tmp, ec);
		if (!unzipTo(path, tmp, &result.error)) {
			return result;
		}
		result = loadUfoDir(ecs, findUfoRoot(tmp));
		fs::remove_all(tmp, ec);
		return result;
	}
	fs::path dir = path;
	if (fs::is_regular_file(dir) &&
		(dir.filename() == "fontinfo.plist" || dir.filename() == "metainfo.plist")) {
		dir = dir.parent_path();
	}
	if (!fs::is_directory(dir)) {
		result.error = "not a .ufo directory";
		return result;
	}
	return loadUfoDir(ecs, dir);
}

bool saveUfo(MEcs& ecs, const std::string& path, std::string* error) {
	if (isUfoz(path)) {
		const auto tmp = fs::temp_directory_path() / "ufomat-ufo-write";
		std::error_code ec;
		fs::remove_all(tmp, ec);
		if (!saveUfoDir(ecs, tmp, error)) {
			return false;
		}
		const bool ok = zipDir(tmp, path, error);
		fs::remove_all(tmp, ec);
		return ok;
	}
	return saveUfoDir(ecs, path, error);
}

bool fontmakeAvailable() {
#ifdef _WIN32
	return std::system("where fontmake >NUL 2>&1") == 0;
#else
	return std::system("command -v fontmake >/dev/null 2>&1") == 0;
#endif
}

bool exportTtf(MEcs& ecs, const std::string& outPath, std::string* error) {
	if (outPath.empty()) {
		if (error) {
			*error = "empty ttf path";
		}
		return false;
	}
	if (!fontmakeAvailable()) {
		if (error) {
			*error = "fontmake not found on PATH";
		}
		return false;
	}
	const auto tmp = fs::temp_directory_path() / "ufomat-fontmake.ufo";
	std::error_code ec;
	fs::remove_all(tmp, ec);
	if (!saveUfo(ecs, tmp.string(), error)) {
		return false;
	}
	std::ostringstream cmd;
	cmd << "fontmake -u \"" << tmp.string() << "\" -o ttf --output-path \"" << outPath << "\"";
	const int rc = std::system(cmd.str().c_str());
	fs::remove_all(tmp, ec);
	if (rc != 0) {
		if (error) {
			*error = "fontmake failed";
		}
		return false;
	}
	if (!fs::exists(outPath) || fs::file_size(outPath) == 0) {
		if (error) {
			*error = "fontmake produced no ttf";
		}
		return false;
	}
	return true;
}

} // namespace ufo
} // namespace rigkit
