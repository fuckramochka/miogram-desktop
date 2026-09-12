// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct LrcLine {
	int timeMs = 0;
	QString text;
	QString translation;
};

struct LrcSong {
	QString title;
	QString artist;
	QString source;
	std::vector<LrcLine> lines;
	QString plainLyrics;

	[[nodiscard]] bool isEmpty() const {
		return lines.empty() && plainLyrics.trimmed().isEmpty();
	}

	[[nodiscard]] int currentLineIndex(int positionMs) const {
		if (lines.empty()) return -1;
		int best = -1;
		for (size_t i = 0; i < lines.size(); ++i) {
			if (lines[i].timeMs <= positionMs) {
				best = static_cast<int>(i);
			} else {
				break;
			}
		}
		return best;
	}
};

class LrcParser {
public:
	static LrcSong parse(
		const QString &content,
		const QString &title = QString(),
		const QString &artist = QString(),
		const QString &source = QStringLiteral("LRC"));
};

class LyricsEngine {
public:
	static LyricsEngine &instance();

	void initialize();
	void fetchLyrics(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback);

	[[nodiscard]] static QString cleanTitle(const QString &title);
	[[nodiscard]] static QString cleanArtist(const QString &artist);

private:
	LyricsEngine();
	void fetchFromLrclib(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback);
	void fetchFromNetEase(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback);

	LrcSong loadFromCache(const QString &key);
	void saveToCache(const QString &key, const LrcSong &song);

	bool _initialized = false;
};

} // namespace Miogram
