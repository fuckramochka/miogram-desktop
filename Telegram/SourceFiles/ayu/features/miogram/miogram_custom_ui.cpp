// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_custom_ui.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QColor>
#include <cmath>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

CustomUiPrefs &CustomUiPrefs::instance() {
	static CustomUiPrefs prefs;
	return prefs;
}

CustomUiPrefs::CustomUiPrefs() {
}

void CustomUiPrefs::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

CustomUiSettings CustomUiPrefs::settings() const {
	return _settings;
}

void CustomUiPrefs::setSettings(const CustomUiSettings &settings) {
	_settings = settings;
	save();
}

BubbleStyle CustomUiPrefs::bubbleStyle() const {
	return _settings.bubbleStyle;
}

void CustomUiPrefs::setBubbleStyle(BubbleStyle style) {
	_settings.bubbleStyle = style;
	save();
}

AvatarShape CustomUiPrefs::avatarShape() const {
	return _settings.avatarShape;
}

void CustomUiPrefs::setAvatarShape(AvatarShape shape) {
	_settings.avatarShape = shape;
	save();
}

int CustomUiPrefs::bubbleRadius() const {
	return _settings.bubbleRadius;
}

void CustomUiPrefs::setBubbleRadius(int radius) {
	_settings.bubbleRadius = std::clamp(radius, 0, 24);
	save();
}

int CustomUiPrefs::avatarCorners() const {
	return _settings.avatarCorners;
}

void CustomUiPrefs::setAvatarCorners(int corners) {
	_settings.avatarCorners = std::clamp(corners, 0, 32);
	save();
}

void CustomUiPrefs::resetToDefaults() {
	_settings = CustomUiSettings{};
	save();
}

void CustomUiPrefs::load() {
	const auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	_settings.bubbleStyle = static_cast<BubbleStyle>(obj.value(u"bubbleStyle"_q).toInt(0));
	_settings.avatarShape = static_cast<AvatarShape>(obj.value(u"avatarShape"_q).toInt(0));
	_settings.avatarGlow = obj.value(u"avatarGlow"_q).toBool(false);
	_settings.textGradient = obj.value(u"textGradient"_q).toBool(false);
	_settings.profileBanner = obj.value(u"profileBanner"_q).toBool(false);
	_settings.bubbleRadius = obj.value(u"bubbleRadius"_q).toInt(16);
	_settings.avatarCorners = obj.value(u"avatarCorners"_q).toInt(23);
	_settings.glassBlur = obj.value(u"glassBlur"_q).toDouble(18.);
	_settings.glassOpacity = obj.value(u"glassOpacity"_q).toDouble(0.55);
	if (obj.contains(u"bubbleFrom"_q)) {
		_settings.bubbleColorFrom = QColor(obj.value(u"bubbleFrom"_q).toString());
	}
	if (obj.contains(u"bubbleTo"_q)) {
		_settings.bubbleColorTo = QColor(obj.value(u"bubbleTo"_q).toString());
	}
	if (obj.contains(u"glow"_q)) {
		_settings.glowColor = QColor(obj.value(u"glow"_q).toString());
	}
}

void CustomUiPrefs::save() {
	QJsonObject obj;
	obj.insert(u"bubbleStyle"_q, static_cast<int>(_settings.bubbleStyle));
	obj.insert(u"avatarShape"_q, static_cast<int>(_settings.avatarShape));
	obj.insert(u"avatarGlow"_q, _settings.avatarGlow);
	obj.insert(u"textGradient"_q, _settings.textGradient);
	obj.insert(u"profileBanner"_q, _settings.profileBanner);
	obj.insert(u"bubbleRadius"_q, _settings.bubbleRadius);
	obj.insert(u"avatarCorners"_q, _settings.avatarCorners);
	obj.insert(u"glassBlur"_q, _settings.glassBlur);
	obj.insert(u"glassOpacity"_q, _settings.glassOpacity);
	obj.insert(u"bubbleFrom"_q, _settings.bubbleColorFrom.name());
	obj.insert(u"bubbleTo"_q, _settings.bubbleColorTo.name());
	obj.insert(u"glow"_q, _settings.glowColor.name());
	MiogramConfig::instance().setDomain(ConfigDomain::Visuals, obj);
}

UiEngine &UiEngine::instance() {
	static UiEngine engine;
	return engine;
}

void UiEngine::initialize() {
	CustomUiPrefs::instance().initialize();
}

QColor UiEngine::bubbleColorAt(
		const CustomUiSettings &settings,
		double t,
		bool outgoing) const {
	const auto clamped = std::clamp(t, 0., 1.);
	auto mix = [&](const QColor &a, const QColor &b) {
		return QColor(
			int(a.red() + (b.red() - a.red()) * clamped),
			int(a.green() + (b.green() - a.green()) * clamped),
			int(a.blue() + (b.blue() - a.blue()) * clamped));
	};
	switch (settings.bubbleStyle) {
	case BubbleStyle::Classic:
		return outgoing ? QColor(239, 253, 222) : QColor(255, 255, 255);
	case BubbleStyle::Glass: {
		const auto base = mix(settings.bubbleColorFrom, settings.bubbleColorTo);
		return QColor(base.red(), base.green(), base.blue(), 200);
	}
	case BubbleStyle::Gradient:
		return mix(settings.bubbleColorFrom, settings.bubbleColorTo);
	case BubbleStyle::Neon: {
		const auto base = mix(settings.bubbleColorFrom, settings.glowColor);
		return base.lighter(115);
	}
	case BubbleStyle::Minimal:
		return outgoing ? QColor(240, 240, 242) : QColor(255, 255, 255);
	}
	return QColor(255, 255, 255);
}

QColor UiEngine::nameColorFor(const QString &name, bool outgoing) const {
	if (name.isEmpty()) {
		return outgoing ? QColor(70, 140, 60) : QColor(80, 120, 220);
	}
	uint hash = 0;
	for (const auto ch : name) {
		hash = hash * 31 + uint(ch.unicode());
	}
	static const QColor palette[] = {
		QColor(220, 80, 120),
		QColor(120, 90, 220),
		QColor(60, 150, 220),
		QColor(50, 170, 140),
		QColor(220, 140, 50),
		QColor(200, 90, 200),
	};
	return palette[hash % 6];
}

std::vector<QPointF> UiEngine::avatarPolygon(AvatarShape shape, double size) const {
	std::vector<QPointF> points;
	const auto half = size / 2.;
	constexpr double kPi = 3.141592653589793;
	if (shape == AvatarShape::Circle || shape == AvatarShape::Rounded) {
		for (int i = 0; i < 32; ++i) {
			const auto a = 2. * kPi * i / 32.;
			points.emplace_back(half + half * std::cos(a), half + half * std::sin(a));
		}
		return points;
	}
	if (shape == AvatarShape::Hexagon) {
		for (int i = 0; i < 6; ++i) {
			const auto a = 2. * kPi * i / 6. - kPi / 2.;
			points.emplace_back(half + half * std::cos(a), half + half * std::sin(a));
		}
		return points;
	}
	if (shape == AvatarShape::Star) {
		for (int i = 0; i < 10; ++i) {
			const auto r = (i % 2 == 0) ? half : half * 0.45;
			const auto a = 2. * kPi * i / 10. - kPi / 2.;
			points.emplace_back(half + r * std::cos(a), half + r * std::sin(a));
		}
		return points;
	}
	const auto corner = size * 0.28;
	points.emplace_back(corner, 0);
	points.emplace_back(size - corner, 0);
	points.emplace_back(size, corner);
	points.emplace_back(size, size - corner);
	points.emplace_back(size - corner, size);
	points.emplace_back(corner, size);
	points.emplace_back(0, size - corner);
	points.emplace_back(0, corner);
	return points;
}

bool UiEngine::shouldDrawGlow(const CustomUiSettings &settings) const {
	return settings.avatarGlow && settings.bubbleStyle != BubbleStyle::Minimal;
}

double UiEngine::glassStrokeAlpha(const CustomUiSettings &settings, bool hovered) const {
	const auto base = settings.glassOpacity;
	return hovered ? std::min(1., base + 0.25) : base;
}

HapticEngine &HapticEngine::instance() {
	static HapticEngine engine;
	return engine;
}

HapticEngine::HapticEngine() {
}

void HapticEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_enabled = MiogramConfig::instance().boolValue(
		ConfigDomain::Visuals,
		u"haptic"_q,
		true);
}

void HapticEngine::tick() {
}

void HapticEngine::confirm() {
}

void HapticEngine::error() {
}

bool HapticEngine::isEnabled() const {
	return _enabled;
}

void HapticEngine::setEnabled(bool enabled) {
	_enabled = enabled;
	MiogramConfig::instance().setBoolValue(ConfigDomain::Visuals, u"haptic"_q, enabled);
}

Glassmorphism &Glassmorphism::instance() {
	static Glassmorphism glass;
	return glass;
}

Glassmorphism::Glassmorphism() {
}

void Glassmorphism::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	_blur = obj.value(u"glassBlur"_q).toDouble(18.);
	_opacity = obj.value(u"glassOpacity"_q).toDouble(0.55);
}

bool Glassmorphism::isSupported() const {
	return true;
}

double Glassmorphism::blurRadius() const {
	return _blur;
}

void Glassmorphism::setBlurRadius(double radius) {
	_blur = std::clamp(radius, 0., 60.);
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	obj.insert(u"glassBlur"_q, _blur);
	MiogramConfig::instance().setDomain(ConfigDomain::Visuals, obj);
}

double Glassmorphism::backgroundOpacity() const {
	return _opacity;
}

void Glassmorphism::setBackgroundOpacity(double opacity) {
	_opacity = std::clamp(opacity, 0., 1.);
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	obj.insert(u"glassOpacity"_q, _opacity);
	MiogramConfig::instance().setDomain(ConfigDomain::Visuals, obj);
}

double PhysicsInterpolator::spring(double t) {
	const auto clamped = std::clamp(t, 0., 1.);
	return 1. - std::exp(-6. * clamped) * std::cos(10. * clamped);
}

double PhysicsInterpolator::easeOutBack(double t) {
	const auto clamped = std::clamp(t, 0., 1.);
	constexpr double c1 = 1.70158;
	constexpr double c3 = c1 + 1.;
	return 1. + c3 * std::pow(clamped - 1., 3.) + c1 * std::pow(clamped - 1., 2.);
}

double PhysicsInterpolator::easeOutCubic(double t) {
	const auto clamped = std::clamp(t, 0., 1.);
	return 1. - std::pow(1. - clamped, 3.);
}

double PhysicsInterpolator::lerp(double a, double b, double t) {
	return a + (b - a) * std::clamp(t, 0., 1.);
}

} // namespace Miogram
