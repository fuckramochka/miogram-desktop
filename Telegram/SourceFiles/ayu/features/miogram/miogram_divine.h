// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <vector>

namespace Miogram {

enum class DivinePreset {
	ClassicTG = 0,
	DiscordUltra = 1,
	IosGlass = 2,
	Minimalist = 3,
	WindowsXP = 4,
};

class DivineEngine {
public:
	static DivineEngine &instance();

	void initialize();
	[[nodiscard]] DivinePreset currentPreset() const;
	void setPreset(DivinePreset preset);

	[[nodiscard]] QString presetTitle(DivinePreset preset) const;
	[[nodiscard]] int bubbleRadius(DivinePreset preset) const;
	[[nodiscard]] float sidebarWidthMultiplier(DivinePreset preset) const;
	[[nodiscard]] bool isCompactRail(DivinePreset preset) const;
	[[nodiscard]] QColor accentColor(DivinePreset preset) const;
	[[nodiscard]] std::vector<DivinePreset> allPresets() const;

	void applyCurrentPreset();

private:
	DivineEngine();
	void loadSettings();
	void saveSettings();

	DivinePreset _current = DivinePreset::ClassicTG;
	bool _initialized = false;
};

} // namespace Miogram
