// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include "base/basic_types.h"

#include "ayu/features/miogram/miogram_lyrics.h"
#include "ayu/features/miogram/miogram_player.h"

namespace Miogram {

class ExtendedLyricsSources {
public:
	static ExtendedLyricsSources &instance();

	void initialize();

	void fetchWithSource(
		LyricsSource source,
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback);

	void fetchChained(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback);

private:
	ExtendedLyricsSources();

	void fetchFromYandex(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback);
	void fetchFromGenius(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback);
	void fetchFromYouTube(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback);
	void fetchFromAi(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback);
	void fetchFromServer(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback);

	bool _initialized = false;
};

} // namespace Miogram
