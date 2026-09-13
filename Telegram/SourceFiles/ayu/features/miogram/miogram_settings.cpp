// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_settings.h"

#include "ayu/features/miogram/miogram_ai.h"
#include "ayu/features/miogram/miogram_ai_companion.h"
#include "ayu/features/miogram/miogram_antiblock.h"
#include "ayu/features/miogram/miogram_badges.h"
#include "ayu/features/miogram/miogram_presence.h"
#include "ayu/features/miogram/miogram_cloudvault.h"
#include "ayu/features/miogram/miogram_config.h"
#include "ayu/features/miogram/miogram_custom_ui.h"
#include "ayu/features/miogram/miogram_feed.h"
#include "ayu/features/miogram/miogram_folders_multichat.h"
#include "ayu/features/miogram/miogram_kanban.h"
#include "ayu/features/miogram/miogram_layouts.h"
#include "ayu/features/miogram/miogram_locale_fun.h"
#include "ayu/features/miogram/miogram_lyrics.h"
#include "ayu/features/miogram/miogram_lyrics_extra.h"
#include "ayu/features/miogram/miogram_music.h"
#include "ayu/features/miogram/miogram_player.h"
#include "ayu/features/miogram/miogram_plugins.h"
#include "ayu/features/miogram/miogram_render.h"
#include "ayu/features/miogram/miogram_storage.h"
#include "ayu/features/miogram/miogram_stt_audio.h"
#include "ayu/features/miogram/miogram_supabase.h"
#include "ayu/features/miogram/miogram_updater.h"
#include "ayu/features/miogram/miogram_userbot_performance.h"
#include "ayu/features/miogram/miogram_vault.h"
#include "ayu/features/miogram/miogram_wasm.h"
#include "ayu/features/miogram/miogram_divine.h"

namespace Miogram {

MiogramSettingsModel &MiogramSettingsModel::instance() {
	static MiogramSettingsModel model;
	return model;
}

MiogramSettingsModel::MiogramSettingsModel() {
}

void MiogramSettingsModel::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_sections = {
		{
			.id = u"miogram/player"_q,
			.titleUk = u"Плеєр Miogram"_q,
			.titleRu = u"Плеер Miogram"_q,
			.titleEn = u"Miogram Player"_q,
			.controlIds = {
				u"miogram/player/shuffle"_q,
				u"miogram/player/repeat"_q,
				u"miogram/player/speed"_q,
				u"miogram/player/volume"_q,
				u"miogram/player/lyrics-source"_q,
				u"miogram/player/backdrop"_q,
			},
		},
		{
			.id = u"miogram/layouts"_q,
			.titleUk = u"Інтерфейс"_q,
			.titleRu = u"Интерфейс"_q,
			.titleEn = u"Interface"_q,
			.controlIds = {
				u"miogram/layouts/mode"_q,
				u"miogram/layouts/bubble-radius"_q,
				u"miogram/layouts/avatar-shape"_q,
				u"miogram/layouts/glass"_q,
				u"miogram/layouts/ame"_q,
			},
		},
		{
			.id = u"miogram/privacy"_q,
			.titleUk = u"Подвійне дно"_q,
			.titleRu = u"Двойное дно"_q,
			.titleEn = u"Double Bottom"_q,
			.controlIds = {
				u"miogram/privacy/double-bottom"_q,
				u"miogram/privacy/duress"_q,
				u"miogram/privacy/hidden-dialogs"_q,
				u"miogram/privacy/cloud-vault"_q,
			},
		},
		{
			.id = u"miogram/ai"_q,
			.titleUk = u"AI помічник"_q,
			.titleRu = u"AI помощник"_q,
			.titleEn = u"AI Assistant"_q,
			.controlIds = {
				u"miogram/ai/keys"_q,
				u"miogram/ai/persona"_q,
				u"miogram/ai/companion"_q,
				u"miogram/ai/privacy-shield"_q,
				u"miogram/ai/stt"_q,
			},
		},
		{
			.id = u"miogram/badges"_q,
			.titleUk = u"Бейджі"_q,
			.titleRu = u"Бейджи"_q,
			.titleEn = u"Badges"_q,
			.controlIds = {
				u"miogram/badges/list"_q,
				u"miogram/badges/sync"_q,
			},
		},
		{
			.id = u"miogram/feed"_q,
			.titleUk = u"Розумна стрічка"_q,
			.titleRu = u"Умная лента"_q,
			.titleEn = u"Smart Feed"_q,
			.controlIds = {
				u"miogram/feed/enabled"_q,
				u"miogram/feed/digest"_q,
			},
		},
		{
			.id = u"miogram/kanban"_q,
			.titleUk = u"Канбан"_q,
			.titleRu = u"Канбан"_q,
			.titleEn = u"Kanban"_q,
			.controlIds = {
				u"miogram/kanban/board"_q,
			},
		},
		{
			.id = u"miogram/folders"_q,
			.titleUk = u"Підпапки"_q,
			.titleRu = u"Подпапки"_q,
			.titleEn = u"Subfolders"_q,
			.controlIds = {
				u"miogram/folders/list"_q,
				u"miogram/folders/floating"_q,
				u"miogram/folders/split"_q,
			},
		},
		{
			.id = u"miogram/plugins"_q,
			.titleUk = u"Плагіни"_q,
			.titleRu = u"Плагины"_q,
			.titleEn = u"Plugins"_q,
			.controlIds = {
				u"miogram/plugins/list"_q,
				u"miogram/plugins/forge"_q,
				u"miogram/plugins/notifications"_q,
			},
		},
		{
			.id = u"miogram/system"_q,
			.titleUk = u"Система"_q,
			.titleRu = u"Система"_q,
			.titleEn = u"System"_q,
			.controlIds = {
				u"miogram/system/updater"_q,
				u"miogram/system/performance"_q,
				u"miogram/system/fps"_q,
				u"miogram/system/heroku"_q,
				u"miogram/system/musordrop"_q,
				u"miogram/system/locale"_q,
			},
		},
	};
}

std::vector<SettingsSection> MiogramSettingsModel::sections() const {
	return _sections;
}

SettingsSection MiogramSettingsModel::sectionById(const QString &id) const {
	for (const auto &section : _sections) {
		if (section.id == id) {
			return section;
		}
	}
	return SettingsSection{};
}

QString MiogramSettingsModel::localizedTitle(const SettingsSection &section) const {
	return MiogramLocale::instance().get(
		section.titleUk,
		section.titleRu,
		section.titleEn);
}

void InitAllMiogramModules() {
	MiogramConfig::instance().initialize();
	ProfileVault::instance().initialize();
	MiogramGate::instance().initialize();
	DoubleBottomManager::instance().initialize();
	CloudVaultEngine::instance().initialize();
	AiService::instance().initialize();
	CompanionPrefs::instance().initialize();
	CompanionToolbox::instance().initialize();
	SttEngine::instance().initialize();
	SupabaseBridge::instance().initialize();
	DivineEngine::instance().initialize();
	LayoutController::instance().initialize();
	MinimalRail::instance().initialize();
	CustomUiPrefs::instance().initialize();
	UiEngine::instance().initialize();
	HapticEngine::instance().initialize();
	Glassmorphism::instance().initialize();
	PlayerPrefs::instance().initialize();
	ModernPlayer::instance().initialize();
	BassVisualizer::instance().initialize();
	AppleMusicSheetState::instance().initialize();
	LyricsEngine::instance().initialize();
	ExtendedLyricsSources::instance().initialize();
	MusicSearchEngine::instance().initialize();
	SmartFeedService::instance().initialize();
	KanbanStorage::instance().initialize();
	SubfolderEngine::instance().initialize();
	FloatingChatState::instance().initialize();
	SplitChatState::instance().initialize();
	MiogramLocale::instance().initialize();
	LocalizerEngine::instance().initialize();
	PluginEngine::instance().initialize();
	HookManager::instance().initialize();
	InAppNotifications::instance().initialize();
	MiogramUpdater::instance().initialize();
	DownloadManager::instance().initialize();
	HerokuManager::instance().initialize();
	PerformanceOptimizer::instance().initialize();
	FpsController::instance().initialize();
	MiogramRender::instance().initialize();
	FileVault::instance().initialize();
	HistoryStoragePolicy::instance().initialize();
	EncryptedKeyValue::instance().initialize();
	AudioFrontend::instance().initialize();
	OnnxWhisperBridge::instance().initialize();
	WasmRuntime::instance().initialize();
	AntiBlockEngine::instance().initialize();
	DigitalPresenceManager::instance().initialize();
	MiogramSettingsModel::instance().initialize();
}

} // namespace Miogram
