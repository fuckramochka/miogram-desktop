// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

enum class LayoutMode {
	Classic = 0,
	Discord = 1,
	Ios = 2,
	MinimalRail = 3,
	Ame = 4,
};

struct DiscordLayoutState {
	bool railVisible = true;
	int railWidth = 72;
	bool channelHeaderVisible = true;
	bool memberListVisible = false;
	QColor blurple = QColor(88, 101, 242);
	QColor railBackground = QColor(32, 34, 37);
};

struct IosLayoutState {
	bool largeTitle = true;
	bool blurHeader = true;
	double headerCollapse = 0.;
	QColor systemBlue = QColor(0, 122, 255);
	int cornerRadius = 18;
};

struct AmePalette {
	QColor neonPink = QColor(255, 42, 147);
	QColor cyan = QColor(0, 240, 255);
	QColor lavender = QColor(180, 140, 255);
	QColor halo = QColor(255, 220, 245);
};

class LayoutController {
public:
	static LayoutController &instance();

	void initialize();

	[[nodiscard]] LayoutMode currentMode() const;
	void setMode(LayoutMode mode);

	[[nodiscard]] DiscordLayoutState discordState() const;
	void setDiscordState(const DiscordLayoutState &state);

	[[nodiscard]] IosLayoutState iosState() const;
	void setIosState(const IosLayoutState &state);

	[[nodiscard]] AmePalette amePalette() const;

	[[nodiscard]] bool isCompactRail() const;
	[[nodiscard]] int effectiveRailWidth() const;
	[[nodiscard]] QString modeTitle(LayoutMode mode) const;
	[[nodiscard]] std::vector<LayoutMode> allModes() const;

	void applyToUiSettings();

private:
	LayoutController();
	void load();
	void save();

	LayoutMode _mode = LayoutMode::Classic;
	DiscordLayoutState _discord;
	IosLayoutState _ios;
	AmePalette _ame;
	bool _initialized = false;
};

class MinimalRail {
public:
	static MinimalRail &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	[[nodiscard]] int railWidth() const;
	void setRailWidth(int width);

	[[nodiscard]] bool showLabels() const;
	void setShowLabels(bool show);

private:
	MinimalRail();

	int _width = 56;
	bool _labels = false;
	bool _initialized = false;
};

} // namespace Miogram
