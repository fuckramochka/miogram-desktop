// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QMap>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct SteamPresence {
	bool active = false;
	QString gameTitle;
	QString iconUrl;
	int hoursPlayed = 0;
	bool inGame = false;
	QString steamId;
};

struct SpotifyPresence {
	bool active = false;
	QString track;
	QString artist;
	QString album;
	QString coverUrl;
	int durationSec = 0;
	bool isPlaying = false;
};

struct GitHubPresence {
	bool active = false;
	QString username;
	QString avatarUrl;
	int publicRepos = 0;
	int followers = 0;
	QString bio;
};

struct DiscordPresence {
	bool active = false;
	QString username;
	QString status;
	QString activity;
};

struct UserPresenceCard {
	uint64 userId = 0;
	bool isOnline = false;
	SteamPresence steam;
	SpotifyPresence spotify;
	GitHubPresence github;
	DiscordPresence discord;

	[[nodiscard]] int activeServicesCount() const {
		int count = 0;
		if (steam.active) count++;
		if (spotify.active) count++;
		if (github.active) count++;
		if (discord.active) count++;
		return count;
	}
};

class DigitalPresenceManager {
public:
	static DigitalPresenceManager &instance();

	void initialize();

	[[nodiscard]] UserPresenceCard selfPresence() const;
	[[nodiscard]] UserPresenceCard presenceForUser(uint64 userId) const;

	void setSteamId(const QString &steamId);
	[[nodiscard]] QString steamId() const;

	void setGitHubUsername(const QString &username);
	[[nodiscard]] QString gitHubUsername() const;

	void updateSpotifyPlayback(const QString &track, const QString &artist, const QString &coverUrl, bool isPlaying);

	void syncSelfToCloud();
	void fetchPresenceFromCloud(uint64 userId, Fn<void(const UserPresenceCard &)> callback = nullptr);

private:
	DigitalPresenceManager();
	void loadSettings();
	void saveSettings();

	UserPresenceCard _selfPresence;
	QMap<uint64, UserPresenceCard> _cache;
	bool _initialized = false;
};

} // namespace Miogram
