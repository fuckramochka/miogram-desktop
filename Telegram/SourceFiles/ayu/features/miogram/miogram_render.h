// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include "base/basic_types.h"

namespace Miogram {

struct BubblePalette {
	QColor incomingFrom;
	QColor incomingTo;
	QColor outgoingFrom;
	QColor outgoingTo;
	int radius = 16;
	bool glass = false;
};

struct AvatarGlowSpec {
	bool enabled = false;
	QColor color;
	int spread = 6;
};

struct DialogCardSpec {
	bool enabled = false;
	QColor background;
	int radius = 12;
};

struct ProfileBannerSpec {
	bool enabled = false;
	QColor from;
	QColor to;
};

class MiogramRender {
public:
	static MiogramRender &instance();

	void initialize();
	void applyToUiSettings();

	[[nodiscard]] BubblePalette bubblePalette(bool outgoing) const;
	[[nodiscard]] QColor bubbleColorAt(bool outgoing, double t) const;
	[[nodiscard]] QColor nameColor(const QString &name, bool outgoing) const;
	[[nodiscard]] AvatarGlowSpec avatarGlow() const;
	[[nodiscard]] DialogCardSpec dialogCard() const;
	[[nodiscard]] ProfileBannerSpec profileBanner() const;
	[[nodiscard]] int avatarCorners() const;
	[[nodiscard]] int bubbleRadius() const;

private:
	MiogramRender();

	bool _initialized = false;
};

} // namespace Miogram
