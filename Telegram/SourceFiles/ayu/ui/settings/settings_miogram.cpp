// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_miogram.h"

#include "lang_auto.h"
#include "ayu/features/miogram/miogram_feed.h"
#include "ayu/features/miogram/miogram_folders_multichat.h"
#include "ayu/features/miogram/miogram_fun_box.h"
#include "ayu/features/miogram/miogram_kanban.h"
#include "ayu/features/miogram/miogram_locale_fun.h"
#include "ayu/features/miogram/miogram_player.h"
#include "ayu/features/miogram/miogram_plugins.h"
#include "ayu/features/miogram/miogram_updater.h"
#include "ayu/features/miogram/miogram_userbot_performance.h"
#include "ayu/features/miogram/miogram_vault.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "boxes/abstract_box.h"
#include "settings/settings_builder.h"
#include "ui/boxes/confirm_box.h"
#include "window/window_session_controller.h"

#include <QDesktopServices>
#include <QtCore/QUrl>

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

void BuildMiogramSections(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::mio_SectionPlayer());
	ayu.addToggle({
		.id = u"miogram/player/shuffle"_q,
		.title = tr::mio_PlayerShuffle(),
		.getter = [] { return Miogram::PlayerPrefs::instance().shuffle(); },
		.setter = [](bool v) { Miogram::PlayerPrefs::instance().setShuffle(v); },
	});
	ayu.addToggle({
		.id = u"miogram/feed/enabled"_q,
		.title = tr::mio_FeedEnabled(),
		.getter = [] { return Miogram::SmartFeedService::instance().isEnabled(); },
		.setter = [](bool v) { Miogram::SmartFeedService::instance().setEnabled(v); },
	});
	ayu.addToggle({
		.id = u"miogram/folders/floating"_q,
		.title = tr::mio_FloatingChats(),
		.getter = [] { return Miogram::FloatingChatState::instance().isEnabled(); },
		.setter = [](bool v) { Miogram::FloatingChatState::instance().setEnabled(v); },
	});
	ayu.addToggle({
		.id = u"miogram/plugins/notifications"_q,
		.title = tr::mio_InAppNotifications(),
		.getter = [] { return Miogram::InAppNotifications::instance().isEnabled(); },
		.setter = [](bool v) { Miogram::InAppNotifications::instance().setEnabled(v); },
	});
	ayu.addToggle({
		.id = u"miogram/privacy/double-bottom"_q,
		.title = tr::mio_DoubleBottom(),
		.getter = [] { return Miogram::DoubleBottomManager::instance().isDoubleBottomEnabled(); },
		.setter = [](bool v) { Miogram::DoubleBottomManager::instance().setDoubleBottomEnabled(v); },
	});
	ayu.addToggle({
		.id = u"miogram/system/animations"_q,
		.title = tr::mio_Animations(),
		.getter = [] { return Miogram::PerformanceOptimizer::instance().animationsEnabled(); },
		.setter = [](bool v) { Miogram::PerformanceOptimizer::instance().setAnimationsEnabled(v); },
	});
	builder.addButton({
		.id = u"miogram/system/updater"_q,
		.title = tr::mio_CheckUpdates(),
		.onClick = [controller = builder.controller()] {
			Miogram::MiogramUpdater::instance().checkAndShow(
				[controller](const Miogram::UpdateInfo &info) {
					if (!controller) {
						return;
					}
					const auto text = info.hasUpdate
						? u"Miogram %1 available (current %2). %3"_q
							.arg(info.version)
							.arg(info.currentVersion)
							.arg(info.changelog.left(500))
						: u"Miogram is up to date (%1)."_q.arg(info.currentVersion);
					Ui::show(Ui::MakeInformBox(text));
				},
				true);
		},
	});
	builder.addButton({
		.id = u"miogram/system/musordrop"_q,
		.title = tr::mio_MusorDrop(),
		.onClick = [controller = builder.controller()] {
			const auto media = Miogram::MusorDrop::locateMediaFile();
			const auto hint = Miogram::MusorDrop::missingHint(
				QString(),
				QString(),
				QString());
			Miogram::MusorDropOverlay::show(media, hint);
			if (!media.isEmpty()) {
				QDesktopServices::openUrl(QUrl::fromLocalFile(media));
			} else if (controller) {
				controller->showToast(hint, 3500);
			}
		},
	});
}

} // namespace Settings
