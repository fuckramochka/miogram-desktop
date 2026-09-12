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

enum class PlayerTab {
	Cover = 0,
	Lyrics = 1,
	Queue = 2,
};

enum class RepeatMode {
	Off = 0,
	All = 1,
	One = 2,
};

enum class LyricsSource {
	Auto = 0,
	Server = 1,
	Lrclib = 2,
	NetEase = 3,
	Yandex = 4,
	Genius = 5,
	YouTube = 6,
	Ai = 7,
};

struct PlayerTrack {
	QString title;
	QString artist;
	QString filePath;
	int durationMs = 0;
	QString coverPath;
	bool isFavorite = false;
};

struct BassBands {
	static constexpr int kBands = 32;
	float values[kBands] = {};
};

class PlayerPrefs {
public:
	static PlayerPrefs &instance();

	void initialize();

	[[nodiscard]] bool shuffle() const;
	void setShuffle(bool shuffle);

	[[nodiscard]] RepeatMode repeat() const;
	void setRepeat(RepeatMode mode);
	void cycleRepeat();

	[[nodiscard]] float speed() const;
	void setSpeed(float speed);

	[[nodiscard]] int volume() const;
	void setVolume(int volume);

	[[nodiscard]] QString backdrop() const;
	void setBackdrop(const QString &path);

	[[nodiscard]] PlayerTab lastTab() const;
	void setLastTab(PlayerTab tab);

	[[nodiscard]] LyricsSource lyricsSource() const;
	void setLyricsSource(LyricsSource source);

private:
	PlayerPrefs();

	bool _shuffle = false;
	RepeatMode _repeat = RepeatMode::Off;
	float _speed = 1.f;
	int _volume = 80;
	QString _backdrop;
	PlayerTab _lastTab = PlayerTab::Cover;
	LyricsSource _lyricsSource = LyricsSource::Auto;
	bool _initialized = false;
};

class ModernPlayer {
public:
	static ModernPlayer &instance();

	void initialize();

	[[nodiscard]] PlayerTab currentTab() const;
	void setCurrentTab(PlayerTab tab);

	[[nodiscard]] std::vector<PlayerTrack> queue() const;
	void setQueue(std::vector<PlayerTrack> queue);
	[[nodiscard]] int currentIndex() const;

	void playAt(int index);
	void playNext();
	void playPrev();
	void togglePlay();

	[[nodiscard]] bool isPlaying() const;
	void setPlaying(bool playing);

	[[nodiscard]] int positionMs() const;
	void seekTo(int ms);
	void onTick(int positionMs);

	[[nodiscard]] PlayerTrack currentTrack() const;
	void toggleFavorite();

	[[nodiscard]] QString lyricsSourceTitle(LyricsSource source) const;
	[[nodiscard]] std::vector<LyricsSource> allLyricsSources() const;

private:
	ModernPlayer();

	std::vector<PlayerTrack> _queue;
	int _index = -1;
	bool _playing = false;
	int _positionMs = 0;
	PlayerTab _tab = PlayerTab::Cover;
	bool _initialized = false;
};

class BassVisualizer {
public:
	static BassVisualizer &instance();

	void initialize();

	BassBands analyze(const std::vector<float> &pcmMono, int sampleRate);
	[[nodiscard]] BassBands current() const;
	void setSmoothing(float smoothing);

	[[nodiscard]] QColor bandColor(float level, const QColor &accent) const;

private:
	BassVisualizer();

	BassBands _current;
	float _smoothing = 0.72f;
	bool _initialized = false;
};

class AppleMusicSheetState {
public:
	static AppleMusicSheetState &instance();

	void initialize();

	[[nodiscard]] bool isExpanded() const;
	void setExpanded(bool expanded);

	[[nodiscard]] bool showWaveform() const;
	void setShowWaveform(bool show);

	[[nodiscard]] bool showLyrics() const;
	void setShowLyrics(bool show);

private:
	AppleMusicSheetState();

	bool _expanded = false;
	bool _waveform = true;
	bool _lyrics = true;
	bool _initialized = false;
};

} // namespace Miogram
