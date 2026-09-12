// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_player.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
#include <algorithm>
#include <cmath>

#include "ayu/features/miogram/miogram_config.h"

namespace Miogram {

PlayerPrefs &PlayerPrefs::instance() {
	static PlayerPrefs prefs;
	return prefs;
}

PlayerPrefs::PlayerPrefs() {
}

void PlayerPrefs::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto &config = MiogramConfig::instance();
	const auto obj = config.getDomain(ConfigDomain::Player);
	_shuffle = obj.value(u"shuffle"_q).toBool(false);
	_repeat = static_cast<RepeatMode>(obj.value(u"repeat"_q).toInt(0));
	_speed = float(obj.value(u"speed"_q).toDouble(1.));
	_volume = obj.value(u"volume"_q).toInt(80);
	_backdrop = obj.value(u"backdrop"_q).toString();
	_lastTab = static_cast<PlayerTab>(obj.value(u"tab"_q).toInt(0));
	_lyricsSource = static_cast<LyricsSource>(obj.value(u"lyricsSource"_q).toInt(0));
}

bool PlayerPrefs::shuffle() const {
	return _shuffle;
}

void PlayerPrefs::setShuffle(bool shuffle) {
	_shuffle = shuffle;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"shuffle"_q, shuffle);
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

RepeatMode PlayerPrefs::repeat() const {
	return _repeat;
}

void PlayerPrefs::setRepeat(RepeatMode mode) {
	_repeat = mode;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"repeat"_q, static_cast<int>(mode));
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

void PlayerPrefs::cycleRepeat() {
	const auto next = (_repeat == RepeatMode::Off)
		? RepeatMode::All
		: (_repeat == RepeatMode::All)
			? RepeatMode::One
			: RepeatMode::Off;
	setRepeat(next);
}

float PlayerPrefs::speed() const {
	return _speed;
}

void PlayerPrefs::setSpeed(float speed) {
	_speed = std::clamp(speed, 0.25f, 2.f);
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"speed"_q, double(_speed));
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

int PlayerPrefs::volume() const {
	return _volume;
}

void PlayerPrefs::setVolume(int volume) {
	_volume = std::clamp(volume, 0, 100);
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"volume"_q, _volume);
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

QString PlayerPrefs::backdrop() const {
	return _backdrop;
}

void PlayerPrefs::setBackdrop(const QString &path) {
	_backdrop = path;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"backdrop"_q, path);
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

PlayerTab PlayerPrefs::lastTab() const {
	return _lastTab;
}

void PlayerPrefs::setLastTab(PlayerTab tab) {
	_lastTab = tab;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"tab"_q, static_cast<int>(tab));
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

LyricsSource PlayerPrefs::lyricsSource() const {
	return _lyricsSource;
}

void PlayerPrefs::setLyricsSource(LyricsSource source) {
	_lyricsSource = source;
	auto obj = MiogramConfig::instance().getDomain(ConfigDomain::Player);
	obj.insert(u"lyricsSource"_q, static_cast<int>(source));
	MiogramConfig::instance().setDomain(ConfigDomain::Player, obj);
}

ModernPlayer &ModernPlayer::instance() {
	static ModernPlayer player;
	return player;
}

ModernPlayer::ModernPlayer() {
}

void ModernPlayer::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	PlayerPrefs::instance().initialize();
	_tab = PlayerPrefs::instance().lastTab();
}

PlayerTab ModernPlayer::currentTab() const {
	return _tab;
}

void ModernPlayer::setCurrentTab(PlayerTab tab) {
	_tab = tab;
	PlayerPrefs::instance().setLastTab(tab);
}

std::vector<PlayerTrack> ModernPlayer::queue() const {
	return _queue;
}

void ModernPlayer::setQueue(std::vector<PlayerTrack> queue) {
	_queue = std::move(queue);
	if (_index >= int(_queue.size())) {
		_index = _queue.empty() ? -1 : 0;
	}
}

int ModernPlayer::currentIndex() const {
	return _index;
}

void ModernPlayer::playAt(int index) {
	if (index < 0 || index >= int(_queue.size())) {
		return;
	}
	_index = index;
	_positionMs = 0;
	_playing = true;
}

void ModernPlayer::playNext() {
	if (_queue.empty()) {
		return;
	}
	const auto prefs = PlayerPrefs::instance();
	if (prefs.shuffle()) {
		_index = int(QRandomGenerator::global()->bounded(quint32(_queue.size())));
	} else {
		_index = (_index + 1) % int(_queue.size());
	}
	_positionMs = 0;
	_playing = true;
}

void ModernPlayer::playPrev() {
	if (_queue.empty()) {
		return;
	}
	if (_positionMs > 3000) {
		_positionMs = 0;
		return;
	}
	_index = (_index - 1 + int(_queue.size())) % int(_queue.size());
	_positionMs = 0;
	_playing = true;
}

void ModernPlayer::togglePlay() {
	_playing = !_playing;
}

bool ModernPlayer::isPlaying() const {
	return _playing;
}

void ModernPlayer::setPlaying(bool playing) {
	_playing = playing;
}

int ModernPlayer::positionMs() const {
	return _positionMs;
}

void ModernPlayer::seekTo(int ms) {
	_positionMs = std::max(0, ms);
}

void ModernPlayer::onTick(int positionMs) {
	_positionMs = positionMs;
	const auto track = currentTrack();
	if (track.durationMs > 0 && positionMs >= track.durationMs) {
		const auto mode = PlayerPrefs::instance().repeat();
		if (mode == RepeatMode::One) {
			_positionMs = 0;
		} else {
			playNext();
		}
	}
}

PlayerTrack ModernPlayer::currentTrack() const {
	if (_index < 0 || _index >= int(_queue.size())) {
		return PlayerTrack{};
	}
	return _queue[_index];
}

void ModernPlayer::toggleFavorite() {
	if (_index < 0 || _index >= int(_queue.size())) {
		return;
	}
	_queue[_index].isFavorite = !_queue[_index].isFavorite;
}

QString ModernPlayer::lyricsSourceTitle(LyricsSource source) const {
	switch (source) {
	case LyricsSource::Auto: return u"Auto"_q;
	case LyricsSource::Server: return u"Server"_q;
	case LyricsSource::Lrclib: return u"LRCLib"_q;
	case LyricsSource::NetEase: return u"NetEase"_q;
	case LyricsSource::Yandex: return u"Yandex"_q;
	case LyricsSource::Genius: return u"Genius"_q;
	case LyricsSource::YouTube: return u"YouTube"_q;
	case LyricsSource::Ai: return u"AI"_q;
	}
	return u"Auto"_q;
}

std::vector<LyricsSource> ModernPlayer::allLyricsSources() const {
	return {
		LyricsSource::Auto,
		LyricsSource::Server,
		LyricsSource::Lrclib,
		LyricsSource::NetEase,
		LyricsSource::Yandex,
		LyricsSource::Genius,
		LyricsSource::YouTube,
		LyricsSource::Ai,
	};
}

BassVisualizer &BassVisualizer::instance() {
	static BassVisualizer visualizer;
	return visualizer;
}

BassVisualizer::BassVisualizer() {
}

void BassVisualizer::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

BassBands BassVisualizer::analyze(const std::vector<float> &pcmMono, int sampleRate) {
// A lightweight 32-band spectrum is derived without FFT by measuring
// RMS energy over exponentially growing time windows, which mirrors
// how bass carries more energy in longer windows. Each band is
// normalized with a logarithmic curve, then temporally smoothed with
// separate attack and release coefficients, so peaks snap instantly
// while decay trails softly like the mobile BassVisualizer.
	BassBands out;
	if (pcmMono.empty()) {
		return _current;
	}
	const auto n = pcmMono.size();
	for (int b = 0; b < BassBands::kBands; ++b) {
		const auto start = (n * b * b) / (BassBands::kBands * BassBands::kBands);
		const auto end = (n * (b + 1) * (b + 1)) / (BassBands::kBands * BassBands::kBands);
		double sum = 0;
		int count = 0;
		for (auto i = start; i < end && i < int(n); ++i) {
			sum += double(pcmMono[i]) * double(pcmMono[i]);
			++count;
		}
		const auto rms = count ? std::sqrt(sum / count) : 0.;
		const auto norm = std::min(1., rms * 3.);
		const auto shaped = float(std::pow(norm, 0.65));
		const auto prev = _current.values[b];
		out.values[b] = (shaped > prev)
			? prev + (shaped - prev) * 0.85f
			: prev + (shaped - prev) * (1.f - _smoothing);
	}
	_current = out;
	Q_UNUSED(sampleRate);
	return _current;
}

BassBands BassVisualizer::current() const {
	return _current;
}

void BassVisualizer::setSmoothing(float smoothing) {
	_smoothing = std::clamp(smoothing, 0.f, 0.95f);
}

QColor BassVisualizer::bandColor(float level, const QColor &accent) const {
	const auto clamped = std::clamp(level, 0.f, 1.f);
	return QColor(
		int(accent.red() * (0.35 + 0.65 * clamped)),
		int(accent.green() * (0.35 + 0.65 * clamped)),
		int(accent.blue() * (0.35 + 0.65 * clamped)));
}

AppleMusicSheetState &AppleMusicSheetState::instance() {
	static AppleMusicSheetState state;
	return state;
}

AppleMusicSheetState::AppleMusicSheetState() {
}

void AppleMusicSheetState::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

bool AppleMusicSheetState::isExpanded() const {
	return _expanded;
}

void AppleMusicSheetState::setExpanded(bool expanded) {
	_expanded = expanded;
}

bool AppleMusicSheetState::showWaveform() const {
	return _waveform;
}

void AppleMusicSheetState::setShowWaveform(bool show) {
	_waveform = show;
}

bool AppleMusicSheetState::showLyrics() const {
	return _lyrics;
}

void AppleMusicSheetState::setShowLyrics(bool show) {
	_lyrics = show;
}

} // namespace Miogram
