// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_layouts.h"

#include <QtCore/QJsonObject>

#include "ayu/features/miogram/miogram_config.h"
#include "ayu/features/miogram/miogram_divine.h"

namespace Miogram {

LayoutController &LayoutController::instance() {
	static LayoutController controller;
	return controller;
}

LayoutController::LayoutController() {
}

void LayoutController::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
	applyToUiSettings();
}

LayoutMode LayoutController::currentMode() const {
	return _mode;
}

void LayoutController::setMode(LayoutMode mode) {
	_mode = mode;
	save();
	applyToUiSettings();
	const auto preset = [&] {
		switch (mode) {
		case LayoutMode::Discord: return DivinePreset::DiscordUltra;
		case LayoutMode::Ios: return DivinePreset::IosGlass;
		case LayoutMode::MinimalRail: return DivinePreset::Minimalist;
		case LayoutMode::Ame:
		case LayoutMode::Classic:
		default: return DivinePreset::ClassicTG;
		}
	}();
	DivineEngine::instance().setPreset(preset);
}

DiscordLayoutState LayoutController::discordState() const {
	return _discord;
}

void LayoutController::setDiscordState(const DiscordLayoutState &state) {
	_discord = state;
	save();
}

IosLayoutState LayoutController::iosState() const {
	return _ios;
}

void LayoutController::setIosState(const IosLayoutState &state) {
	_ios = state;
	save();
}

AmePalette LayoutController::amePalette() const {
	return _ame;
}

bool LayoutController::isCompactRail() const {
	return _mode == LayoutMode::MinimalRail || _mode == LayoutMode::Discord;
}

int LayoutController::effectiveRailWidth() const {
	if (_mode == LayoutMode::Discord) {
		return _discord.railWidth;
	}
	if (_mode == LayoutMode::MinimalRail) {
		return MinimalRail::instance().railWidth();
	}
	return 0;
}

QString LayoutController::modeTitle(LayoutMode mode) const {
	switch (mode) {
	case LayoutMode::Classic: return u"Classic Telegram"_q;
	case LayoutMode::Discord: return u"Discord Rail"_q;
	case LayoutMode::Ios: return u"iOS Cupertino"_q;
	case LayoutMode::MinimalRail: return u"Minimalist Rail"_q;
	case LayoutMode::Ame: return u"Ame-chan Cyberpastel"_q;
	}
	return u"Classic"_q;
}

std::vector<LayoutMode> LayoutController::allModes() const {
	return {
		LayoutMode::Classic,
		LayoutMode::Discord,
		LayoutMode::Ios,
		LayoutMode::MinimalRail,
		LayoutMode::Ame,
	};
}

void LayoutController::applyToUiSettings() {
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	obj.insert(u"mode"_q, static_cast<int>(_mode));
	MiogramConfig::instance().setDomain(ConfigDomain::Layouts, obj);
}

void LayoutController::load() {
	const auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	_mode = static_cast<LayoutMode>(obj.value(u"mode"_q).toInt(0));
	_discord.railWidth = obj.value(u"discordRail"_q).toInt(72);
	_ios.cornerRadius = obj.value(u"iosRadius"_q).toInt(18);
	_ios.blurHeader = obj.value(u"iosBlur"_q).toBool(true);
}

void LayoutController::save() {
	QJsonObject obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	obj.insert(u"mode"_q, static_cast<int>(_mode));
	obj.insert(u"discordRail"_q, _discord.railWidth);
	obj.insert(u"iosRadius"_q, _ios.cornerRadius);
	obj.insert(u"iosBlur"_q, _ios.blurHeader);
	MiogramConfig::instance().setDomain(ConfigDomain::Layouts, obj);
}

MinimalRail &MinimalRail::instance() {
	static MinimalRail rail;
	return rail;
}

MinimalRail::MinimalRail() {
}

void MinimalRail::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	_width = obj.value(u"minimalRail"_q).toInt(56);
	_labels = obj.value(u"minimalLabels"_q).toBool(false);
}

bool MinimalRail::isEnabled() const {
	return LayoutController::instance().currentMode() == LayoutMode::MinimalRail;
}

int MinimalRail::railWidth() const {
	return _width;
}

void MinimalRail::setRailWidth(int width) {
	_width = std::clamp(width, 48, 96);
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	obj.insert(u"minimalRail"_q, _width);
	MiogramConfig::instance().setDomain(ConfigDomain::Layouts, obj);
}

bool MinimalRail::showLabels() const {
	return _labels;
}

void MinimalRail::setShowLabels(bool show) {
	_labels = show;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Layouts);
	obj.insert(u"minimalLabels"_q, show);
	MiogramConfig::instance().setDomain(ConfigDomain::Layouts, obj);
}

} // namespace Miogram
