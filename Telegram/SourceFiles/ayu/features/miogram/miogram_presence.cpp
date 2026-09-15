// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_presence.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "ayu/features/miogram/miogram_badges.h"
#include "core/application.h"

namespace Miogram {

namespace {

QString PresenceSettingsPath() {
	return cWorkingDir() + u"tdata/mio_presence.json"_q;
}

QJsonObject SerializePresence(const UserPresenceCard &card) {
	QJsonObject root;
	root[u"user_id"_q] = static_cast<qint64>(card.userId);
	root[u"online"_q] = card.isOnline;

	if (card.steam.active) {
		QJsonObject s;
		s[u"game"_q] = card.steam.gameTitle;
		s[u"icon"_q] = card.steam.iconUrl;
		s[u"hours"_q] = card.steam.hoursPlayed;
		s[u"in_game"_q] = card.steam.inGame;
		s[u"steam_id"_q] = card.steam.steamId;
		root[u"steam"_q] = s;
	}

	if (card.spotify.active) {
		QJsonObject sp;
		sp[u"track"_q] = card.spotify.track;
		sp[u"artist"_q] = card.spotify.artist;
		sp[u"album"_q] = card.spotify.album;
		sp[u"cover"_q] = card.spotify.coverUrl;
		sp[u"duration"_q] = card.spotify.durationSec;
		sp[u"playing"_q] = card.spotify.isPlaying;
		root[u"spotify"_q] = sp;
	}

	if (card.github.active) {
		QJsonObject gh;
		gh[u"username"_q] = card.github.username;
		gh[u"avatar"_q] = card.github.avatarUrl;
		gh[u"repos"_q] = card.github.publicRepos;
		gh[u"followers"_q] = card.github.followers;
		gh[u"bio"_q] = card.github.bio;
		root[u"github"_q] = gh;
	}

	if (card.discord.active) {
		QJsonObject dc;
		dc[u"username"_q] = card.discord.username;
		dc[u"status"_q] = card.discord.status;
		dc[u"activity"_q] = card.discord.activity;
		root[u"discord"_q] = dc;
	}

	return root;
}

UserPresenceCard DeserializePresence(const QJsonObject &root) {
	UserPresenceCard card;
	card.userId = static_cast<uint64>(root.value(u"user_id"_q).toVariant().toLongLong());
	card.isOnline = root.value(u"online"_q).toBool(false);

	if (root.contains(u"steam"_q)) {
		const auto s = root.value(u"steam"_q).toObject();
		card.steam.active = true;
		card.steam.gameTitle = s.value(u"game"_q).toString();
		card.steam.iconUrl = s.value(u"icon"_q).toString();
		card.steam.hoursPlayed = s.value(u"hours"_q).toInt();
		card.steam.inGame = s.value(u"in_game"_q).toBool();
		card.steam.steamId = s.value(u"steam_id"_q).toString();
	}

	if (root.contains(u"spotify"_q)) {
		const auto sp = root.value(u"spotify"_q).toObject();
		card.spotify.active = true;
		card.spotify.track = sp.value(u"track"_q).toString();
		card.spotify.artist = sp.value(u"artist"_q).toString();
		card.spotify.album = sp.value(u"album"_q).toString();
		card.spotify.coverUrl = sp.value(u"cover"_q).toString();
		card.spotify.durationSec = sp.value(u"duration"_q).toInt();
		card.spotify.isPlaying = sp.value(u"playing"_q).toBool();
	}

	if (root.contains(u"github"_q)) {
		const auto gh = root.value(u"github"_q).toObject();
		card.github.active = true;
		card.github.username = gh.value(u"username"_q).toString();
		card.github.avatarUrl = gh.value(u"avatar"_q).toString();
		card.github.publicRepos = gh.value(u"repos"_q).toInt();
		card.github.followers = gh.value(u"followers"_q).toInt();
		card.github.bio = gh.value(u"bio"_q).toString();
	}

	if (root.contains(u"discord"_q)) {
		const auto dc = root.value(u"discord"_q).toObject();
		card.discord.active = true;
		card.discord.username = dc.value(u"username"_q).toString();
		card.discord.status = dc.value(u"status"_q).toString();
		card.discord.activity = dc.value(u"activity"_q).toString();
	}

	return card;
}

} // namespace

DigitalPresenceManager &DigitalPresenceManager::instance() {
	static DigitalPresenceManager mgr;
	return mgr;
}

DigitalPresenceManager::DigitalPresenceManager() {
}

void DigitalPresenceManager::initialize() {
	if (_initialized) return;
	_initialized = true;

	loadSettings();
}

UserPresenceCard DigitalPresenceManager::selfPresence() const {
	return _selfPresence;
}

UserPresenceCard DigitalPresenceManager::presenceForUser(uint64 userId) const {
	const auto it = _cache.find(userId);
	if (it != _cache.end()) {
		return it.value();
	}
	UserPresenceCard empty;
	empty.userId = userId;
	return empty;
}

void DigitalPresenceManager::setSteamId(const QString &steamId) {
	_selfPresence.steam.steamId = steamId.trimmed();
	_selfPresence.steam.active = !steamId.trimmed().isEmpty();
	saveSettings();
	syncSelfToCloud();
}

QString DigitalPresenceManager::steamId() const {
	return _selfPresence.steam.steamId;
}

void DigitalPresenceManager::setGitHubUsername(const QString &username) {
	_selfPresence.github.username = username.trimmed();
	_selfPresence.github.active = !username.trimmed().isEmpty();
	saveSettings();
	syncSelfToCloud();
}

QString DigitalPresenceManager::gitHubUsername() const {
	return _selfPresence.github.username;
}

void DigitalPresenceManager::updateSpotifyPlayback(
	const QString &track,
	const QString &artist,
	const QString &coverUrl,
	bool isPlaying) {
	_selfPresence.spotify.track = track;
	_selfPresence.spotify.artist = artist;
	_selfPresence.spotify.coverUrl = coverUrl;
	_selfPresence.spotify.isPlaying = isPlaying;
	_selfPresence.spotify.active = isPlaying && !track.isEmpty();

	syncSelfToCloud();
}

void DigitalPresenceManager::syncSelfToCloud() {
	auto *nam = new QNetworkAccessManager();
	QUrl url(kSupabaseUrl + u"/rest/v1/miogram_presence"_q);
	QNetworkRequest req(url);
	req.setRawHeader("apikey", kSupabaseAnonKey.toUtf8());
	req.setRawHeader("Authorization", ("Bearer " + kSupabaseAnonKey).toUtf8());
	req.setRawHeader("Content-Type", "application/json");
	req.setRawHeader("Prefer", "resolution=merge-duplicates");

	QJsonObject payload = SerializePresence(_selfPresence);
	payload[u"updated_at"_q] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	QNetworkReply *reply = nam->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
	QObject::connect(reply, &QNetworkReply::finished, [nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
	});
}

void DigitalPresenceManager::fetchPresenceFromCloud(
	uint64 userId,
	Fn<void(const UserPresenceCard &)> callback) {
	if (!userId) {
		if (callback) callback(UserPresenceCard{});
		return;
	}

	auto *nam = new QNetworkAccessManager();
	QUrl url(QString("%1/rest/v1/miogram_presence?user_id=eq.%2&select=*")
		.arg(kSupabaseUrl)
		.arg(userId));
	QNetworkRequest req(url);
	req.setRawHeader("apikey", kSupabaseAnonKey.toUtf8());
	req.setRawHeader("Authorization", ("Bearer " + kSupabaseAnonKey).toUtf8());

	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [this, userId, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() == QNetworkReply::NoError) {
			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (doc.isArray() && !doc.array().isEmpty()) {
				const auto obj = doc.array().first().toObject();
				const auto card = DeserializePresence(obj);
				_cache.insert(userId, card);
				if (callback) callback(card);
				return;
			}
		}

		if (callback) callback(presenceForUser(userId));
	});
}

void DigitalPresenceManager::loadSettings() {
	QFile f(PresenceSettingsPath());
	if (!f.open(QIODevice::ReadOnly)) return;

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return;

	_selfPresence = DeserializePresence(doc.object());
}

void DigitalPresenceManager::saveSettings() {
	const auto root = SerializePresence(_selfPresence);
	QFile f(PresenceSettingsPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
