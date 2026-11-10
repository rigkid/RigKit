#include "PrefsDoc.h"

#include "Json.h"

#include <cmath>
#include <cstring>

namespace rigkit::espapp {
namespace {

bool copyText(char* dst, size_t cap, const std::string& src, const char* tooLong,
			  std::string& err) {
	if (src.size() >= cap) {
		err = tooLong;
		return false;
	}
	if (!src.empty()) {
		std::memcpy(dst, src.data(), src.size());
	}
	dst[src.size()] = '\0';
	return true;
}

json::Value field(const char* key, const char* label, const char* type) {
	json::Value row = json::Value::obj();
	row.set("key", json::Value::str(key));
	row.set("label", json::Value::str(label));
	row.set("type", json::Value::str(type));
	return row;
}

} // namespace

const char* radioModeName(RadioMode mode) {
	switch (mode) {
	case RadioMode::Off:
		return "off";
	case RadioMode::Ap:
		return "ap";
	case RadioMode::Sta:
		return "sta";
	case RadioMode::ApSta:
		return "ap-sta";
	}
	return "off";
}

bool radioModeParse(const char* text, RadioMode& mode) {
	if (!text) {
		return false;
	}
	if (std::strcmp(text, "off") == 0) {
		mode = RadioMode::Off;
		return true;
	}
	if (std::strcmp(text, "ap") == 0) {
		mode = RadioMode::Ap;
		return true;
	}
	if (std::strcmp(text, "sta") == 0) {
		mode = RadioMode::Sta;
		return true;
	}
	if (std::strcmp(text, "ap-sta") == 0) {
		mode = RadioMode::ApSta;
		return true;
	}
	return false;
}

bool radioMerge(Radio& radio, const std::string& json, std::string& err) {
	json::Value root;
	if (!json::parse(json.c_str(), root, err)) {
		return false;
	}
	const json::Value* sections = root.find("sections");
	if (!sections) {
		err = "missing sections";
		return false;
	}
	const json::Value* host = sections->find("host.radio");
	if (!host) {
		err = "missing host.radio";
		return false;
	}
	const json::Value* values = host->find("values");
	const json::Value* src = values ? values : host;
	if (const json::Value* mode = src->find("mode")) {
		if (mode->type != json::Value::Type::String ||
			!radioModeParse(mode->s.c_str(), radio.mode)) {
			err = "mode must be off, ap, sta, or ap-sta";
			return false;
		}
	}
	if (const json::Value* ap = src->find("apSsid")) {
		if (ap->type != json::Value::Type::String ||
			!copyText(radio.apSsid, sizeof radio.apSsid, ap->s, "apSsid is too long", err)) {
			if (err.empty()) {
				err = "apSsid must be a string";
			}
			return false;
		}
	}
	if (const json::Value* sta = src->find("staSsid")) {
		if (sta->type != json::Value::Type::String ||
			!copyText(radio.staSsid, sizeof radio.staSsid, sta->s, "staSsid is too long", err)) {
			if (err.empty()) {
				err = "staSsid must be a string";
			}
			return false;
		}
	}
	if (const json::Value* channel = src->find("channel")) {
		if (channel->type != json::Value::Type::Number) {
			err = "channel must be 1-14";
			return false;
		}
		const int n = static_cast<int>(std::lround(channel->n));
		if (std::fabs(channel->n - static_cast<double>(n)) > 1e-6 || n < 1 || n > 14) {
			err = "channel must be 1-14";
			return false;
		}
		radio.channel = n;
	}
	err.clear();
	return true;
}

std::string radioWrite(const Radio& radio) {
	json::Value values = json::Value::obj();
	values.set("mode", json::Value::str(radioModeName(radio.mode)));
	values.set("apSsid", json::Value::str(radio.apSsid));
	values.set("staSsid", json::Value::str(radio.staSsid));
	values.set("channel", json::Value::number(radio.channel));
	json::Value host = json::Value::obj();
	host.set("host.radio", std::move(values));
	json::Value sections = json::Value::obj();
	sections.set("sections", std::move(host));
	return sections.dump();
}

std::string prefsResponse(const Radio& radio) {
	json::Value mode = field("mode", "Mode", "enum");
	json::Value options = json::Value::arr();
	options.a.push_back(json::Value::str("off"));
	options.a.push_back(json::Value::str("ap"));
	options.a.push_back(json::Value::str("sta"));
	options.a.push_back(json::Value::str("ap-sta"));
	mode.set("options", std::move(options));
	json::Value channel = field("channel", "Channel", "int");
	channel.set("min", json::Value::number(1));
	channel.set("max", json::Value::number(14));
	json::Value fields = json::Value::arr();
	fields.a.push_back(std::move(mode));
	fields.a.push_back(field("apSsid", "AP name", "string"));
	fields.a.push_back(field("staSsid", "Network", "string"));
	fields.a.push_back(std::move(channel));

	json::Value values = json::Value::obj();
	values.set("mode", json::Value::str(radioModeName(radio.mode)));
	values.set("apSsid", json::Value::str(radio.apSsid));
	values.set("staSsid", json::Value::str(radio.staSsid));
	values.set("channel", json::Value::number(radio.channel));

	json::Value section = json::Value::obj();
	section.set("label", json::Value::str("Radio"));
	section.set("fields", std::move(fields));
	section.set("values", std::move(values));
	json::Value sections = json::Value::obj();
	sections.set("host.radio", std::move(section));
	json::Value root = json::Value::obj();
	root.set("sections", std::move(sections));
	return root.dump();
}

} // namespace rigkit::espapp
