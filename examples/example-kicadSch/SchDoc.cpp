#include "SchDoc.h"

#include "rendering/IRenderer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kPinEps = 0.02f;

struct Sexpr {
	bool list = false;
	std::string atom;
	std::vector<Sexpr> kids;
};

struct Xf {
	float x = 0.f;
	float y = 0.f;
	float deg = 0.f;
	bool mx = false;
	bool my = false;
	bool on = false;
};

struct Box {
	float x0 = 0.f;
	float y0 = 0.f;
	float x1 = 0.f;
	float y1 = 0.f;
	bool ok = false;

	void add(float x, float y) {
		if (!ok) {
			x0 = x1 = x;
			y0 = y1 = y;
			ok = true;
			return;
		}
		x0 = (std::min)(x0, x);
		y0 = (std::min)(y0, y);
		x1 = (std::max)(x1, x);
		y1 = (std::max)(y1, y);
	}
};

void skipWs(const std::string& s, size_t& i) {
	while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
		++i;
	}
}

Sexpr parseSexpr(const std::string& s, size_t& i, bool& ok) {
	skipWs(s, i);
	Sexpr n;
	if (!ok || i >= s.size()) {
		ok = false;
		return n;
	}
	if (s[i] == '(') {
		++i;
		n.list = true;
		skipWs(s, i);
		while (ok && i < s.size() && s[i] != ')') {
			n.kids.push_back(parseSexpr(s, i, ok));
			skipWs(s, i);
		}
		if (!ok || i >= s.size() || s[i] != ')') {
			ok = false;
			return n;
		}
		++i;
		return n;
	}
	if (s[i] == '"') {
		++i;
		while (i < s.size() && s[i] != '"') {
			if (s[i] == '\\' && i + 1 < s.size()) {
				n.atom.push_back(s[i + 1]);
				i += 2;
				continue;
			}
			n.atom.push_back(s[i++]);
		}
		if (i >= s.size()) {
			ok = false;
			return n;
		}
		++i;
		return n;
	}
	const size_t start = i;
	while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i])) && s[i] != '(' &&
		   s[i] != ')') {
		++i;
	}
	if (start == i) {
		ok = false;
		return n;
	}
	n.atom = s.substr(start, i - start);
	return n;
}

bool bareAtom(const std::string& a) {
	if (a.empty()) {
		return false;
	}
	for (char c : a) {
		const unsigned char u = static_cast<unsigned char>(c);
		if (!(std::isalnum(u) || c == '_' || c == '.' || c == ':' || c == '~' || c == '+' ||
			  c == '-' || c == '/')) {
			return false;
		}
	}
	return true;
}

void printSexpr(std::ostream& o, const Sexpr& n, int indent) {
	if (!n.list) {
		if (bareAtom(n.atom)) {
			o << n.atom;
			return;
		}
		o << '"';
		for (char c : n.atom) {
			if (c == '"' || c == '\\') {
				o << '\\';
			}
			o << c;
		}
		o << '"';
		return;
	}
	o << '(';
	for (size_t k = 0; k < n.kids.size(); ++k) {
		if (k) {
			if (n.kids[k].list) {
				o << '\n' << std::string(static_cast<size_t>(indent + 1), '\t');
			} else {
				o << ' ';
			}
		}
		printSexpr(o, n.kids[k], indent + 1);
	}
	o << ')';
}

bool headed(const Sexpr& n, const char* name) {
	return n.list && !n.kids.empty() && !n.kids[0].list && n.kids[0].atom == name;
}

const Sexpr* child(const Sexpr& n, const char* name) {
	if (!n.list) {
		return nullptr;
	}
	for (const Sexpr& c : n.kids) {
		if (headed(c, name)) {
			return &c;
		}
	}
	return nullptr;
}

Sexpr* child(Sexpr& n, const char* name) {
	return const_cast<Sexpr*>(child(static_cast<const Sexpr&>(n), name));
}

std::string nth(const Sexpr& n, size_t i) {
	if (!n.list || n.kids.size() <= i || n.kids[i].list) {
		return {};
	}
	return n.kids[i].atom;
}

float nthf(const Sexpr& n, size_t i) {
	try {
		return std::stof(nth(n, i));
	} catch (...) {
		return 0.f;
	}
}

std::string formatMm(float v) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(v));
	std::string s = buf;
	while (s.size() > 1 && s.back() == '0') {
		s.pop_back();
	}
	if (!s.empty() && s.back() == '.') {
		s.pop_back();
	}
	if (s == "-0") {
		return "0";
	}
	return s;
}

void setNum(Sexpr& n, size_t i, float v) {
	if (n.list && n.kids.size() > i && !n.kids[i].list) {
		n.kids[i].atom = formatMm(v);
	}
}

bool hidden(const Sexpr& n) {
	if (const Sexpr* hide = child(n, "hide")) {
		if (nth(*hide, 1) == "yes") {
			return true;
		}
	}
	const Sexpr* effects = child(n, "effects");
	if (!effects) {
		return false;
	}
	const Sexpr* hide = child(*effects, "hide");
	return hide && nth(*hide, 1) == "yes";
}

float fontMm(const Sexpr& n) {
	const Sexpr* effects = child(n, "effects");
	const Sexpr* font = effects ? child(*effects, "font") : nullptr;
	const Sexpr* size = font ? child(*font, "size") : nullptr;
	if (!size) {
		return 1.27f;
	}
	const float h = nthf(*size, 1);
	return h > 0.f ? h : 1.27f;
}

float strokeMm(const Sexpr& n) {
	const Sexpr* stroke = child(n, "stroke");
	const Sexpr* width = stroke ? child(*stroke, "width") : nullptr;
	if (!width) {
		return 0.15f;
	}
	const float w = nthf(*width, 1);
	return w > 0.f ? w : 0.15f;
}

int fillKind(const Sexpr& n) {
	const Sexpr* fill = child(n, "fill");
	const Sexpr* type = fill ? child(*fill, "type") : nullptr;
	if (!type) {
		return 0;
	}
	const std::string t = nth(*type, 1);
	if (t == "background") {
		return 1;
	}
	if (t == "outline") {
		return 2;
	}
	return 0;
}

std::string localName(const std::string& libId) {
	const auto c = libId.rfind(':');
	return c == std::string::npos ? libId : libId.substr(c + 1);
}

bool splitSub(const std::string& name, std::string& base, int& unit, int& style) {
	const auto s = name.rfind('_');
	if (s == std::string::npos || s == 0) {
		return false;
	}
	const auto u = name.rfind('_', s - 1);
	if (u == std::string::npos) {
		return false;
	}
	try {
		unit = std::stoi(name.substr(u + 1, s - u - 1));
		style = std::stoi(name.substr(s + 1));
	} catch (...) {
		return false;
	}
	base = name.substr(0, u);
	return true;
}

glm::vec2 applyXf(const Xf& xf, float lx, float ly) {
	if (xf.mx) {
		lx = -lx;
	}
	if (xf.my) {
		ly = -ly;
	}
	if (!xf.on) {
		return {lx, ly};
	}
	const float r = xf.deg * kPi / 180.f;
	const float c = std::cos(r);
	const float s = std::sin(r);
	return {xf.x + lx * c - ly * s, xf.y + lx * s + ly * c};
}

Xf readXf(const Sexpr& inst) {
	Xf xf;
	xf.on = true;
	if (const Sexpr* at = child(inst, "at")) {
		xf.x = nthf(*at, 1);
		xf.y = nthf(*at, 2);
		xf.deg = nthf(*at, 3);
	}
	if (const Sexpr* mirror = child(inst, "mirror")) {
		for (size_t i = 1; i < mirror->kids.size(); ++i) {
			if (mirror->kids[i].list) {
				continue;
			}
			if (mirror->kids[i].atom == "x") {
				xf.mx = true;
			}
			if (mirror->kids[i].atom == "y") {
				xf.my = true;
			}
		}
	}
	return xf;
}

int readUnit(const Sexpr& inst) {
	if (const Sexpr* u = child(inst, "unit")) {
		const int n = static_cast<int>(nthf(*u, 1));
		return n > 0 ? n : 1;
	}
	return 1;
}

int readStyle(const Sexpr& inst) {
	if (const Sexpr* c = child(inst, "convert")) {
		const int n = static_cast<int>(nthf(*c, 1));
		return n > 0 ? n : 1;
	}
	return 1;
}

struct Out {
	std::function<void(float, float, float, float, float)> line;
	std::function<void(const std::vector<glm::vec2>&, float, int)> poly;
	std::function<void(float, float)> pin;
};

void emitPoly(Out& o, std::vector<glm::vec2> pts, float w, int fill) {
	if (pts.size() < 2) {
		return;
	}
	if (fill && o.poly) {
		o.poly(pts, w, fill);
	}
	if (o.line) {
		for (size_t i = 1; i < pts.size(); ++i) {
			o.line(pts[i - 1].x, pts[i - 1].y, pts[i].x, pts[i].y, w);
		}
	}
}

std::vector<glm::vec2> xyList(const Sexpr& pts) {
	std::vector<glm::vec2> v;
	for (const Sexpr& c : pts.kids) {
		if (headed(c, "xy")) {
			v.push_back({nthf(c, 1), nthf(c, 2)});
		}
	}
	return v;
}

void emitArc(Out& o, float ax, float ay, float bx, float by, float cx, float cy, float w) {
	const float d = 2.f * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
	if (std::fabs(d) < 1e-6f) {
		if (o.line) {
			o.line(ax, ay, bx, by, w);
			o.line(bx, by, cx, cy, w);
		}
		return;
	}
	const float a2 = ax * ax + ay * ay;
	const float b2 = bx * bx + by * by;
	const float c2 = cx * cx + cy * cy;
	const float ux = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
	const float uy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;
	auto ang = [&](float x, float y) { return std::atan2(y - uy, x - ux); };
	auto norm = [](float a) {
		const float pi2 = kPi * 2.f;
		while (a < 0.f) {
			a += pi2;
		}
		while (a >= pi2) {
			a -= pi2;
		}
		return a;
	};
	const float as = ang(ax, ay);
	const float am = ang(bx, by);
	const float ae = ang(cx, cy);
	const float dse = norm(ae - as);
	float sweep = dse;
	if (norm(am - as) > dse) {
		sweep = dse - kPi * 2.f;
	}
	std::vector<glm::vec2> pts;
	constexpr int kN = 16;
	pts.reserve(kN + 1);
	const float r = std::hypot(ax - ux, ay - uy);
	for (int i = 0; i <= kN; ++i) {
		const float t = as + sweep * (static_cast<float>(i) / kN);
		pts.push_back({ux + r * std::cos(t), uy + r * std::sin(t)});
	}
	emitPoly(o, std::move(pts), w, 0);
}

void emitBezier(Out& o, const std::vector<glm::vec2>& p, float w) {
	if (p.size() < 4) {
		emitPoly(o, p, w, 0);
		return;
	}
	std::vector<glm::vec2> pts;
	constexpr int kN = 12;
	for (int i = 0; i <= kN; ++i) {
		const float t = static_cast<float>(i) / kN;
		const float u = 1.f - t;
		pts.push_back(u * u * u * p[0] + 3.f * u * u * t * p[1] + 3.f * u * t * t * p[2] +
					  t * t * t * p[3]);
	}
	emitPoly(o, std::move(pts), w, 0);
}

void emitOne(const Sexpr& n, Out& o) {
	if (headed(n, "polyline")) {
		const Sexpr* pts = child(n, "pts");
		if (!pts) {
			return;
		}
		auto v = xyList(*pts);
		const float w = strokeMm(n);
		const int fill = fillKind(n);
		if (fill && o.poly) {
			o.poly(v, w, fill);
		}
		for (size_t i = 1; i < v.size() && o.line; ++i) {
			o.line(v[i - 1].x, v[i - 1].y, v[i].x, v[i].y, w);
		}
		return;
	}
	if (headed(n, "rectangle")) {
		const Sexpr* a = child(n, "start");
		const Sexpr* b = child(n, "end");
		if (!a || !b) {
			return;
		}
		const float x0 = nthf(*a, 1);
		const float y0 = nthf(*a, 2);
		const float x1 = nthf(*b, 1);
		const float y1 = nthf(*b, 2);
		std::vector<glm::vec2> v{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
		const float w = strokeMm(n);
		if (o.poly) {
			o.poly(v, w, fillKind(n));
		}
		if (o.line) {
			for (size_t i = 1; i < v.size(); ++i) {
				o.line(v[i - 1].x, v[i - 1].y, v[i].x, v[i].y, w);
			}
		}
		return;
	}
	if (headed(n, "circle")) {
		const Sexpr* c = child(n, "center");
		const Sexpr* rad = child(n, "radius");
		if (!c || !rad) {
			return;
		}
		const float cx = nthf(*c, 1);
		const float cy = nthf(*c, 2);
		const float r = nthf(*rad, 1);
		std::vector<glm::vec2> v;
		constexpr int kN = 24;
		for (int i = 0; i <= kN; ++i) {
			const float t = 2.f * kPi * static_cast<float>(i) / kN;
			v.push_back({cx + r * std::cos(t), cy + r * std::sin(t)});
		}
		const float w = strokeMm(n);
		if (o.poly) {
			o.poly(v, w, fillKind(n));
		}
		if (o.line) {
			for (size_t i = 1; i < v.size(); ++i) {
				o.line(v[i - 1].x, v[i - 1].y, v[i].x, v[i].y, w);
			}
		}
		return;
	}
	if (headed(n, "arc")) {
		const Sexpr* a = child(n, "start");
		const Sexpr* b = child(n, "mid");
		const Sexpr* c = child(n, "end");
		if (!a || !b || !c) {
			return;
		}
		emitArc(o, nthf(*a, 1), nthf(*a, 2), nthf(*b, 1), nthf(*b, 2), nthf(*c, 1), nthf(*c, 2),
				strokeMm(n));
		return;
	}
	if (headed(n, "bezier")) {
		const Sexpr* pts = child(n, "pts");
		if (!pts) {
			return;
		}
		emitBezier(o, xyList(*pts), strokeMm(n));
		return;
	}
	if (headed(n, "pin")) {
		const Sexpr* at = child(n, "at");
		const Sexpr* len = child(n, "length");
		if (!at) {
			return;
		}
		const float x = nthf(*at, 1);
		const float y = nthf(*at, 2);
		const float ang = nthf(*at, 3) * kPi / 180.f;
		const float L = len ? nthf(*len, 1) : 0.f;
		if (o.line) {
			o.line(x, y, x + std::cos(ang) * L, y + std::sin(ang) * L, 0.15f);
		}
		if (o.pin) {
			o.pin(x, y);
		}
	}
}

void visitDef(const Sexpr& def, int unit, int style, Out& o,
			  const std::function<const Sexpr*(const std::string&)>& parent = {}, int depth = 0) {
	if (depth > 6) {
		return;
	}
	if (parent) {
		if (const Sexpr* ext = child(def, "extends")) {
			if (const Sexpr* p = parent(nth(*ext, 1))) {
				visitDef(*p, unit, style, o, parent, depth + 1);
			}
		}
	}
	const std::string local = localName(nth(def, 1));
	for (const Sexpr& c : def.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		std::string base;
		int u = 0;
		int st = 0;
		if (!splitSub(nth(c, 1), base, u, st)) {
			continue;
		}
		if (base != local || st != style || !(u == 0 || u == unit)) {
			continue;
		}
		for (const Sexpr& g : c.kids) {
			emitOne(g, o);
		}
	}
}

// 5x7, bit 4 is the left column. Missing glyphs draw nothing.
const uint8_t* glyph(char ch) {
	static const uint8_t g[][7] = {
		{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
		{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
		{0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F}, // 2
		{0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, // 3
		{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
		{0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
		{0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
		{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
		{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
		{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
		{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
		{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
		{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
		{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, // D
		{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
		{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
		{0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
		{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
		{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
		{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}, // J
		{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
		{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
		{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
		{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
		{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
		{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
		{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
		{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
		{0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E}, // S
		{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
		{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
		{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
		{0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, // W
		{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
		{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // Y
		{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
	};
	if (ch >= 'a' && ch <= 'z') {
		ch = static_cast<char>(ch - 'a' + 'A');
	}
	if (ch >= '0' && ch <= '9') {
		return g[ch - '0'];
	}
	if (ch >= 'A' && ch <= 'Z') {
		return g[10 + (ch - 'A')];
	}
	static const uint8_t dot[] = {0, 0, 0, 0, 0, 0, 0x04};
	static const uint8_t minus[] = {0, 0, 0, 0x0E, 0, 0, 0};
	static const uint8_t plus[] = {0, 0x04, 0x04, 0x1F, 0x04, 0x04, 0};
	static const uint8_t us[] = {0, 0, 0, 0, 0, 0, 0x1F};
	static const uint8_t slash[] = {0x01, 0x02, 0x04, 0x04, 0x08, 0x10, 0};
	static const uint8_t colon[] = {0, 0x04, 0, 0, 0, 0x04, 0};
	if (ch == '.') {
		return dot;
	}
	if (ch == '-') {
		return minus;
	}
	if (ch == '+') {
		return plus;
	}
	if (ch == '_') {
		return us;
	}
	if (ch == '/') {
		return slash;
	}
	if (ch == ':') {
		return colon;
	}
	return nullptr;
}

bool isDir(const std::filesystem::path& p) {
	std::error_code ec;
	return std::filesystem::is_directory(p, ec);
}

std::string findSymbolDir() {
	const char* vars[] = {"KICAD10_SYMBOL_DIR", "KICAD9_SYMBOL_DIR", "KICAD8_SYMBOL_DIR",
						  "KICAD_SYMBOL_DIR"};
	for (const char* v : vars) {
		if (const char* e = std::getenv(v)) {
			if (isDir(e)) {
				return e;
			}
		}
	}
#ifdef _WIN32
	if (const char* pf = std::getenv("ProgramFiles")) {
		std::error_code ec;
		const std::filesystem::path root = std::filesystem::path(pf) / "KiCad";
		if (isDir(root)) {
			for (const auto& ent : std::filesystem::directory_iterator(root, ec)) {
				const auto sym = ent.path() / "share" / "kicad" / "symbols";
				if (isDir(sym)) {
					return sym.string();
				}
			}
		}
	}
#else
	if (isDir("/usr/share/kicad/symbols")) {
		return "/usr/share/kicad/symbols";
	}
#endif
	return {};
}

std::string expandUri(const std::string& uri, const std::string& symbolDir) {
	std::string out;
	for (size_t i = 0; i < uri.size(); ++i) {
		if (uri[i] == '$' && i + 1 < uri.size() && uri[i + 1] == '{') {
			const auto end = uri.find('}', i + 2);
			if (end == std::string::npos) {
				out.push_back(uri[i]);
				continue;
			}
			const std::string var = uri.substr(i + 2, end - (i + 2));
			if (const char* env = std::getenv(var.c_str()); env && env[0]) {
				out += env;
			} else if (var.find("SYMBOL_DIR") != std::string::npos) {
				out += symbolDir;
			}
			i = end;
			continue;
		}
		out.push_back(uri[i]);
	}
	return out;
}

void readTable(const std::filesystem::path& table, const std::string& symbolDir,
			   std::unordered_map<std::string, std::string>& uris) {
	std::error_code ec;
	if (!std::filesystem::is_regular_file(table, ec)) {
		return;
	}
	std::ifstream in(table, std::ios::binary);
	if (!in) {
		return;
	}
	std::stringstream buf;
	buf << in.rdbuf();
	std::string text = buf.str();
	size_t i = 0;
	if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF) {
		i = 3;
	}
	bool ok = true;
	Sexpr root = parseSexpr(text, i, ok);
	if (!ok) {
		return;
	}
	for (const Sexpr& c : root.kids) {
		if (!headed(c, "lib")) {
			continue;
		}
		const Sexpr* name = child(c, "name");
		const Sexpr* uri = child(c, "uri");
		if (!name || !uri) {
			continue;
		}
		const std::string nick = nth(*name, 1);
		if (nick.empty() || uris.count(nick)) {
			continue;
		}
		uris.emplace(nick, expandUri(nth(*uri, 1), symbolDir));
	}
}

float paperW(const std::string& name) {
	if (name == "A3") {
		return 420.f;
	}
	if (name == "A2") {
		return 594.f;
	}
	if (name == "A1") {
		return 841.f;
	}
	if (name == "A0") {
		return 1189.f;
	}
	if (name == "A") {
		return 279.4f;
	}
	if (name == "B") {
		return 431.8f;
	}
	if (name == "C") {
		return 558.8f;
	}
	if (name == "D") {
		return 863.6f;
	}
	if (name == "E") {
		return 1117.6f;
	}
	if (name == "USLetter") {
		return 279.4f;
	}
	if (name == "USLegal") {
		return 355.6f;
	}
	return 297.f;
}

float paperH(const std::string& name) {
	if (name == "A3") {
		return 297.f;
	}
	if (name == "A2") {
		return 420.f;
	}
	if (name == "A1") {
		return 594.f;
	}
	if (name == "A0") {
		return 841.f;
	}
	if (name == "A") {
		return 215.9f;
	}
	if (name == "B") {
		return 279.4f;
	}
	if (name == "C") {
		return 431.8f;
	}
	if (name == "D") {
		return 558.8f;
	}
	if (name == "E") {
		return 863.6f;
	}
	if (name == "USLetter") {
		return 215.9f;
	}
	if (name == "USLegal") {
		return 215.9f;
	}
	return 210.f;
}

} // namespace

struct SchSheet {
	Sexpr root;
	std::string bytes;
	std::string path;
	std::string error;
	float pageW = 297.f;
	float pageH = 210.f;
	float gridMm = 1.27f;
	bool backedUp = false;
	std::string symbolDir;
	std::unordered_map<std::string, std::string> libUri;
	std::list<Sexpr> libStorage;
	std::unordered_map<std::string, Sexpr*> libByPath;
	std::unordered_map<std::string, Sexpr*> byId;

	struct End {
		Sexpr* xy = nullptr;
		glm::vec2 tip{0.f};
	};
	struct Rider {
		Sexpr* at = nullptr;
		float x = 0.f;
		float y = 0.f;
	};
	struct Drag {
		bool active = false;
		Sexpr* inst = nullptr;
		float startX = 0.f;
		float startY = 0.f;
		float startKx = 0.f;
		float startKy = 0.f;
		std::vector<End> ends;
		std::vector<Rider> riders;
	} drag;

	Sexpr* selected = nullptr;

	struct Place {
		bool open = false;
		bool armed = false;
		bool listed = false;
		std::string query;
		std::string armedId;
		int index = 0;
		std::vector<std::string> hits;
		std::vector<std::string> nicks;
		std::unordered_map<std::string, std::vector<std::string>> names;
		float aimX = 0.f;
		float aimY = 0.f;
		float panelX = 8.f;
		float panelY = 8.f;
		float panelW = 420.f;
		float rowH = 18.f;
	} place;
};

namespace {

Sexpr* loadLibFile(SchSheet& d, const std::string& path) {
	if (path.empty()) {
		return nullptr;
	}
	if (auto it = d.libByPath.find(path); it != d.libByPath.end()) {
		return it->second;
	}
	std::ifstream in(std::filesystem::path(path), std::ios::binary);
	if (!in) {
		spdlog::warn("symbol library missing: {}", path);
		return nullptr;
	}
	std::stringstream buf;
	buf << in.rdbuf();
	std::string text = buf.str();
	size_t i = 0;
	bool ok = true;
	Sexpr parsed = parseSexpr(text, i, ok);
	if (!ok || !parsed.list) {
		spdlog::warn("symbol library did not parse: {}", path);
		return nullptr;
	}
	d.libStorage.push_back(std::move(parsed));
	Sexpr* stored = &d.libStorage.back();
	d.libByPath.emplace(path, stored);
	return stored;
}

Sexpr* resolveId(SchSheet& d, const std::string& id) {
	if (auto it = d.byId.find(id); it != d.byId.end()) {
		return it->second;
	}
	const auto colon = id.find(':');
	if (colon == std::string::npos) {
		return nullptr;
	}
	const std::string nick = id.substr(0, colon);
	const std::string name = id.substr(colon + 1);
	auto uit = d.libUri.find(nick);
	if (uit == d.libUri.end() && !d.symbolDir.empty()) {
		d.libUri.emplace(nick, d.symbolDir + "/" + nick + ".kicad_sym");
		uit = d.libUri.find(nick);
	}
	if (uit == d.libUri.end()) {
		return nullptr;
	}
	Sexpr* file = loadLibFile(d, uit->second);
	if (!file) {
		return nullptr;
	}
	for (Sexpr& c : file->kids) {
		if (headed(c, "symbol") && nth(c, 1) == name) {
			d.byId.emplace(id, &c);
			return &c;
		}
	}
	spdlog::warn("symbol {} not in {}", id, uit->second);
	return nullptr;
}

void indexEmbedded(SchSheet& d) {
	d.byId.clear();
	const Sexpr* libs = child(d.root, "lib_symbols");
	if (!libs) {
		return;
	}
	for (Sexpr& c : const_cast<Sexpr*>(libs)->kids) {
		if (headed(c, "symbol")) {
			d.byId[nth(c, 1)] = &c;
		}
	}
}

void loadTables(SchSheet& d, const std::filesystem::path& schDir) {
	d.symbolDir = findSymbolDir();
	std::vector<std::filesystem::path> tables;
	tables.push_back(schDir / "sym-lib-table");
	if (const char* appdata = std::getenv("APPDATA")) {
		const std::filesystem::path base(appdata);
		tables.push_back(base / "kicad" / "10.0" / "sym-lib-table");
		tables.push_back(base / "kicad" / "9.0" / "sym-lib-table");
	}
	if (const char* home = std::getenv("HOME")) {
		const std::filesystem::path base(home);
		tables.push_back(base / ".config" / "kicad" / "10.0" / "sym-lib-table");
		tables.push_back(base / ".config" / "kicad" / "9.0" / "sym-lib-table");
	}
	if (!d.symbolDir.empty()) {
		auto sym = std::filesystem::path(d.symbolDir);
		tables.push_back(sym.parent_path() / "template" / "sym-lib-table");
	}
	for (const auto& t : tables) {
		readTable(t, d.symbolDir, d.libUri);
	}
}

void readPage(SchSheet& d) {
	const Sexpr* paper = child(d.root, "paper");
	if (!paper) {
		return;
	}
	const std::string name = nth(*paper, 1);
	if (paper->kids.size() >= 4 && !paper->kids[2].list && !paper->kids[3].list) {
		d.pageW = nthf(*paper, 2);
		d.pageH = nthf(*paper, 3);
		return;
	}
	d.pageW = paperW(name);
	d.pageH = paperH(name);
}

Out sheetOut(const Xf& xf, const std::function<void(glm::vec2, glm::vec2, float)>& seg,
			 const std::function<void(const std::vector<glm::vec2>&, float, int)>& poly,
			 const std::function<void(glm::vec2)>& pin) {
	Out o;
	o.line = [=](float x0, float y0, float x1, float y1, float w) {
		seg(applyXf(xf, x0, y0), applyXf(xf, x1, y1), w);
	};
	o.poly = [=](const std::vector<glm::vec2>& pts, float w, int fill) {
		std::vector<glm::vec2> s;
		s.reserve(pts.size());
		for (const glm::vec2& p : pts) {
			s.push_back(applyXf(xf, p.x, p.y));
		}
		poly(s, w, fill);
	};
	o.pin = [=](float x, float y) { pin(applyXf(xf, x, y)); };
	return o;
}

std::function<const Sexpr*(const std::string&)> parentLookup(SchSheet& d,
															 const std::string& libId) {
	const auto colon = libId.find(':');
	const std::string nick = colon == std::string::npos ? std::string() : libId.substr(0, colon);
	return [&d, nick](const std::string& shortName) -> const Sexpr* {
		if (nick.empty()) {
			return resolveId(d, shortName);
		}
		return resolveId(d, nick + ":" + shortName);
	};
}

std::vector<glm::vec2> pinTips(SchSheet& d, Sexpr& inst) {
	std::vector<glm::vec2> tips;
	const Sexpr* id = child(inst, "lib_id");
	const std::string libId = id ? nth(*id, 1) : std::string();
	Sexpr* def = id ? resolveId(d, libId) : nullptr;
	if (!def) {
		return tips;
	}
	Xf xf = readXf(inst);
	Out o = sheetOut(
		xf, [](glm::vec2, glm::vec2, float) {}, [](const std::vector<glm::vec2>&, float, int) {},
		[&](glm::vec2 p) { tips.push_back(p); });
	visitDef(*def, readUnit(inst), readStyle(inst), o, parentLookup(d, libId));
	return tips;
}

Box boundsOf(SchSheet& d, Sexpr& inst) {
	Box box;
	const Sexpr* id = child(inst, "lib_id");
	const std::string libId = id ? nth(*id, 1) : std::string();
	Sexpr* def = id ? resolveId(d, libId) : nullptr;
	Xf xf = readXf(inst);
	if (def) {
		Out o = sheetOut(
			xf,
			[&](glm::vec2 a, glm::vec2 b, float) {
				box.add(a.x, a.y);
				box.add(b.x, b.y);
			},
			[&](const std::vector<glm::vec2>& pts, float, int) {
				for (const glm::vec2& p : pts) {
					box.add(p.x, p.y);
				}
			},
			[&](glm::vec2 p) { box.add(p.x, p.y); });
		visitDef(*def, readUnit(inst), readStyle(inst), o, parentLookup(d, libId));
	}
	if (!box.ok) {
		box.add(xf.x - 4.f, xf.y - 4.f);
		box.add(xf.x + 4.f, xf.y + 4.f);
	} else {
		box.x0 -= 0.8f;
		box.y0 -= 0.8f;
		box.x1 += 0.8f;
		box.y1 += 0.8f;
	}
	return box;
}

bool closeTo(glm::vec2 a, float x, float y, float eps) {
	return std::fabs(a.x - x) <= eps && std::fabs(a.y - y) <= eps;
}

bool wireHas(const Sexpr& root, glm::vec2 p, float eps) {
	for (const Sexpr& c : root.kids) {
		if (!headed(c, "wire") && !headed(c, "bus") && !headed(c, "bus_entry")) {
			continue;
		}
		const Sexpr* pts = child(c, "pts");
		if (!pts) {
			continue;
		}
		for (const Sexpr& xy : pts->kids) {
			if (headed(xy, "xy") && closeTo(p, nthf(xy, 1), nthf(xy, 2), eps)) {
				return true;
			}
		}
	}
	return false;
}

float snapTo(float v, float grid) {
	if (grid <= 1e-6f) {
		return v;
	}
	return std::round(v / grid) * grid;
}

glm::vec2 toScreen(const rigkit::View2D& view, float pageH, float x, float y) {
	return view.toScreen(x, pageH - y);
}

void drawSeg(IRenderer& r, const rigkit::View2D& view, float pageH, glm::vec2 a, glm::vec2 b,
			 float widthMm, const glm::vec4& color) {
	const glm::vec2 sa = toScreen(view, pageH, a.x, a.y);
	const glm::vec2 sb = toScreen(view, pageH, b.x, b.y);
	const float w = (std::max)(1.f, widthMm * view.zoomAbs);
	r.drawLine(sa.x, sa.y, sb.x, sb.y, Paint::stroke(color, w));
}

void drawSheetPoly(IRenderer& r, const rigkit::View2D& view, float pageH,
				   const std::vector<glm::vec2>& pts, float widthMm, int fill) {
	if (pts.size() < 3) {
		return;
	}
	std::vector<glm::vec2> s;
	s.reserve(pts.size());
	for (const glm::vec2& p : pts) {
		s.push_back(toScreen(view, pageH, p.x, p.y));
	}
	if (fill == 1) {
		r.drawPolygon(s, Paint::fill({0.86f, 0.9f, 0.78f, 1.f}));
	} else if (fill == 2) {
		r.drawPolygon(s, Paint::fill({0.1f, 0.1f, 0.12f, 1.f}));
	}
	(void)widthMm;
}

void drawGlyphs(IRenderer& r, const rigkit::View2D& view, float pageH, float x, float y, float deg,
				float sizeMm, const std::string& text, const glm::vec4& color) {
	if (text.empty() || text == "~") {
		return;
	}
	const float step = sizeMm / 7.f;
	const float rad = deg * kPi / 180.f;
	const float c = std::cos(rad);
	const float s = std::sin(rad);
	float pen = 0.f;
	for (char ch : text) {
		if (ch == ' ') {
			pen += step * 4.f;
			continue;
		}
		const uint8_t* rows = glyph(ch);
		if (rows) {
			for (int row = 0; row < 7; ++row) {
				for (int col = 0; col < 5; ++col) {
					if ((rows[row] & (0x10 >> col)) == 0) {
						continue;
					}
					const float lx = pen + static_cast<float>(col) * step;
					const float ly = -static_cast<float>(row) * step;
					auto corner = [&](float ox, float oy) {
						return toScreen(view, pageH, x + (lx + ox) * c - (ly + oy) * s,
										y + (lx + ox) * s + (ly + oy) * c);
					};
					const glm::vec2 p0 = corner(0.f, 0.f);
					const glm::vec2 p1 = corner(step, 0.f);
					const glm::vec2 p2 = corner(step, -step);
					const glm::vec2 p3 = corner(0.f, -step);
					r.drawPolygon({p0, p1, p2, p3}, Paint::fill(color));
				}
			}
		}
		pen += step * 6.f;
	}
}

const glm::vec4 kWire{0.f, 0.42f, 0.22f, 1.f};
const glm::vec4 kBus{0.15f, 0.28f, 0.72f, 1.f};
const glm::vec4 kInk{0.08f, 0.08f, 0.1f, 1.f};
const glm::vec4 kLabel{0.12f, 0.22f, 0.62f, 1.f};
const glm::vec4 kGhost{0.12f, 0.32f, 0.72f, 1.f};

std::string lowerStr(std::string s) {
	for (char& c : s) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

int rankName(const std::string& lowerName, const std::string& rest) {
	if (rest.empty()) {
		return 1;
	}
	if (lowerName == rest) {
		return 0;
	}
	if (lowerName.compare(0, rest.size(), rest) == 0) {
		return 1;
	}
	if (lowerName.find(rest) != std::string::npos) {
		return 2;
	}
	return 3;
}

bool queryChar(char c) {
	const unsigned char u = static_cast<unsigned char>(c);
	if (std::isalnum(u)) {
		return true;
	}
	return c == ':' || c == '_' || c == '-' || c == '+' || c == '.' || c == '/' || c == '~';
}

bool topLevelSymbol(const std::string& line, std::string& name) {
	if (line.size() < 12 || line[0] != '\t' || line[1] == '\t') {
		return false;
	}
	static const char head[] = "(symbol \"";
	constexpr size_t hlen = sizeof(head) - 1;
	if (line.compare(1, hlen, head) != 0) {
		return false;
	}
	name.clear();
	for (size_t i = 1 + hlen; i < line.size(); ++i) {
		if (line[i] == '"') {
			return !name.empty();
		}
		if (line[i] == '\\' && i + 1 < line.size()) {
			name.push_back(line[++i]);
			continue;
		}
		name.push_back(line[i]);
	}
	return false;
}

void scanSymbolNames(const std::string& path, std::vector<std::string>& out) {
	std::ifstream in{std::filesystem::path(path)};
	if (!in) {
		return;
	}
	std::string line;
	while (std::getline(in, line)) {
		std::string name;
		if (topLevelSymbol(line, name)) {
			out.push_back(std::move(name));
		}
	}
}

std::string canonicalNick(const SchSheet& d, const std::string& typed) {
	const std::string low = lowerStr(typed);
	for (const std::string& n : d.place.nicks) {
		if (lowerStr(n) == low) {
			return n;
		}
	}
	return {};
}

void listLibraries(SchSheet& d) {
	d.place.nicks.clear();
	std::unordered_map<std::string, int> seen;
	auto add = [&](const std::string& n) {
		if (n.empty() || seen.count(n)) {
			return;
		}
		seen[n] = 1;
		d.place.nicks.push_back(n);
	};
	for (const auto& kv : d.libUri) {
		add(kv.first);
	}
	if (!d.symbolDir.empty()) {
		std::error_code ec;
		for (const auto& ent : std::filesystem::directory_iterator(d.symbolDir, ec)) {
			if (ent.path().extension() != ".kicad_sym") {
				continue;
			}
			const std::string nick = ent.path().stem().string();
			if (!d.libUri.count(nick)) {
				d.libUri.emplace(nick, ent.path().string());
			}
			add(nick);
		}
	}
	if (const Sexpr* libs = child(d.root, "lib_symbols")) {
		for (const Sexpr& c : libs->kids) {
			if (!headed(c, "symbol")) {
				continue;
			}
			const std::string id = nth(c, 1);
			const auto colon = id.find(':');
			if (colon != std::string::npos && colon > 0) {
				add(id.substr(0, colon));
			}
		}
	}
	std::sort(d.place.nicks.begin(), d.place.nicks.end(),
			  [](const std::string& a, const std::string& b) { return lowerStr(a) < lowerStr(b); });
	spdlog::info("{} symbol libraries", d.place.nicks.size());
}

void ensureNames(SchSheet& d, const std::string& nick) {
	if (d.place.names.count(nick)) {
		return;
	}
	std::vector<std::string> out;
	auto uit = d.libUri.find(nick);
	if (uit != d.libUri.end()) {
		scanSymbolNames(uit->second, out);
	} else if (!d.symbolDir.empty()) {
		scanSymbolNames(d.symbolDir + "/" + nick + ".kicad_sym", out);
	}
	if (const Sexpr* libs = child(d.root, "lib_symbols")) {
		const std::string prefix = nick + ":";
		for (const Sexpr& c : libs->kids) {
			if (!headed(c, "symbol")) {
				continue;
			}
			const std::string id = nth(c, 1);
			if (id.compare(0, prefix.size(), prefix) == 0) {
				out.push_back(id.substr(prefix.size()));
			}
		}
	}
	std::sort(out.begin(), out.end());
	out.erase(std::unique(out.begin(), out.end()), out.end());
	spdlog::info("{} symbols in {}", out.size(), nick);
	d.place.names.emplace(nick, std::move(out));
}

void refreshHits(SchSheet& d) {
	d.place.hits.clear();
	d.place.index = 0;
	const std::string q = lowerStr(d.place.query);
	const auto colon = q.find(':');
	struct Ranked {
		int rank = 0;
		std::string label;
	};
	std::vector<Ranked> ranked;
	if (colon == std::string::npos) {
		for (const std::string& nick : d.place.nicks) {
			const int rank = rankName(lowerStr(nick), q);
			if (rank < 3) {
				ranked.push_back({rank, nick + ":"});
			}
		}
	} else {
		const std::string nick = canonicalNick(d, d.place.query.substr(0, colon));
		if (!nick.empty()) {
			ensureNames(d, nick);
			const std::string rest = q.substr(colon + 1);
			auto it = d.place.names.find(nick);
			if (it != d.place.names.end()) {
				for (const std::string& name : it->second) {
					const int rank = rankName(lowerStr(name), rest);
					if (rank < 3) {
						ranked.push_back({rank, nick + ":" + name});
					}
				}
			}
		}
	}
	std::sort(ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b) {
		if (a.rank != b.rank) {
			return a.rank < b.rank;
		}
		return lowerStr(a.label) < lowerStr(b.label);
	});
	if (ranked.size() > 14) {
		ranked.resize(14);
	}
	for (const Ranked& h : ranked) {
		d.place.hits.push_back(h.label);
	}
}

int pickerRows(const SchSheet::Place& p) {
	if (p.hits.empty()) {
		return 4;
	}
	return 3 + static_cast<int>(p.hits.size());
}

void drawScreenText(IRenderer& r, float x, float y, float step, const std::string& text,
					const glm::vec4& color, float maxX) {
	float pen = x;
	for (char ch : text) {
		if (pen > maxX) {
			break;
		}
		if (ch == ' ') {
			pen += step * 4.f;
			continue;
		}
		const uint8_t* rows = glyph(ch);
		if (rows) {
			for (int row = 0; row < 7; ++row) {
				for (int col = 0; col < 5; ++col) {
					if ((rows[row] & (0x10 >> col)) == 0) {
						continue;
					}
					const float lx = pen + static_cast<float>(col) * step;
					const float ly = y + static_cast<float>(row) * step;
					r.drawPolygon(
						{{lx, ly}, {lx + step, ly}, {lx + step, ly + step}, {lx, ly + step}},
						Paint::fill(color));
				}
			}
		}
		pen += step * 6.f;
	}
}

void drawPicker(IRenderer& r, const SchSheet& d) {
	if (!d.place.open) {
		return;
	}
	const SchSheet::Place& p = d.place;
	const float h = p.rowH * static_cast<float>(pickerRows(p));
	r.drawRect(p.panelX, p.panelY, p.panelW, h, Paint::fill({0.14f, 0.15f, 0.17f, 1.f}));
	r.drawRect(p.panelX, p.panelY, p.panelW, h, Paint::stroke({0.35f, 0.4f, 0.48f, 1.f}, 1.f));
	const float textX = p.panelX + 8.f;
	const float maxX = p.panelX + p.panelW - 8.f;
	const float step = 2.f;
	auto rowY = [&](int row) { return p.panelY + static_cast<float>(row) * p.rowH + 2.f; };
	drawScreenText(r, textX, rowY(0), step, "PLACE FROM LIBRARY", {0.55f, 0.75f, 0.95f, 1.f}, maxX);
	drawScreenText(r, textX, rowY(1), step, "> " + p.query, {0.92f, 0.92f, 0.88f, 1.f}, maxX);
	const float caret = textX + static_cast<float>(2 + p.query.size()) * step * 6.f;
	if (caret < maxX) {
		r.drawRect(caret, rowY(1), step, step * 7.f, Paint::fill({0.92f, 0.92f, 0.88f, 1.f}));
	}
	if (p.hits.empty()) {
		drawScreenText(r, textX, rowY(2), step, "NO MATCH", {0.7f, 0.55f, 0.45f, 1.f}, maxX);
	}
	for (int i = 0; i < static_cast<int>(p.hits.size()); ++i) {
		const int row = 2 + i;
		if (i == p.index) {
			r.drawRect(p.panelX + 2.f, p.panelY + static_cast<float>(row) * p.rowH + 1.f,
					   p.panelW - 4.f, p.rowH - 2.f, Paint::fill({0.22f, 0.34f, 0.52f, 1.f}));
		}
		drawScreenText(r, textX, rowY(row), step, p.hits[static_cast<size_t>(i)],
					   {0.92f, 0.92f, 0.9f, 1.f}, maxX);
	}
	const int foot = pickerRows(p) - 1;
	drawScreenText(r, textX, rowY(foot), step, "UP DOWN ENTER   CLICK A ROW   ESC",
				   {0.55f, 0.58f, 0.62f, 1.f}, maxX);
}

Sexpr atomNode(std::string a) {
	Sexpr s;
	s.atom = std::move(a);
	return s;
}

Sexpr listNode(std::vector<Sexpr> kids) {
	Sexpr s;
	s.list = true;
	s.kids = std::move(kids);
	return s;
}

std::string newUuid() {
	static std::mt19937 rng{std::random_device{}()};
	std::uniform_int_distribution<int> dist(0, 15);
	std::string hex;
	hex.reserve(32);
	for (int i = 0; i < 32; ++i) {
		int v = dist(rng);
		if (i == 12) {
			v = 4;
		}
		if (i == 16) {
			v = (v & 3) | 8;
		}
		hex.push_back("0123456789abcdef"[v]);
	}
	std::string out;
	out.reserve(36);
	for (int i = 0; i < 32; ++i) {
		if (i == 8 || i == 12 || i == 16 || i == 20) {
			out.push_back('-');
		}
		out.push_back(hex[static_cast<size_t>(i)]);
	}
	return out;
}

std::string sheetUuid(SchSheet& d) {
	if (const Sexpr* u = child(d.root, "uuid")) {
		const std::string id = nth(*u, 1);
		if (!id.empty()) {
			return id;
		}
	}
	const std::string id = newUuid();
	d.root.kids.push_back(listNode({atomNode("uuid"), atomNode(id)}));
	return id;
}

std::string projectName(const SchSheet& d) {
	std::string name = std::filesystem::path(d.path).stem().string();
	return name.empty() ? std::string("sheet") : name;
}

bool isEmbedded(const SchSheet& d, const std::string& id) {
	const Sexpr* libs = child(d.root, "lib_symbols");
	if (!libs) {
		return false;
	}
	for (const Sexpr& c : libs->kids) {
		if (headed(c, "symbol") && nth(c, 1) == id) {
			return true;
		}
	}
	return false;
}

bool embeddedBody(const SchSheet& d, const std::string& id) {
	const Sexpr* libs = child(d.root, "lib_symbols");
	if (!libs) {
		return false;
	}
	for (const Sexpr& c : libs->kids) {
		if (!headed(c, "symbol") || nth(c, 1) != id) {
			continue;
		}
		if (child(c, "extends")) {
			return true;
		}
		for (const Sexpr& g : c.kids) {
			if (headed(g, "symbol")) {
				return true;
			}
		}
	}
	return false;
}

Sexpr* ensureLibs(SchSheet& d) {
	if (Sexpr* existing = child(d.root, "lib_symbols")) {
		return existing;
	}
	Sexpr node = listNode({atomNode("lib_symbols")});
	size_t at = 1;
	for (size_t i = 0; i < d.root.kids.size(); ++i) {
		if (headed(d.root.kids[i], "paper") || headed(d.root.kids[i], "uuid") ||
			headed(d.root.kids[i], "version") || headed(d.root.kids[i], "generator")) {
			at = i + 1;
		}
	}
	if (at > d.root.kids.size()) {
		at = d.root.kids.size();
	}
	d.root.kids.insert(d.root.kids.begin() + static_cast<std::ptrdiff_t>(at), std::move(node));
	return child(d.root, "lib_symbols");
}

bool embedOne(SchSheet& d, const std::string& id, int depth) {
	if (depth > 6 || id.empty()) {
		return false;
	}
	if (isEmbedded(d, id)) {
		return true;
	}
	Sexpr* def = resolveId(d, id);
	if (!def) {
		return false;
	}
	std::string parentShort;
	if (const Sexpr* ext = child(*def, "extends")) {
		parentShort = nth(*ext, 1);
	}
	if (!parentShort.empty()) {
		const auto colon = id.find(':');
		const std::string nick = colon == std::string::npos ? std::string() : id.substr(0, colon);
		const std::string parentId = nick.empty() ? parentShort : nick + ":" + parentShort;
		if (!embedOne(d, parentId, depth + 1)) {
			return false;
		}
		def = resolveId(d, id);
		if (!def) {
			return false;
		}
	}
	if (isEmbedded(d, id)) {
		return true;
	}
	Sexpr* libs = ensureLibs(d);
	if (!libs) {
		return false;
	}
	Sexpr copy = *def;
	if (copy.list && copy.kids.size() > 1 && !copy.kids[1].list) {
		copy.kids[1].atom = id;
	}
	libs->kids.push_back(std::move(copy));
	indexEmbedded(d);
	return isEmbedded(d, id);
}

void splitRef(const std::string& ref, std::string& prefix, int& num, bool& digits) {
	size_t i = ref.size();
	while (i > 0 && std::isdigit(static_cast<unsigned char>(ref[i - 1]))) {
		--i;
	}
	prefix = ref.substr(0, i);
	digits = i < ref.size();
	num = 0;
	if (!digits) {
		return;
	}
	try {
		num = std::stoi(ref.substr(i));
	} catch (...) {
		num = 0;
	}
}

std::string nextRef(const SchSheet& d, const std::string& seed) {
	std::string prefix;
	int num = 0;
	bool digits = false;
	splitRef(seed, prefix, num, digits);
	if (prefix.empty()) {
		prefix = "U";
	}
	int maxn = 0;
	for (const Sexpr& c : d.root.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		for (const Sexpr& p : c.kids) {
			if (!headed(p, "property") || nth(p, 1) != "Reference") {
				continue;
			}
			std::string rp;
			int rn = 0;
			bool rd = false;
			splitRef(nth(p, 2), rp, rn, rd);
			if (rd && rp == prefix && rn > maxn) {
				maxn = rn;
			}
		}
	}
	return prefix + std::to_string(maxn + 1);
}

std::string flagOf(const Sexpr& def, const char* name, const char* fallback) {
	if (const Sexpr* f = child(def, name)) {
		const std::string v = nth(*f, 1);
		if (!v.empty()) {
			return v;
		}
	}
	return fallback;
}

struct PropRec {
	std::string key;
	std::string val;
	float x = 0.f;
	float y = 0.f;
	float rot = 0.f;
	float size = 1.27f;
	bool hide = false;
};

std::vector<PropRec> propsFrom(const SchSheet& d, const Sexpr& def, float x, float y) {
	std::vector<PropRec> out;
	for (const Sexpr& p : def.kids) {
		if (!headed(p, "property")) {
			continue;
		}
		PropRec rec;
		rec.key = nth(p, 1);
		rec.val = nth(p, 2);
		if (rec.key.empty()) {
			continue;
		}
		if (rec.key == "Reference") {
			rec.val = nextRef(d, rec.val);
		}
		rec.size = fontMm(p);
		rec.hide = hidden(p);
		if (const Sexpr* at = child(p, "at")) {
			rec.x = x + nthf(*at, 1);
			rec.y = y + nthf(*at, 2);
			rec.rot = nthf(*at, 3);
		} else {
			rec.x = x + 2.54f;
			rec.y = y;
		}
		out.push_back(std::move(rec));
	}
	return out;
}

void collectPins(const Sexpr& def, std::vector<std::string>& out,
				 const std::function<const Sexpr*(const std::string&)>& parent, int depth) {
	if (depth > 6) {
		return;
	}
	if (parent) {
		if (const Sexpr* ext = child(def, "extends")) {
			if (const Sexpr* p = parent(nth(*ext, 1))) {
				collectPins(*p, out, parent, depth + 1);
			}
		}
	}
	const std::string local = localName(nth(def, 1));
	for (const Sexpr& c : def.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		std::string base;
		int unit = 0;
		int style = 0;
		if (!splitSub(nth(c, 1), base, unit, style)) {
			continue;
		}
		if (base != local || style != 1 || !(unit == 0 || unit == 1)) {
			continue;
		}
		for (const Sexpr& g : c.kids) {
			if (!headed(g, "pin")) {
				continue;
			}
			const Sexpr* number = child(g, "number");
			if (!number) {
				continue;
			}
			const std::string n = nth(*number, 1);
			if (n.empty() || std::find(out.begin(), out.end(), n) != out.end()) {
				continue;
			}
			out.push_back(n);
		}
	}
}

Sexpr propNode(const PropRec& rec) {
	Sexpr effects = listNode(
		{atomNode("effects"),
		 listNode({atomNode("font"), listNode({atomNode("size"), atomNode(formatMm(rec.size)),
											   atomNode(formatMm(rec.size))})})});
	if (rec.hide) {
		effects.kids.push_back(listNode({atomNode("hide"), atomNode("yes")}));
	}
	return listNode({atomNode("property"), atomNode(rec.key), atomNode(rec.val),
					 listNode({atomNode("at"), atomNode(formatMm(rec.x)), atomNode(formatMm(rec.y)),
							   atomNode(formatMm(rec.rot))}),
					 std::move(effects)});
}

std::string refOf(const Sexpr& inst) {
	for (const Sexpr& p : inst.kids) {
		if (headed(p, "property") && nth(p, 1) == "Reference") {
			return nth(p, 2);
		}
	}
	return {};
}

std::string libIdOf(const Sexpr& inst) {
	const Sexpr* id = child(inst, "lib_id");
	return id ? nth(*id, 1) : std::string();
}

int countSymbols(const SchSheet& d) {
	int n = 0;
	for (const Sexpr& c : d.root.kids) {
		if (headed(c, "symbol")) {
			++n;
		}
	}
	return n;
}

Sexpr* lastSymbol(SchSheet& d) {
	Sexpr* s = nullptr;
	for (Sexpr& c : d.root.kids) {
		if (headed(c, "symbol")) {
			s = &c;
		}
	}
	return s;
}

int countRef(const SchSheet& d, const std::string& ref) {
	int n = 0;
	for (const Sexpr& c : d.root.kids) {
		if (headed(c, "symbol") && refOf(c) == ref) {
			++n;
		}
	}
	return n;
}

} // namespace

struct SheetLink {
	std::string name;
	std::string path;
	int depth = 0;
};

struct SchNav {
	bool open = false;
	bool refit = false;
	int index = 0;
	float gridMm = 1.27f;
	float panelX = 8.f;
	float panelY = 8.f;
	float panelW = 460.f;
	float rowH = 18.f;
	std::vector<SheetLink> rows;
	std::vector<std::string> stack;
	std::unordered_map<std::string, std::unique_ptr<SchSheet>> cache;
};

std::string canonicalPath(const std::string& path) {
	namespace fs = std::filesystem;
	std::error_code ec;
	fs::path canon = fs::weakly_canonical(fs::path(path), ec);
	if (ec) {
		canon = fs::absolute(fs::path(path), ec).lexically_normal();
	}
	return canon.string();
}

bool fileExists(const std::filesystem::path& p) {
	std::error_code ec;
	return std::filesystem::is_regular_file(p, ec);
}

std::string findProjectRootSch(const std::string& anyPath) {
	namespace fs = std::filesystem;
	fs::path dir = fs::path(anyPath).parent_path();
	for (int up = 0; up < 4 && !dir.empty(); ++up) {
		std::error_code ec;
		for (const auto& ent : fs::directory_iterator(dir, ec)) {
			if (ent.path().extension() != ".kicad_pro") {
				continue;
			}
			fs::path sch = ent.path();
			sch.replace_extension(".kicad_sch");
			if (fileExists(sch)) {
				return canonicalPath(sch.string());
			}
		}
		if (!dir.has_parent_path() || dir == dir.parent_path()) {
			break;
		}
		dir = dir.parent_path();
	}
	return canonicalPath(anyPath);
}

std::string expandMod(std::string file, const std::filesystem::path& projectDir) {
	const std::string token = "${KIPRJMOD}";
	const auto at = file.find(token);
	if (at == std::string::npos) {
		return file;
	}
	std::string rest = file.substr(at + token.size());
	while (!rest.empty() && (rest[0] == '/' || rest[0] == '\\')) {
		rest.erase(rest.begin());
	}
	return (projectDir / rest).string();
}

std::string resolveSheetFile(const std::string& hostSheet, const std::string& file,
							 const std::filesystem::path& projectDir) {
	namespace fs = std::filesystem;
	const std::string expanded = expandMod(file, projectDir);
	fs::path rel(expanded);
	fs::path full = rel.is_absolute() ? rel : fs::path(hostSheet).parent_path() / rel;
	return canonicalPath(full.string());
}

bool quotedProp(const std::string& line, const char* key, std::string& out) {
	const std::string needle = std::string("(property \"") + key + "\" \"";
	const auto at = line.find(needle);
	if (at == std::string::npos) {
		return false;
	}
	out.clear();
	for (size_t i = at + needle.size(); i < line.size(); ++i) {
		if (line[i] == '"') {
			return true;
		}
		if (line[i] == '\\' && i + 1 < line.size()) {
			out.push_back(line[++i]);
			continue;
		}
		out.push_back(line[i]);
	}
	return false;
}

struct ChildRef {
	std::string name;
	std::string file;
};

std::vector<ChildRef> scanChildSheets(const std::filesystem::path& file) {
	std::vector<ChildRef> out;
	std::ifstream in{file};
	if (!in) {
		return out;
	}
	std::vector<std::string> names;
	std::vector<std::string> files;
	std::string line;
	while (std::getline(in, line)) {
		std::string value;
		if (quotedProp(line, "Sheetname", value)) {
			names.push_back(value);
		}
		if (quotedProp(line, "Sheetfile", value)) {
			files.push_back(value);
		}
	}
	const size_t n = (std::min)(names.size(), files.size());
	out.reserve(n);
	for (size_t i = 0; i < n; ++i) {
		if (!files[i].empty()) {
			out.push_back({names[i].empty() ? files[i] : names[i], files[i]});
		}
	}
	return out;
}

void walkSheets(const std::string& path, const std::string& name, int depth,
				const std::filesystem::path& projectDir, std::vector<std::string>& chain,
				std::vector<SheetLink>& out) {
	if (depth > 16 || out.size() > 200) {
		return;
	}
	const std::string key = canonicalPath(path);
	for (const std::string& seen : chain) {
		if (seen == key) {
			return;
		}
	}
	std::string label = name.empty() ? std::filesystem::path(key).stem().string() : name;
	out.push_back({label, key, depth});
	if (!fileExists(key)) {
		return;
	}
	chain.push_back(key);
	for (const ChildRef& child : scanChildSheets(key)) {
		const std::string childPath = resolveSheetFile(key, child.file, projectDir);
		walkSheets(childPath, child.name, depth + 1, projectDir, chain, out);
	}
	chain.pop_back();
}

std::vector<SheetLink> buildSheetTree(const std::string& anyPath) {
	std::vector<SheetLink> out;
	if (anyPath.empty()) {
		return out;
	}
	const std::string root = findProjectRootSch(anyPath);
	const auto projectDir = std::filesystem::path(root).parent_path();
	std::vector<std::string> chain;
	const std::string rootName = std::filesystem::path(root).stem().string();
	walkSheets(root, rootName, 0, projectDir, chain, out);
	return out;
}

std::string sheetProp(const Sexpr& sheet, const char* key) {
	for (const Sexpr& p : sheet.kids) {
		if (headed(p, "property") && nth(p, 1) == key) {
			return nth(p, 2);
		}
	}
	return {};
}

std::string sheetFileAt(const SchSheet& d, float kx, float ky) {
	if (d.path.empty()) {
		return {};
	}
	const auto projectDir = std::filesystem::path(findProjectRootSch(d.path)).parent_path();
	std::string found;
	for (const Sexpr& c : d.root.kids) {
		if (!headed(c, "sheet")) {
			continue;
		}
		const Sexpr* at = child(c, "at");
		const Sexpr* size = child(c, "size");
		if (!at || !size) {
			continue;
		}
		const float x = nthf(*at, 1);
		const float y = nthf(*at, 2);
		const float w = nthf(*size, 1);
		const float h = nthf(*size, 2);
		if (kx < x || kx > x + w || ky > y || ky < y - h) {
			continue;
		}
		const std::string file = sheetProp(c, "Sheetfile");
		if (!file.empty()) {
			found = resolveSheetFile(d.path, file, projectDir);
		}
	}
	return found;
}

int navFirst(const SchNav& nav) {
	constexpr int kWindow = 14;
	const int n = static_cast<int>(nav.rows.size());
	if (n <= kWindow) {
		return 0;
	}
	int first = nav.index - kWindow + 1;
	if (first < 0) {
		first = 0;
	}
	const int maxFirst = n - kWindow;
	if (first > maxFirst) {
		first = maxFirst;
	}
	return first;
}

int navVisible(const SchNav& nav) {
	constexpr int kWindow = 14;
	const int n = static_cast<int>(nav.rows.size());
	return n > 0 ? (std::min)(n, kWindow) : 1;
}

void drawNav(IRenderer& r, const SchNav& nav, const std::string& current) {
	if (!nav.open) {
		return;
	}
	const int first = navFirst(nav);
	const int vis = navVisible(nav);
	const float h = nav.rowH * static_cast<float>(vis + 2);
	r.drawRect(nav.panelX, nav.panelY, nav.panelW, h, Paint::fill({0.14f, 0.15f, 0.17f, 1.f}));
	r.drawRect(nav.panelX, nav.panelY, nav.panelW, h,
			   Paint::stroke({0.35f, 0.4f, 0.48f, 1.f}, 1.f));
	const float textX = nav.panelX + 8.f;
	const float maxX = nav.panelX + nav.panelW - 8.f;
	const float step = 2.f;
	auto rowY = [&](int row) { return nav.panelY + static_cast<float>(row) * nav.rowH + 2.f; };
	drawScreenText(r, textX, rowY(0), step, "SHEETS", {0.55f, 0.75f, 0.95f, 1.f}, maxX);
	if (nav.rows.empty()) {
		drawScreenText(r, textX, rowY(1), step, "NO SHEETS", {0.7f, 0.55f, 0.45f, 1.f}, maxX);
	}
	for (int i = 0; i < vis && !nav.rows.empty(); ++i) {
		const int idx = first + i;
		const SheetLink& link = nav.rows[static_cast<size_t>(idx)];
		const int row = 1 + i;
		if (idx == nav.index) {
			r.drawRect(nav.panelX + 2.f, nav.panelY + static_cast<float>(row) * nav.rowH + 1.f,
					   nav.panelW - 4.f, nav.rowH - 2.f, Paint::fill({0.22f, 0.34f, 0.52f, 1.f}));
		}
		const bool here = link.path == current;
		const glm::vec4 color =
			here ? glm::vec4{0.65f, 0.88f, 0.7f, 1.f} : glm::vec4{0.92f, 0.92f, 0.9f, 1.f};
		drawScreenText(r, textX + static_cast<float>(link.depth) * 12.f, rowY(row), step, link.name,
					   color, maxX);
	}
	drawScreenText(r, textX, rowY(vis + 1), step, "ENTER OPENS   BACKSPACE UP   ESC",
				   {0.55f, 0.58f, 0.62f, 1.f}, maxX);
}

bool loadSheet(SchSheet& d, const std::string& path) {
	d.error.clear();
	d.path = canonicalPath(path);
	d.drag = {};
	d.selected = nullptr;
	d.place = {};
	d.byId.clear();
	d.libUri.clear();
	d.libByPath.clear();
	d.libStorage.clear();
	d.backedUp = false;
	std::ifstream in(std::filesystem::path(d.path), std::ios::binary);
	if (!in) {
		d.error = "could not open " + path;
		return false;
	}
	std::stringstream buf;
	buf << in.rdbuf();
	d.bytes = buf.str();
	size_t i = 0;
	if (d.bytes.size() >= 3 && static_cast<unsigned char>(d.bytes[0]) == 0xEF) {
		i = 3;
	}
	bool ok = true;
	d.root = parseSexpr(d.bytes, i, ok);
	if (!ok || !headed(d.root, "kicad_sch")) {
		d.error = "not a kicad_sch: " + path;
		return false;
	}
	readPage(d);
	loadTables(d, std::filesystem::path(d.path).parent_path());
	indexEmbedded(d);
	int symbols = 0;
	int fetched = 0;
	for (Sexpr& c : d.root.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		++symbols;
		const Sexpr* id = child(c, "lib_id");
		if (!id) {
			continue;
		}
		const std::string libId = nth(*id, 1);
		if (!d.byId.count(libId)) {
			if (resolveId(d, libId)) {
				++fetched;
			}
		}
	}
	spdlog::info("sheet {}  {} x {} mm  {} symbols  {} from libraries  grid {} mm", d.path, d.pageW,
				 d.pageH, symbols, fetched, d.gridMm);
	return true;
}

SchDoc::SchDoc() : m(std::make_unique<SchSheet>()), n(std::make_unique<SchNav>()) {}

SchDoc::~SchDoc() = default;

bool SchDoc::load(const std::string& path) {
	n->cache.clear();
	n->stack.clear();
	n->rows.clear();
	n->open = false;
	n->index = 0;
	if (!loadSheet(*m, path)) {
		return false;
	}
	m->gridMm = n->gridMm;
	n->stack.push_back(m->path);
	return true;
}

bool SchDoc::save(const std::string& path) {
	if (!headed(m->root, "kicad_sch")) {
		m->error = "nothing loaded";
		return false;
	}
	namespace fs = std::filesystem;
	const fs::path dest(path.empty() ? m->path : path);
	if (!m->backedUp && !m->bytes.empty()) {
		const fs::path bak = dest.string() + ".bak";
		std::error_code ec;
		if (!fs::exists(bak, ec)) {
			std::ofstream out(bak, std::ios::binary);
			out << m->bytes;
		}
		m->backedUp = true;
	}
	std::ofstream out(dest, std::ios::binary);
	if (!out) {
		m->error = "could not write " + dest.string();
		return false;
	}
	printSexpr(out, m->root, 0);
	out << '\n';
	m->path = dest.string();
	spdlog::info("wrote {}", m->path);
	return true;
}

const std::string& SchDoc::path() const {
	return m->path;
}

const std::string& SchDoc::error() const {
	return m->error;
}

float SchDoc::pageW() const {
	return m->pageW;
}

float SchDoc::pageH() const {
	return m->pageH;
}

float SchDoc::grid() const {
	return m->gridMm;
}

void SchDoc::setGrid(float mm) {
	if (mm > 0.f) {
		m->gridMm = mm;
		n->gridMm = mm;
	}
}

void SchDoc::press(float kx, float ky) {
	m->drag = {};
	m->selected = nullptr;
	Sexpr* hit = nullptr;
	for (Sexpr& c : m->root.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		const Box box = boundsOf(*m, c);
		if (kx >= box.x0 && kx <= box.x1 && ky >= box.y0 && ky <= box.y1) {
			hit = &c;
		}
	}
	if (!hit) {
		const std::string child = sheetFileAt(*m, kx, ky);
		if (!child.empty() && child != m->path) {
			if (!openSheet(child)) {
				spdlog::error("{}", m->error);
			}
		}
		return;
	}
	m->selected = hit;
	const Sexpr* at = child(*hit, "at");
	if (!at) {
		return;
	}
	SchSheet::Drag& drag = m->drag;
	drag.active = true;
	drag.inst = hit;
	drag.startX = nthf(*at, 1);
	drag.startY = nthf(*at, 2);
	drag.startKx = kx;
	drag.startKy = ky;
	const std::vector<glm::vec2> tips = pinTips(*m, *hit);
	std::vector<const Sexpr*> used;
	auto fresh = [&](const Sexpr* p) {
		if (std::find(used.begin(), used.end(), p) != used.end()) {
			return false;
		}
		used.push_back(p);
		return true;
	};
	for (const glm::vec2& tip : tips) {
		for (Sexpr& c : m->root.kids) {
			if (!headed(c, "wire") && !headed(c, "bus") && !headed(c, "bus_entry")) {
				continue;
			}
			Sexpr* pts = child(c, "pts");
			if (!pts) {
				continue;
			}
			for (Sexpr& xy : pts->kids) {
				if (headed(xy, "xy") && closeTo(tip, nthf(xy, 1), nthf(xy, 2), kPinEps) &&
					fresh(&xy)) {
					drag.ends.push_back({&xy, tip});
				}
			}
		}
		for (Sexpr& c : m->root.kids) {
			const bool rider = headed(c, "junction") || headed(c, "label") ||
							   headed(c, "global_label") || headed(c, "hierarchical_label") ||
							   headed(c, "no_connect") || headed(c, "text");
			if (!rider) {
				continue;
			}
			Sexpr* atNode = child(c, "at");
			if (atNode && closeTo(tip, nthf(*atNode, 1), nthf(*atNode, 2), kPinEps) &&
				fresh(atNode)) {
				drag.riders.push_back({atNode, nthf(*atNode, 1), nthf(*atNode, 2)});
			}
		}
	}
	for (Sexpr& c : hit->kids) {
		if (!headed(c, "property")) {
			continue;
		}
		Sexpr* atNode = child(c, "at");
		if (atNode && fresh(atNode)) {
			drag.riders.push_back({atNode, nthf(*atNode, 1), nthf(*atNode, 2)});
		}
	}
}

void SchDoc::dragTo(float kx, float ky) {
	SchSheet::Drag& drag = m->drag;
	if (!drag.active || !drag.inst) {
		return;
	}
	Sexpr* at = child(*drag.inst, "at");
	if (!at) {
		return;
	}
	const float nx = snapTo(drag.startX + (kx - drag.startKx), m->gridMm);
	const float ny = snapTo(drag.startY + (ky - drag.startKy), m->gridMm);
	const float dx = nx - drag.startX;
	const float dy = ny - drag.startY;
	setNum(*at, 1, nx);
	setNum(*at, 2, ny);
	for (SchSheet::End& e : drag.ends) {
		setNum(*e.xy, 1, e.tip.x + dx);
		setNum(*e.xy, 2, e.tip.y + dy);
	}
	for (SchSheet::Rider& r : drag.riders) {
		setNum(*r.at, 1, r.x + dx);
		setNum(*r.at, 2, r.y + dy);
	}
}

void SchDoc::release() {
	m->drag.active = false;
}

bool SchDoc::dragging() const {
	return m->drag.active;
}

bool SchDoc::connectionsSurviveGridChange() {
	Sexpr* inst = nullptr;
	for (Sexpr& c : m->root.kids) {
		if (!headed(c, "symbol")) {
			continue;
		}
		for (const glm::vec2& tip : pinTips(*m, c)) {
			if (wireHas(m->root, tip, kPinEps)) {
				inst = &c;
				break;
			}
		}
		if (inst) {
			break;
		}
	}
	if (!inst) {
		m->error = "no pin sat on a wire";
		return false;
	}
	const Sexpr* at = child(*inst, "at");
	if (!at) {
		m->error = "symbol has no position";
		return false;
	}
	const float x = nthf(*at, 1);
	const float y = nthf(*at, 2);
	setGrid(2.54f);
	press(x, y);
	if (!dragging()) {
		m->error = "symbol did not pick up";
		return false;
	}
	dragTo(x + 3.f, y);
	release();
	const Sexpr* at2 = child(*inst, "at");
	const float x2 = at2 ? nthf(*at2, 1) : x;
	const float y2 = at2 ? nthf(*at2, 2) : y;
	if (std::fabs(x2 - x) < 0.01f && std::fabs(y2 - y) < 0.01f) {
		m->error = "symbol did not move";
		return false;
	}
	int still = 0;
	for (const glm::vec2& tip : pinTips(*m, *inst)) {
		if (wireHas(m->root, tip, 0.001f)) {
			++still;
		}
	}
	if (still < 1) {
		m->error = "a pin left its wire when the grid changed";
		return false;
	}
	return true;
}

bool SchDoc::probeInstalledLibraries() {
	if (m->symbolDir.empty() && m->libUri.empty()) {
		loadTables(*m, {});
	}
	if (m->symbolDir.empty() && !m->libUri.count("Device")) {
		spdlog::info("no KiCad symbol libraries on this machine - library probe skipped");
		return true;
	}
	if (!m->libUri.count("Device") && !m->symbolDir.empty()) {
		m->libUri.emplace("Device", m->symbolDir + "/Device.kicad_sym");
	}
	Sexpr* sym = resolveId(*m, "Device:R");
	if (!sym) {
		m->error = "KiCad libraries are present but Device:R did not resolve";
		return false;
	}
	spdlog::info("KiCad library ok - Device:R from {}", m->libUri["Device"]);
	return true;
}

void SchDoc::openChooser() {
	if (!m->place.listed) {
		listLibraries(*m);
		m->place.listed = true;
	}
	m->place.open = true;
	n->open = false;
	refreshHits(*m);
}

void SchDoc::closeChooser() {
	m->place.open = false;
}

void SchDoc::cancelPlace() {
	m->place.open = false;
	m->place.armed = false;
	m->place.armedId.clear();
}

bool SchDoc::choosing() const {
	return m->place.open;
}

bool SchDoc::placingSymbol() const {
	return m->place.armed && !m->place.open;
}

void SchDoc::typeChar(char c) {
	if (!queryChar(c) || m->place.query.size() >= 80) {
		return;
	}
	m->place.query.push_back(c);
	refreshHits(*m);
}

void SchDoc::backspace() {
	if (m->place.query.empty()) {
		return;
	}
	m->place.query.pop_back();
	refreshHits(*m);
}

void SchDoc::moveChoice(int delta) {
	if (m->place.hits.empty() || delta == 0) {
		return;
	}
	const int n = static_cast<int>(m->place.hits.size());
	int i = m->place.index + delta;
	if (i < 0) {
		i = n - 1;
	}
	if (i >= n) {
		i = 0;
	}
	m->place.index = i;
}

void SchDoc::confirmChoice() {
	if (m->place.hits.empty()) {
		return;
	}
	const std::string hit = m->place.hits[static_cast<size_t>(m->place.index)];
	if (!hit.empty() && hit.back() == ':') {
		m->place.query = hit;
		refreshHits(*m);
		return;
	}
	if (!resolveId(*m, hit)) {
		m->error = "symbol not in the library: " + hit;
		spdlog::warn("{}", m->error);
		return;
	}
	m->place.armedId = hit;
	m->place.armed = true;
	m->place.open = false;
	m->error.clear();
	spdlog::info("armed {} - click the sheet to place, esc to stop", hit);
}

bool SchDoc::pressChooser(float sx, float sy) {
	if (!m->place.open) {
		return false;
	}
	const float x0 = m->place.panelX;
	const float y0 = m->place.panelY;
	const float h = m->place.rowH * static_cast<float>(pickerRows(m->place));
	const bool inside = sx >= x0 && sx <= x0 + m->place.panelW && sy >= y0 && sy <= y0 + h;
	if (!inside) {
		m->place.open = false;
		return true;
	}
	const int row = static_cast<int>((sy - y0) / m->place.rowH);
	const int hit = row - 2;
	if (hit >= 0 && hit < static_cast<int>(m->place.hits.size())) {
		m->place.index = hit;
		confirmChoice();
	}
	return true;
}

void SchDoc::aim(float kx, float ky) {
	m->place.aimX = kx;
	m->place.aimY = ky;
}

bool SchDoc::placeLibrarySymbol(const std::string& id, float x, float y) {
	m->error.clear();
	m->drag = {};
	m->selected = nullptr;
	if (!headed(m->root, "kicad_sch")) {
		m->error = "nothing loaded";
		return false;
	}
	const std::string sheet = sheetUuid(*m);
	indexEmbedded(*m);
	if (!embedOne(*m, id, 0)) {
		m->error = "could not read " + id;
		return false;
	}
	Sexpr* def = resolveId(*m, id);
	if (!def) {
		m->error = "could not read " + id;
		return false;
	}
	x = snapTo(x, m->gridMm);
	y = snapTo(y, m->gridMm);
	const std::string sim = flagOf(*def, "exclude_from_sim", "no");
	const std::string bom = flagOf(*def, "in_bom", "yes");
	const std::string board = flagOf(*def, "on_board", "yes");
	const std::string dnp = flagOf(*def, "dnp", "no");
	std::vector<PropRec> props = propsFrom(*m, *def, x, y);
	bool hasRef = false;
	bool hasValue = false;
	for (const PropRec& p : props) {
		if (p.key == "Reference") {
			hasRef = true;
		}
		if (p.key == "Value") {
			hasValue = true;
		}
	}
	if (!hasRef) {
		std::string seed;
		for (const Sexpr& c : m->root.kids) {
			if (!headed(c, "symbol") || libIdOf(c) != id) {
				continue;
			}
			seed = refOf(c);
			if (!seed.empty()) {
				break;
			}
		}
		if (seed.empty()) {
			for (char ch : localName(id)) {
				if (!std::isalpha(static_cast<unsigned char>(ch))) {
					break;
				}
				seed.push_back(ch);
			}
		}
		if (seed.empty()) {
			seed = "U";
		}
		PropRec rec;
		rec.key = "Reference";
		rec.val = nextRef(*m, seed);
		rec.x = x + 2.54f;
		rec.y = y + 2.54f;
		props.insert(props.begin(), rec);
	}
	if (!hasValue) {
		PropRec rec;
		rec.key = "Value";
		rec.val = localName(id);
		rec.x = x + 2.54f;
		rec.y = y - 2.54f;
		props.push_back(rec);
	}
	std::vector<std::string> pins;
	collectPins(*def, pins, parentLookup(*m, id), 0);
	std::string ref = "U1";
	for (const PropRec& p : props) {
		if (p.key == "Reference") {
			ref = p.val;
			break;
		}
	}
	const std::string symUuid = newUuid();
	Sexpr inst = listNode(
		{atomNode("symbol"), listNode({atomNode("lib_id"), atomNode(id)}),
		 listNode({atomNode("at"), atomNode(formatMm(x)), atomNode(formatMm(y)), atomNode("0")}),
		 listNode({atomNode("unit"), atomNode("1")}),
		 listNode({atomNode("exclude_from_sim"), atomNode(sim)}),
		 listNode({atomNode("in_bom"), atomNode(bom)}),
		 listNode({atomNode("on_board"), atomNode(board)}),
		 listNode({atomNode("dnp"), atomNode(dnp)}),
		 listNode({atomNode("uuid"), atomNode(symUuid)})});
	for (const PropRec& p : props) {
		inst.kids.push_back(propNode(p));
	}
	for (const std::string& n : pins) {
		inst.kids.push_back(listNode(
			{atomNode("pin"), atomNode(n), listNode({atomNode("uuid"), atomNode(newUuid())})}));
	}
	inst.kids.push_back(
		listNode({atomNode("instances"),
				  listNode({atomNode("project"), atomNode(projectName(*m)),
							listNode({atomNode("path"), atomNode("/" + sheet),
									  listNode({atomNode("reference"), atomNode(ref)}),
									  listNode({atomNode("unit"), atomNode("1")})})})}));
	m->root.kids.push_back(std::move(inst));
	indexEmbedded(*m);
	m->selected = lastSymbol(*m);
	spdlog::info("placed {} as {}", id, ref);
	return true;
}

bool SchDoc::placeAtCursor() {
	if (!m->place.armed || m->place.armedId.empty()) {
		return false;
	}
	return placeLibrarySymbol(m->place.armedId, m->place.aimX, m->place.aimY);
}

bool SchDoc::placeSmoke() {
	std::string embedded;
	for (Sexpr& c : m->root.kids) {
		if (!headed(c, "symbol") || pinTips(*m, c).empty()) {
			continue;
		}
		embedded = libIdOf(c);
		if (!embedded.empty()) {
			break;
		}
	}
	const bool device = resolveId(*m, "Device:R") != nullptr;
	if (embedded.empty() && !device) {
		m->error = "nothing to place";
		return false;
	}
	auto drop = [&](const std::string& id, float x, float y) {
		const int before = countSymbols(*m);
		openChooser();
		m->place.query.clear();
		for (char c : id) {
			typeChar(c);
		}
		confirmChoice();
		if (!placingSymbol() || m->place.armedId != id) {
			if (m->place.hits.empty()) {
				m->error = "chooser had no match for " + id;
			} else if (m->place.armedId != id) {
				m->error = "chooser armed " + m->place.armedId + " instead of " + id;
			}
			spdlog::error("{}", m->error);
			for (const std::string& h : m->place.hits) {
				spdlog::error("  {}", h);
			}
			return false;
		}
		aim(x, y);
		if (!placeAtCursor()) {
			return false;
		}
		if (countSymbols(*m) != before + 1) {
			m->error = "place did not add a symbol";
			return false;
		}
		Sexpr* s = lastSymbol(*m);
		if (!s || libIdOf(*s) != id) {
			m->error = "placed the wrong symbol";
			return false;
		}
		const size_t pins = pinTips(*m, *s).size();
		if (pins < 1 || (id == "Device:R" && pins < 2)) {
			m->error = id + " lost its pins";
			return false;
		}
		if (!embeddedBody(*m, id)) {
			m->error = "symbol drawing was not stored in the sheet";
			return false;
		}
		const std::string ref = refOf(*s);
		if (ref.empty() || countRef(*m, ref) != 1) {
			m->error = "reference " + ref + " is not unique";
			return false;
		}
		return true;
	};
	if (!embedded.empty() && embedded != "Device:R") {
		if (!drop(embedded, 24.f, 36.f)) {
			return false;
		}
	}
	const std::string lib = device ? std::string("Device:R") : embedded;
	if (!drop(lib, 50.f, 36.f) || !drop(lib, 76.f, 36.f)) {
		return false;
	}
	spdlog::info("place ok");
	return true;
}

void SchDoc::refreshSheetList() {
	n->rows = buildSheetTree(m->path);
	const std::string here = m->path;
	n->index = 0;
	for (int i = 0; i < static_cast<int>(n->rows.size()); ++i) {
		if (n->rows[static_cast<size_t>(i)].path == here) {
			n->index = i;
			break;
		}
	}
}

bool SchDoc::showSheet(const std::string& path, bool push) {
	const std::string key = canonicalPath(path);
	if (key.empty()) {
		m->error = "missing sheet path";
		return false;
	}
	if (m->path == key && headed(m->root, "kicad_sch")) {
		return true;
	}
	std::unique_ptr<SchSheet> next;
	auto it = n->cache.find(key);
	if (it != n->cache.end()) {
		next = std::move(it->second);
		n->cache.erase(it);
	} else {
		next = std::make_unique<SchSheet>();
		next->gridMm = n->gridMm;
		if (!loadSheet(*next, key)) {
			m->error = next->error;
			return false;
		}
	}
	next->gridMm = n->gridMm;
	if (!m->path.empty() && headed(m->root, "kicad_sch")) {
		auto parked = std::move(m);
		n->cache[parked->path] = std::move(parked);
	}
	m = std::move(next);
	if (push && (n->stack.empty() || n->stack.back() != m->path)) {
		n->stack.push_back(m->path);
	}
	n->refit = true;
	if (n->open) {
		refreshSheetList();
	}
	spdlog::info("opened {}", std::filesystem::path(m->path).filename().string());
	return true;
}

bool SchDoc::openSheet(const std::string& path) {
	return showSheet(path, true);
}

bool SchDoc::goBack() {
	if (n->stack.size() < 2) {
		return false;
	}
	const std::string prev = n->stack[n->stack.size() - 2];
	if (!showSheet(prev, false)) {
		return false;
	}
	n->stack.pop_back();
	spdlog::info("back to {}", std::filesystem::path(m->path).filename().string());
	return true;
}

void SchDoc::toggleSheets() {
	n->open = !n->open;
	if (!n->open) {
		return;
	}
	m->place.open = false;
	refreshSheetList();
	spdlog::info("{} sheets in this project", n->rows.size());
}

bool SchDoc::sheetsOpen() const {
	return n->open;
}

void SchDoc::moveSheet(int delta) {
	if (n->rows.empty() || delta == 0) {
		return;
	}
	const int count = static_cast<int>(n->rows.size());
	int i = n->index + delta;
	if (i < 0) {
		i = count - 1;
	}
	if (i >= count) {
		i = 0;
	}
	n->index = i;
}

void SchDoc::confirmSheet() {
	if (n->rows.empty()) {
		n->open = false;
		return;
	}
	const SheetLink& link = n->rows[static_cast<size_t>(n->index)];
	n->open = false;
	if (link.path == m->path) {
		return;
	}
	if (!openSheet(link.path)) {
		spdlog::error("{}", m->error);
	}
}

bool SchDoc::pressSheets(float sx, float sy) {
	if (!n->open) {
		return false;
	}
	const int vis = navVisible(*n);
	const float h = n->rowH * static_cast<float>(vis + 2);
	const bool inside =
		sx >= n->panelX && sx <= n->panelX + n->panelW && sy >= n->panelY && sy <= n->panelY + h;
	if (!inside) {
		n->open = false;
		return true;
	}
	const int row = static_cast<int>((sy - n->panelY) / n->rowH);
	const int hit = navFirst(*n) + (row - 1);
	if (row >= 1 && row < vis + 1 && hit >= 0 && hit < static_cast<int>(n->rows.size())) {
		n->index = hit;
		confirmSheet();
	}
	return true;
}

bool SchDoc::consumeRefit() {
	const bool fit = n->refit;
	n->refit = false;
	return fit;
}

bool SchDoc::hierarchySmoke() {
	const std::string home = m->path;
	const int symbols = countSymbols(*m);
	if (m->symbolDir.empty()) {
		spdlog::info("no KiCad install - hierarchy probe skipped");
		return true;
	}
	const auto demo = std::filesystem::path(m->symbolDir).parent_path() / "demos" /
					  "complex_hierarchy" / "complex_hierarchy.kicad_sch";
	if (!fileExists(demo)) {
		spdlog::info("hierarchy demo not installed - probe skipped");
		return true;
	}
	if (!openSheet(demo.string())) {
		return false;
	}
	const std::vector<SheetLink> tree = buildSheetTree(m->path);
	int children = 0;
	std::string childPath;
	std::string childName;
	for (const SheetLink& link : tree) {
		if (link.depth < 1) {
			continue;
		}
		++children;
		if (childPath.empty()) {
			childPath = link.path;
			childName = link.name;
		}
	}
	if (children < 2 || childPath.empty()) {
		m->error = "hierarchy demo did not list its child sheets";
		return false;
	}
	const std::string hit = sheetFileAt(*m, 90.f, 90.f);
	if (hit.empty() || std::filesystem::path(hit).filename() != "ampli_ht.kicad_sch") {
		m->error = "click on the ampli sheet did not resolve ampli_ht.kicad_sch";
		return false;
	}
	if (!openSheet(childPath)) {
		return false;
	}
	if (std::filesystem::path(m->path).filename() != "ampli_ht.kicad_sch") {
		m->error = "opened " + m->path + " instead of ampli_ht.kicad_sch";
		return false;
	}
	if (!goBack() || std::filesystem::path(m->path).filename() != "complex_hierarchy.kicad_sch") {
		m->error = "back from a child did not return to the root sheet";
		return false;
	}
	if (!goBack() || m->path != home) {
		m->error = "back from the demo did not return to the open sheet";
		return false;
	}
	if (countSymbols(*m) != symbols) {
		m->error = "leaving a sheet and coming back dropped edits";
		return false;
	}
	spdlog::info("hierarchy ok - {} ({} child sheets, first {})", demo.filename().string(),
				 children, childName);
	return true;
}

void SchDoc::draw(IRenderer& renderer, const rigkit::View2D& view) {
	const glm::vec2 origin = view.toScreen(0.f, 0.f);
	renderer.drawRect(origin.x, origin.y, m->pageW * view.zoomAbs, m->pageH * view.zoomAbs,
					  Paint::fill({0.96f, 0.96f, 0.94f, 1.f}));
	renderer.drawRect(origin.x, origin.y, m->pageW * view.zoomAbs, m->pageH * view.zoomAbs,
					  Paint::stroke({0.7f, 0.7f, 0.68f, 1.f}, 1.f));

	if (m->gridMm * view.zoomAbs >= 6.f) {
		const glm::vec4 grid{0.86f, 0.86f, 0.82f, 1.f};
		for (float x = 0.f; x <= m->pageW + 0.01f; x += m->gridMm) {
			drawSeg(renderer, view, m->pageH, {x, 0.f}, {x, m->pageH}, 0.f, grid);
		}
		for (float y = 0.f; y <= m->pageH + 0.01f; y += m->gridMm) {
			drawSeg(renderer, view, m->pageH, {0.f, y}, {m->pageW, y}, 0.f, grid);
		}
	}

	auto paintLine = [&](glm::vec2 a, glm::vec2 b, float w, const glm::vec4& color) {
		drawSeg(renderer, view, m->pageH, a, b, w, color);
	};

	for (Sexpr& c : m->root.kids) {
		if (headed(c, "wire") || headed(c, "bus") || headed(c, "bus_entry")) {
			const Sexpr* pts = child(c, "pts");
			if (!pts) {
				continue;
			}
			const auto v = xyList(*pts);
			const glm::vec4 color = headed(c, "wire") ? kWire : kBus;
			const float w = headed(c, "bus") ? 0.35f : strokeMm(c);
			for (size_t i = 1; i < v.size(); ++i) {
				paintLine(v[i - 1], v[i], w, color);
			}
			continue;
		}
		if (headed(c, "junction")) {
			const Sexpr* at = child(c, "at");
			if (!at) {
				continue;
			}
			float diam = 1.f;
			if (const Sexpr* d = child(c, "diameter")) {
				diam = nthf(*d, 1);
			}
			const glm::vec2 s = toScreen(view, m->pageH, nthf(*at, 1), nthf(*at, 2));
			renderer.drawCircle(s.x, s.y, (std::max)(1.5f, diam * 0.5f * view.zoomAbs),
								Paint::fill(kWire));
			continue;
		}
		if (headed(c, "no_connect")) {
			const Sexpr* at = child(c, "at");
			if (!at) {
				continue;
			}
			const float x = nthf(*at, 1);
			const float y = nthf(*at, 2);
			paintLine({x - 0.6f, y - 0.6f}, {x + 0.6f, y + 0.6f}, 0.15f, kInk);
			paintLine({x - 0.6f, y + 0.6f}, {x + 0.6f, y - 0.6f}, 0.15f, kInk);
			continue;
		}
		if (headed(c, "label") || headed(c, "global_label") || headed(c, "hierarchical_label") ||
			headed(c, "text")) {
			const Sexpr* at = child(c, "at");
			if (!at || hidden(c)) {
				continue;
			}
			const std::string text = nth(c, 1);
			drawGlyphs(renderer, view, m->pageH, nthf(*at, 1), nthf(*at, 2), nthf(*at, 3),
					   fontMm(c), text, headed(c, "text") ? kInk : kLabel);
			continue;
		}
		if (headed(c, "polyline") || headed(c, "rectangle") || headed(c, "circle") ||
			headed(c, "arc") || headed(c, "bezier")) {
			Xf id;
			Out o = sheetOut(
				id, [&](glm::vec2 a, glm::vec2 b, float w) { paintLine(a, b, w, kInk); },
				[&](const std::vector<glm::vec2>& pts, float w, int fill) {
					drawSheetPoly(renderer, view, m->pageH, pts, w, fill);
				},
				[](glm::vec2) {});
			emitOne(c, o);
			continue;
		}
		if (headed(c, "sheet")) {
			const Sexpr* at = child(c, "at");
			const Sexpr* size = child(c, "size");
			if (at && size) {
				const float x = nthf(*at, 1);
				const float y = nthf(*at, 2);
				const float w = nthf(*size, 1);
				const float h = nthf(*size, 2);
				const glm::vec2 s = toScreen(view, m->pageH, x, y);
				renderer.drawRect(s.x, s.y, w * view.zoomAbs, h * view.zoomAbs,
								  Paint::fill({0.90f, 0.93f, 0.96f, 1.f}));
				drawSeg(renderer, view, m->pageH, {x, y}, {x + w, y}, 0.3f, kInk);
				drawSeg(renderer, view, m->pageH, {x + w, y}, {x + w, y - h}, 0.3f, kInk);
				drawSeg(renderer, view, m->pageH, {x + w, y - h}, {x, y - h}, 0.3f, kInk);
				drawSeg(renderer, view, m->pageH, {x, y - h}, {x, y}, 0.3f, kInk);
			}
			for (const Sexpr& p : c.kids) {
				if (headed(p, "property") && !hidden(p)) {
					const Sexpr* pat = child(p, "at");
					if (!pat) {
						continue;
					}
					drawGlyphs(renderer, view, m->pageH, nthf(*pat, 1), nthf(*pat, 2),
							   nthf(*pat, 3), fontMm(p), nth(p, 2), kInk);
					continue;
				}
				if (!headed(p, "pin")) {
					continue;
				}
				const Sexpr* pat = child(p, "at");
				if (!pat) {
					continue;
				}
				const float px = nthf(*pat, 1);
				const float py = nthf(*pat, 2);
				const float ang = nthf(*pat, 3) * kPi / 180.f;
				const float ix = px - std::cos(ang) * 1.6f;
				const float iy = py - std::sin(ang) * 1.6f;
				paintLine({px, py}, {ix, iy}, 0.15f, kInk);
				drawGlyphs(renderer, view, m->pageH, ix, iy, 0.f, fontMm(p), nth(p, 1), kLabel);
			}
			continue;
		}
		if (!headed(c, "symbol")) {
			continue;
		}
		const Sexpr* id = child(c, "lib_id");
		const std::string libId = id ? nth(*id, 1) : std::string();
		Sexpr* def = id ? resolveId(*m, libId) : nullptr;
		if (def) {
			const Xf xf = readXf(c);
			Out o = sheetOut(
				xf, [&](glm::vec2 a, glm::vec2 b, float w) { paintLine(a, b, w, kInk); },
				[&](const std::vector<glm::vec2>& pts, float w, int fill) {
					drawSheetPoly(renderer, view, m->pageH, pts, w, fill);
				},
				[](glm::vec2) {});
			visitDef(*def, readUnit(c), readStyle(c), o, parentLookup(*m, libId));
		}
		for (const Sexpr& p : c.kids) {
			if (!headed(p, "property") || hidden(p)) {
				continue;
			}
			const Sexpr* at = child(p, "at");
			if (!at) {
				continue;
			}
			drawGlyphs(renderer, view, m->pageH, nthf(*at, 1), nthf(*at, 2), nthf(*at, 3),
					   fontMm(p), nth(p, 2), kInk);
		}
	}

	if (m->place.armed && !m->place.open) {
		Sexpr* def = resolveId(*m, m->place.armedId);
		if (def) {
			Xf xf;
			xf.on = true;
			xf.x = snapTo(m->place.aimX, m->gridMm);
			xf.y = snapTo(m->place.aimY, m->gridMm);
			Out o = sheetOut(
				xf, [&](glm::vec2 a, glm::vec2 b, float w) { paintLine(a, b, w, kGhost); },
				[&](const std::vector<glm::vec2>& pts, float w, int fill) {
					drawSheetPoly(renderer, view, m->pageH, pts, w, fill);
				},
				[](glm::vec2) {});
			visitDef(*def, 1, 1, o, parentLookup(*m, m->place.armedId));
		}
	}

	if (m->selected) {
		const Box box = boundsOf(*m, *m->selected);
		if (box.ok) {
			const glm::vec2 s = toScreen(view, m->pageH, box.x0, box.y1);
			const float w = (box.x1 - box.x0) * view.zoomAbs;
			const float h = (box.y1 - box.y0) * view.zoomAbs;
			renderer.drawRect(s.x, s.y, w, h, Paint::stroke({0.15f, 0.4f, 0.9f, 1.f}, 1.5f));
		}
	}

	const std::string stem = std::filesystem::path(m->path).stem().string();
	char hud[192];
	if (m->place.armed) {
		std::snprintf(hud, sizeof(hud), "grid %g mm   %s   placing %s",
					  static_cast<double>(m->gridMm), stem.c_str(), m->place.armedId.c_str());
	} else {
		std::snprintf(hud, sizeof(hud), "grid %g mm   %s   H sheets   A place",
					  static_cast<double>(m->gridMm), stem.c_str());
	}
	drawGlyphs(renderer, view, m->pageH, 2.f, m->pageH - 3.f, 0.f, 2.2f, hud,
			   {0.35f, 0.35f, 0.38f, 1.f});
	drawPicker(renderer, *m);
	drawNav(renderer, *n, m->path);
}
