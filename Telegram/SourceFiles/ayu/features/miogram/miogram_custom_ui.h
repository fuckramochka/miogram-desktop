// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtCore/QPointF>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

enum class AvatarShape {
	Circle = 0,
	Squircle = 1,
	Hexagon = 2,
	Star = 3,
	Rounded = 4,
};

enum class BubbleStyle {
	Classic = 0,
	Glass = 1,
	Gradient = 2,
	Neon = 3,
	Minimal = 4,
};

struct CustomUiSettings {
	BubbleStyle bubbleStyle = BubbleStyle::Classic;
	AvatarShape avatarShape = AvatarShape::Circle;
	bool avatarGlow = false;
	bool textGradient = false;
	bool profileBanner = false;
	QColor bubbleColorFrom = QColor(255, 255, 255);
	QColor bubbleColorTo = QColor(240, 240, 245);
	QColor glowColor = QColor(255, 105, 180);
	QColor bannerFrom = QColor(255, 105, 180);
	QColor bannerTo = QColor(88, 101, 242);
	int bubbleRadius = 16;
	int avatarCorners = 23;
	double glassBlur = 18.;
	double glassOpacity = 0.55;
};

class CustomUiPrefs {
public:
	static CustomUiPrefs &instance();

	void initialize();

	[[nodiscard]] CustomUiSettings settings() const;
	void setSettings(const CustomUiSettings &settings);

	[[nodiscard]] BubbleStyle bubbleStyle() const;
	void setBubbleStyle(BubbleStyle style);

	[[nodiscard]] AvatarShape avatarShape() const;
	void setAvatarShape(AvatarShape shape);

	[[nodiscard]] int bubbleRadius() const;
	void setBubbleRadius(int radius);

	[[nodiscard]] int avatarCorners() const;
	void setAvatarCorners(int corners);

	void resetToDefaults();

private:
	CustomUiPrefs();
	void load();
	void save();

	CustomUiSettings _settings;
	bool _initialized = false;
};

class UiEngine {
public:
	static UiEngine &instance();

	void initialize();

	[[nodiscard]] QColor bubbleColorAt(
		const CustomUiSettings &settings,
		double t,
		bool outgoing) const;
	[[nodiscard]] QColor nameColorFor(
		const QString &name,
		bool outgoing) const;
	[[nodiscard]] std::vector<QPointF> avatarPolygon(
		AvatarShape shape,
		double size) const;
	[[nodiscard]] bool shouldDrawGlow(const CustomUiSettings &settings) const;
	[[nodiscard]] double glassStrokeAlpha(
		const CustomUiSettings &settings,
		bool hovered) const;
};

class HapticEngine {
public:
	static HapticEngine &instance();

	void initialize();
	void tick();
	void confirm();
	void error();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

private:
	HapticEngine();

	bool _enabled = true;
	bool _initialized = false;
};

class Glassmorphism {
public:
	static Glassmorphism &instance();

	void initialize();

	[[nodiscard]] bool isSupported() const;
	[[nodiscard]] double blurRadius() const;
	void setBlurRadius(double radius);

	[[nodiscard]] double backgroundOpacity() const;
	void setBackgroundOpacity(double opacity);

private:
	Glassmorphism();

	double _blur = 18.;
	double _opacity = 0.55;
	bool _initialized = false;
};

class PhysicsInterpolator {
public:
	[[nodiscard]] static double spring(double t);
	[[nodiscard]] static double easeOutBack(double t);
	[[nodiscard]] static double easeOutCubic(double t);
	[[nodiscard]] static double lerp(double a, double b, double t);
};

} // namespace Miogram
