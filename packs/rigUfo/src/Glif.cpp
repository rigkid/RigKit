#include "Glif.h"

#include "XmlUtil.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace rigkit {
namespace ufo {
namespace {

using Cmd = ecs::CPath::Cmd;

struct Pt {
	float x = 0.f;
	float y = 0.f;
	std::string type;
};

uint32_t parseHex(const std::string& hex) {
	if (hex.empty()) {
		return 0;
	}
	return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
}

std::string hex4(uint32_t u) {
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%04X", u);
	return buf;
}

void emitContour(const std::vector<Pt>& pts, bool closed, ecs::CPath& path) {
	if (pts.empty()) {
		return;
	}
	auto on = [](const Pt& p) { return !p.type.empty() && p.type != "offcurve"; };
	path.commands.push_back({Cmd::MoveTo, {pts[0].x, pts[0].y}, {}, {}});
	size_t i = 1;
	while (i < pts.size()) {
		if (!on(pts[i])) {
			std::vector<Pt> offs;
			while (i < pts.size() && !on(pts[i])) {
				offs.push_back(pts[i]);
				++i;
			}
			if (i >= pts.size()) {
				break;
			}
			const Pt& dest = pts[i];
			ecs::CPath::Command c;
			c.p = {dest.x, dest.y};
			if ((dest.type == "curve" || dest.type.empty()) && offs.size() >= 2) {
				c.type = Cmd::CubicTo;
				c.c1 = {offs[0].x, offs[0].y};
				c.c2 = {offs[1].x, offs[1].y};
			} else if (dest.type == "qcurve" && !offs.empty()) {
				c.type = Cmd::QuadTo;
				c.c1 = {offs[0].x, offs[0].y};
			} else {
				c.type = Cmd::LineTo;
			}
			path.commands.push_back(c);
			++i;
			continue;
		}
		ecs::CPath::Command c;
		c.type = (pts[i].type == "move") ? Cmd::MoveTo : Cmd::LineTo;
		c.p = {pts[i].x, pts[i].y};
		path.commands.push_back(c);
		++i;
	}
	if (closed) {
		path.commands.push_back({Cmd::Close, {}, {}, {}});
	}
}

void writeContour(std::ostringstream& o, const ecs::CPath& path, size_t begin, size_t end,
				  bool closed) {
	o << "    <contour>\n";
	for (size_t i = begin; i < end; ++i) {
		const auto& c = path.commands[i];
		if (c.type == Cmd::Close) {
			continue;
		}
		const bool first = (i == begin);
		if (c.type == Cmd::MoveTo || c.type == Cmd::LineTo) {
			const char* type = (first && !closed) ? "move" : "line";
			o << "      <point x=\"" << c.p.x << "\" y=\"" << c.p.y << "\" type=\"" << type
			  << "\"/>\n";
		} else if (c.type == Cmd::CubicTo) {
			o << "      <point x=\"" << c.c1.x << "\" y=\"" << c.c1.y << "\"/>\n";
			o << "      <point x=\"" << c.c2.x << "\" y=\"" << c.c2.y << "\"/>\n";
			o << "      <point x=\"" << c.p.x << "\" y=\"" << c.p.y << "\" type=\"curve\"/>\n";
		} else if (c.type == Cmd::QuadTo) {
			o << "      <point x=\"" << c.c1.x << "\" y=\"" << c.c1.y << "\"/>\n";
			o << "      <point x=\"" << c.p.x << "\" y=\"" << c.p.y << "\" type=\"qcurve\"/>\n";
		}
	}
	o << "    </contour>\n";
}

} // namespace

bool parseGlif(std::string_view xml, Glif& out) {
	out = {};
	const auto glyphOpen = xml.find("<glyph");
	if (glyphOpen == std::string_view::npos) {
		return false;
	}
	const auto tagEnd = xml.find('>', glyphOpen);
	if (tagEnd == std::string_view::npos) {
		return false;
	}
	out.name = xmlAttr(xml.substr(glyphOpen, tagEnd - glyphOpen), "name");

	auto adv = xml.find("<advance");
	if (adv != std::string_view::npos) {
		const auto end = xml.find('>', adv);
		out.width = std::strtof(xmlAttr(xml.substr(adv, end - adv), "width").c_str(), nullptr);
	}

	size_t search = 0;
	while (true) {
		auto u = xml.find("<unicode", search);
		if (u == std::string_view::npos) {
			break;
		}
		const auto end = xml.find('>', u);
		const auto hex = xmlAttr(xml.substr(u, end - u), "hex");
		if (!hex.empty()) {
			out.unicodes.push_back(parseHex(hex));
		}
		search = end + 1;
	}

	search = 0;
	while (true) {
		auto a = xml.find("<anchor", search);
		if (a == std::string_view::npos) {
			break;
		}
		const auto end = xml.find('>', a);
		const auto tag = xml.substr(a, end - a);
		GlifAnchor an;
		an.name = xmlAttr(tag, "name");
		an.x = std::strtof(xmlAttr(tag, "x").c_str(), nullptr);
		an.y = std::strtof(xmlAttr(tag, "y").c_str(), nullptr);
		out.anchors.push_back(an);
		search = end + 1;
	}

	search = 0;
	while (true) {
		auto c = xml.find("<component", search);
		if (c == std::string_view::npos) {
			break;
		}
		const auto end = xml.find('>', c);
		const auto tag = xml.substr(c, end - c);
		ecs::CFontComponent comp;
		comp.glyph = xmlAttr(tag, "base");
		const auto sx = xmlAttr(tag, "xScale");
		const auto sy = xmlAttr(tag, "yScale");
		const auto xy = xmlAttr(tag, "xyScale");
		const auto yx = xmlAttr(tag, "yxScale");
		const auto xo = xmlAttr(tag, "xOffset");
		const auto yo = xmlAttr(tag, "yOffset");
		if (!sx.empty()) {
			comp.scaleX = std::strtof(sx.c_str(), nullptr);
		}
		if (!sy.empty()) {
			comp.scaleY = std::strtof(sy.c_str(), nullptr);
		}
		if (!xy.empty()) {
			comp.xyScale = std::strtof(xy.c_str(), nullptr);
		}
		if (!yx.empty()) {
			comp.yxScale = std::strtof(yx.c_str(), nullptr);
		}
		if (!xo.empty()) {
			comp.offsetX = std::strtof(xo.c_str(), nullptr);
		}
		if (!yo.empty()) {
			comp.offsetY = std::strtof(yo.c_str(), nullptr);
		}
		out.components.push_back(comp);
		search = end + 1;
	}

	search = 0;
	while (true) {
		auto c0 = xml.find("<contour", search);
		if (c0 == std::string_view::npos) {
			break;
		}
		const auto c1 = xml.find("</contour>", c0);
		if (c1 == std::string_view::npos) {
			break;
		}
		const auto body = xml.substr(c0, c1 - c0);
		std::vector<Pt> pts;
		size_t psearch = 0;
		while (true) {
			auto p = body.find("<point", psearch);
			if (p == std::string_view::npos) {
				break;
			}
			const auto end = body.find('>', p);
			const auto tag = body.substr(p, end - p);
			Pt pt;
			pt.x = std::strtof(xmlAttr(tag, "x").c_str(), nullptr);
			pt.y = std::strtof(xmlAttr(tag, "y").c_str(), nullptr);
			pt.type = xmlAttr(tag, "type");
			pts.push_back(pt);
			psearch = end + 1;
		}
		bool closed = true;
		if (!pts.empty() && pts[0].type == "move") {
			closed = false;
		}
		emitContour(pts, closed, out.path);
		search = c1 + 10;
	}
	return !out.name.empty();
}

std::string writeGlif(const Glif& g) {
	std::ostringstream o;
	o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	o << "<glyph name=\"" << xmlEscape(g.name) << "\" format=\"2\">\n";
	o << "  <advance width=\"" << g.width << "\"/>\n";
	for (uint32_t u : g.unicodes) {
		o << "  <unicode hex=\"" << hex4(u) << "\"/>\n";
	}
	o << "  <outline>\n";
	const auto& cmds = g.path.commands;
	size_t i = 0;
	while (i < cmds.size()) {
		if (cmds[i].type != Cmd::MoveTo) {
			++i;
			continue;
		}
		size_t end = i + 1;
		bool closed = false;
		while (end < cmds.size() && cmds[end].type != Cmd::MoveTo) {
			if (cmds[end].type == Cmd::Close) {
				closed = true;
			}
			++end;
		}
		writeContour(o, g.path, i, end, closed);
		i = end;
	}
	for (const auto& c : g.components) {
		o << "    <component base=\"" << xmlEscape(c.glyph) << "\"";
		if (c.scaleX != 1.f) {
			o << " xScale=\"" << c.scaleX << "\"";
		}
		if (c.scaleY != 1.f) {
			o << " yScale=\"" << c.scaleY << "\"";
		}
		if (c.xyScale != 0.f) {
			o << " xyScale=\"" << c.xyScale << "\"";
		}
		if (c.yxScale != 0.f) {
			o << " yxScale=\"" << c.yxScale << "\"";
		}
		if (c.offsetX != 0.f) {
			o << " xOffset=\"" << c.offsetX << "\"";
		}
		if (c.offsetY != 0.f) {
			o << " yOffset=\"" << c.offsetY << "\"";
		}
		o << "/>\n";
	}
	o << "  </outline>\n";
	for (const auto& a : g.anchors) {
		o << "  <anchor name=\"" << xmlEscape(a.name) << "\" x=\"" << a.x << "\" y=\"" << a.y
		  << "\"/>\n";
	}
	o << "</glyph>\n";
	return o.str();
}

} // namespace ufo
} // namespace rigkit
