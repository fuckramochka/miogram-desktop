// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct MusicTrack {
	QString id;
	QString title;
	QString artist;
	QString album;
	int durationSec = 0;
	QString previewAudioUrl;
	QString coverArtworkUrl;
	QString source; // "deezer" or "itunes"
};

class MusicSearchEngine {
public:
	static MusicSearchEngine &instance();

	void initialize();
	void search(const QString &query, Fn<void(std::vector<MusicTrack>)> callback);

	[[nodiscard]] static QString normalizeForDedup(const QString &str);

private:
	MusicSearchEngine();

	void searchDeezer(const QString &query, Fn<void(std::vector<MusicTrack>)> callback);
	void searchItunes(const QString &query, Fn<void(std::vector<MusicTrack>)> callback);

	bool _initialized = false;
};

} // namespace Miogram
