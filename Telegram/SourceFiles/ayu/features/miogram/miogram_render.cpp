// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_render.h"

#include <algorithm>

#include "ayu/features/miogram/miogram_custom_ui.h"
#include "ayu/features/miogram/miogram_divine.h"
#include "ayu/features/miogram/miogram_layouts.h"
#include "ui/chat/chat_style_radius.h"

namespace Miogram {

MiogramRender &MiogramRender::instance() {
	static MiogramRender render;
	return render;
}

MiogramRender::MiogramRender() {
}

void MiogramRender::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	applyToUiSettings();
}

void MiogramRender::applyToUiSettings() {
	const auto radius = bubbleRadius();
	Ui::SetAppliedBubbleRadius(radius);
	LayoutController::instance().applyToUiSettings();
}

BubblePalette MiogramRender::bubblePalette(bool outgoing) const {
	const auto settings = CustomUiPrefs::instance().settings();
	const auto preset = DivineEngine::instance().currentPreset();
	BubblePalette palette;
	palette.radius = bubbleRadius();
	palette.glass = (settings.bubbleStyle == BubbleStyle::Glass);
	if (outgoing) {
		switch (preset) {
		case DivinePreset::DiscordUltra:
			palette.outgoingFrom = QColor(88, 101, 242);
			palette.outgoingTo = QColor(71, 82, 196);
			break;
		case DivinePreset::IosGlass:
			palette.outgoingFrom = QColor(0, 122, 255);
			palette.outgoingTo = QColor(10, 132, 255);
			break;
		case DivinePreset::Minimalist:
			palette.outgoingFrom = QColor(240, 240, 242);
			palette.outgoingTo = QColor(240, 240, 242);
			break;
		case DivinePreset::WindowsXP:
			palette.outgoingFrom = QColor(220, 235, 252);
			palette.outgoingTo = QColor(190, 215, 245);
			break;
		case DivinePreset::ClassicTG:
		default:
			palette.outgoingFrom = settings.bubbleColorFrom;
			palette.outgoingTo = settings.bubbleColorTo;
			break;
		}
		palette.incomingFrom = QColor(255, 255, 255);
		palette.incomingTo = QColor(255, 255, 255);
	} else {
		palette.incomingFrom = QColor(255, 255, 255);
		palette.incomingTo = QColor(245, 245, 248);
		palette.outgoingFrom = settings.bubbleColorFrom;
		palette.outgoingTo = settings.bubbleColorTo;
	}
	return palette;
}

QColor MiogramRender::bubbleColorAt(bool outgoing, double t) const {
	const auto palette = bubblePalette(outgoing);
	const auto from = outgoing ? palette.outgoingFrom : palette.incomingFrom;
	const auto to = outgoing ? palette.outgoingTo : palette.incomingTo;
	const auto clamped = std::clamp(t, 0., 1.);
	return QColor(
		int(from.red() + (to.red() - from.red()) * clamped),
		int(from.green() + (to.green() - from.green()) * clamped),
		int(from.blue() + (to.blue() - from.blue()) * clamped));
}

QColor MiogramRender::nameColor(const QString &name, bool outgoing) const {
	return UiEngine::instance().nameColorFor(name, outgoing);
}

AvatarGlowSpec MiogramRender::avatarGlow() const {
	const auto settings = CustomUiPrefs::instance().settings();
	AvatarGlowSpec spec;
	spec.enabled = UiEngine::instance().shouldDrawGlow(settings);
	spec.color = settings.glowColor;
	return spec;
}

DialogCardSpec MiogramRender::dialogCard() const {
	const auto mode = LayoutController::instance().currentMode();
	DialogCardSpec spec;
	spec.enabled = (mode == LayoutMode::Ios);
	spec.background = QColor(255, 255, 255);
	spec.radius = LayoutController::instance().iosState().cornerRadius;
	return spec;
}

ProfileBannerSpec MiogramRender::profileBanner() const {
	const auto settings = CustomUiPrefs::instance().settings();
	ProfileBannerSpec spec;
	spec.enabled = settings.profileBanner;
	spec.from = settings.bannerFrom;
	spec.to = settings.bannerTo;
	return spec;
}

int MiogramRender::avatarCorners() const {
	return CustomUiPrefs::instance().avatarCorners();
}

int MiogramRender::bubbleRadius() const {
	const auto preset = DivineEngine::instance().currentPreset();
	const auto custom = CustomUiPrefs::instance().bubbleRadius();
	if (preset == DivinePreset::ClassicTG) {
		return custom;
	}
	return DivineEngine::instance().bubbleRadius(preset);
}

} // namespace Miogram
