// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_divine.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include "ayu/ayu_ui_settings.h"
#include "core/application.h"
#include "ui/chat/chat_style_radius.h"

namespace Miogram {

namespace {

QString DivineSettingsPath() {
	return cWorkingDir() + u"tdata/mio_divine.json"_q;
}

void RepaintAllWidgets() {
	for (QWidget *w : QApplication::allWidgets()) {
		w->update();
	}
}

} // namespace

DivineEngine &DivineEngine::instance() {
	static DivineEngine engine;
	return engine;
}

DivineEngine::DivineEngine() {
}

void DivineEngine::initialize() {
	if (_initialized) return;
	_initialized = true;

	loadSettings();
	applyCurrentPreset();
}

DivinePreset DivineEngine::currentPreset() const {
	return _current;
}

void DivineEngine::setPreset(DivinePreset preset) {
	_current = preset;
	saveSettings();
	applyCurrentPreset();
}

QString DivineEngine::presetTitle(DivinePreset preset) const {
	switch (preset) {
	case DivinePreset::ClassicTG:
		return QStringLiteral("Classic TG (Ame-Chan ໒꒱)");
	case DivinePreset::DiscordUltra:
		return QStringLiteral("Discord Ultra (Dark Rail & Squircle)");
	case DivinePreset::IosGlass:
		return QStringLiteral("iOS Glassmorphism (Apple Cupertino)");
	case DivinePreset::Minimalist:
		return QStringLiteral("Minimalist (Speed & Screen Focus)");
	case DivinePreset::WindowsXP:
		return QStringLiteral("Windows XP (Luna Blue Nostalgia)");
	}
	return QStringLiteral("Classic TG");
}

int DivineEngine::bubbleRadius(DivinePreset preset) const {
	switch (preset) {
	case DivinePreset::ClassicTG: return 16;
	case DivinePreset::DiscordUltra: return 4;
	case DivinePreset::IosGlass: return 18;
	case DivinePreset::Minimalist: return 6;
	case DivinePreset::WindowsXP: return 2;
	}
	return 16;
}

float DivineEngine::sidebarWidthMultiplier(DivinePreset preset) const {
	switch (preset) {
	case DivinePreset::ClassicTG: return 1.0f;
	case DivinePreset::DiscordUltra: return 0.85f;
	case DivinePreset::IosGlass: return 1.1f;
	case DivinePreset::Minimalist: return 0.75f;
	case DivinePreset::WindowsXP: return 1.0f;
	}
	return 1.0f;
}

bool DivineEngine::isCompactRail(DivinePreset preset) const {
	return (preset == DivinePreset::Minimalist || preset == DivinePreset::DiscordUltra);
}

QColor DivineEngine::accentColor(DivinePreset preset) const {
	switch (preset) {
	case DivinePreset::ClassicTG: return QColor(255, 105, 180); // Pink Ame
	case DivinePreset::DiscordUltra: return QColor(88, 101, 242); // Blurple
	case DivinePreset::IosGlass: return QColor(0, 122, 255); // Cupertino Blue
	case DivinePreset::Minimalist: return QColor(140, 140, 140); // Monochrome
	case DivinePreset::WindowsXP: return QColor(0, 85, 230); // Luna Blue
	}
	return QColor(255, 105, 180);
}

std::vector<DivinePreset> DivineEngine::allPresets() const {
	return {
		DivinePreset::ClassicTG,
		DivinePreset::DiscordUltra,
		DivinePreset::IosGlass,
		DivinePreset::Minimalist,
		DivinePreset::WindowsXP,
	};
}

void DivineEngine::applyCurrentPreset() {
	const auto radius = bubbleRadius(_current);
	Ui::SetAppliedBubbleRadius(radius);

	switch (_current) {
	case DivinePreset::ClassicTG:
		AyuUiSettings::setAvatarCorners(23);
		AyuUiSettings::setWideMultiplier(1.0);
		break;
	case DivinePreset::DiscordUltra:
		AyuUiSettings::setAvatarCorners(12); // Squircle
		AyuUiSettings::setWideMultiplier(0.85);
		break;
	case DivinePreset::IosGlass:
		AyuUiSettings::setAvatarCorners(23);
		AyuUiSettings::setWideMultiplier(1.1);
		break;
	case DivinePreset::Minimalist:
		AyuUiSettings::setAvatarCorners(6);
		AyuUiSettings::setWideMultiplier(0.75);
		break;
	case DivinePreset::WindowsXP:
		AyuUiSettings::setAvatarCorners(0); // Sharp corners
		AyuUiSettings::setWideMultiplier(1.0);
		break;
	}

	RepaintAllWidgets();
}

void DivineEngine::loadSettings() {
	QFile f(DivineSettingsPath());
	if (!f.open(QIODevice::ReadOnly)) return;

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return;

	const auto obj = doc.object();
	const int p = obj.value(u"preset"_q).toInt(0);
	if (p >= 0 && p <= static_cast<int>(DivinePreset::WindowsXP)) {
		_current = static_cast<DivinePreset>(p);
	}
}

void DivineEngine::saveSettings() {
	QJsonObject obj;
	obj[u"preset"_q] = static_cast<int>(_current);

	QFile f(DivineSettingsPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
