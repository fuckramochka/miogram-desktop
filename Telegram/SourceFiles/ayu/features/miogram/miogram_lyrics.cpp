// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_lyrics.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUrlQuery>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <algorithm>

#include "core/application.h"

namespace Miogram {

namespace {

QString LyricsCacheDir() {
	const auto dir = cWorkingDir() + u"tdata/mio_lrc/"_q;
	QDir().mkpath(dir);
	return dir;
}

QString CacheKey(const QString &title, const QString &artist) {
	const auto raw = (artist.trimmed() + u" - "_q + title.trimmed()).toLower();
	return QString::fromLatin1(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha1).toHex());
}

} // namespace

LrcSong LrcParser::parse(
	const QString &content,
	const QString &title,
	const QString &artist,
	const QString &source) {
	LrcSong song;
	song.title = title;
	song.artist = artist;
	song.source = source;

	static const QRegularExpression timeRegex(
		QStringLiteral(R"(\[(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?\])"));

	const auto lines = content.split(u'\n');
	int offsetMs = 0;

	for (const auto &rawLine : lines) {
		const auto line = rawLine.trimmed();
		if (line.isEmpty()) continue;

		if (line.startsWith(u"[offset:"_q, Qt::CaseInsensitive)) {
			const auto val = line.mid(8).remove(u']').trimmed().toInt();
			offsetMs = val;
			continue;
		}

		auto matchIterator = timeRegex.globalMatch(line);
		std::vector<int> timestamps;
		int lastMatchEnd = 0;

		while (matchIterator.hasNext()) {
			auto match = matchIterator.next();
			const int min = match.captured(1).toInt();
			const int sec = match.captured(2).toInt();
			int ms = 0;
			if (!match.captured(3).isEmpty()) {
				const auto msStr = match.captured(3);
				if (msStr.length() == 1) ms = msStr.toInt() * 100;
				else if (msStr.length() == 2) ms = msStr.toInt() * 10;
				else ms = msStr.left(3).toInt();
			}
			timestamps.push_back(min * 60000 + sec * 1000 + ms);
			lastMatchEnd = match.capturedEnd();
		}

		const auto text = line.mid(lastMatchEnd).trimmed();
		for (int t : timestamps) {
			song.lines.push_back(LrcLine{
				.timeMs = qMax(0, t + offsetMs),
				.text = text,
			});
		}
	}

	std::sort(song.lines.begin(), song.lines.end(), [](const LrcLine &a, const LrcLine &b) {
		return a.timeMs < b.timeMs;
	});

	return song;
}

LyricsEngine &LyricsEngine::instance() {
	static LyricsEngine engine;
	return engine;
}

LyricsEngine::LyricsEngine() {
}

void LyricsEngine::initialize() {
	if (_initialized) return;
	_initialized = true;
	LyricsCacheDir();
}

QString LyricsEngine::cleanTitle(const QString &title) {
	QString t = title;
	static const QRegularExpression extRegex(QStringLiteral(R"(\.(mp3|flac|wav|m4a|ogg|opus)$)"), QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression tagRegex(QStringLiteral(R"(\s*[\(\[](official\s*(video|audio|lyrics|music\s*video)|lyrics|remastered|feat\..*|ft\..*)[\)\]])"), QRegularExpression::CaseInsensitiveOption);
	t.remove(extRegex);
	t.remove(tagRegex);
	return t.trimmed();
}

QString LyricsEngine::cleanArtist(const QString &artist) {
	QString a = artist;
	static const QRegularExpression topicRegex(QStringLiteral(R"(\s*[-_–]\s*topic$)"), QRegularExpression::CaseInsensitiveOption);
	a.remove(topicRegex);
	return a.trimmed();
}

LrcSong LyricsEngine::loadFromCache(const QString &key) {
	QFile f(LyricsCacheDir() + key + u".json"_q);
	if (!f.open(QIODevice::ReadOnly)) return LrcSong{};

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return LrcSong{};

	const auto root = doc.object();
	LrcSong song;
	song.title = root.value(u"title"_q).toString();
	song.artist = root.value(u"artist"_q).toString();
	song.source = root.value(u"source"_q).toString();
	song.plainLyrics = root.value(u"plain"_q).toString();

	const auto arr = root.value(u"lines"_q).toArray();
	for (const auto &v : arr) {
		const auto o = v.toObject();
		song.lines.push_back(LrcLine{
			.timeMs = o.value(u"t"_q).toInt(),
			.text = o.value(u"x"_q).toString(),
			.translation = o.value(u"tr"_q).toString(),
		});
	}

	return song;
}

void LyricsEngine::saveToCache(const QString &key, const LrcSong &song) {
	QJsonObject root;
	root[u"title"_q] = song.title;
	root[u"artist"_q] = song.artist;
	root[u"source"_q] = song.source;
	root[u"plain"_q] = song.plainLyrics;

	QJsonArray arr;
	for (const auto &line : song.lines) {
		QJsonObject o;
		o[u"t"_q] = line.timeMs;
		o[u"x"_q] = line.text;
		if (!line.translation.isEmpty()) {
			o[u"tr"_q] = line.translation;
		}
		arr.append(o);
	}
	root[u"lines"_q] = arr;

	QFile f(LyricsCacheDir() + key + u".json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

void LyricsEngine::fetchLyrics(
	const QString &title,
	const QString &artist,
	int durationSec,
	Fn<void(const LrcSong &)> callback) {
	const auto cleanT = cleanTitle(title);
	const auto cleanA = cleanArtist(artist);
	if (cleanT.isEmpty()) {
		if (callback) callback(LrcSong{});
		return;
	}

	const auto key = CacheKey(cleanT, cleanA);
	const auto cached = loadFromCache(key);
	if (!cached.isEmpty()) {
		if (callback) callback(cached);
		return;
	}

	fetchFromLrclib(cleanT, cleanA, durationSec, [this, cleanT, cleanA, key, callback](const LrcSong &song) {
		if (!song.isEmpty()) {
			saveToCache(key, song);
			if (callback) callback(song);
		} else {
			fetchFromNetEase(cleanT, cleanA, [this, key, callback](const LrcSong &fallbackSong) {
				if (!fallbackSong.isEmpty()) {
					saveToCache(key, fallbackSong);
				}
				if (callback) callback(fallbackSong);
			});
		}
	});
}

void LyricsEngine::fetchFromLrclib(
	const QString &title,
	const QString &artist,
	int durationSec,
	Fn<void(const LrcSong &)> callback) {
	auto *nam = new QNetworkAccessManager();

	QUrl url(u"https://lrclib.net/api/get"_q);
	QUrlQuery query;
	query.addQueryItem(u"track_name"_q, title);
	if (!artist.isEmpty()) {
		query.addQueryItem(u"artist_name"_q, artist);
	}
	if (durationSec > 0) {
		query.addQueryItem(u"duration"_q, QString::number(durationSec));
	}
	url.setQuery(query);

	QNetworkRequest req(url);
	req.setRawHeader("User-Agent", "Miogram-Desktop/7.0.9 (https://github.com/fuckramochka/miogram-desktop)");

	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() == QNetworkReply::NoError) {
			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (doc.isObject()) {
				const auto obj = doc.object();
				const auto synced = obj.value(u"syncedLyrics"_q).toString();
				const auto plain = obj.value(u"plainLyrics"_q).toString();

				if (!synced.trimmed().isEmpty()) {
					auto s = LrcParser::parse(synced, title, artist, QStringLiteral("LRCLib"));
					s.plainLyrics = plain;
					if (callback) callback(s);
					return;
				} else if (!plain.trimmed().isEmpty()) {
					LrcSong s;
					s.title = title;
					s.artist = artist;
					s.source = QStringLiteral("LRCLib");
					s.plainLyrics = plain;
					if (callback) callback(s);
					return;
				}
			}
		}

		if (callback) callback(LrcSong{});
	});
}

void LyricsEngine::fetchFromNetEase(
	const QString &title,
	const QString &artist,
	Fn<void(const LrcSong &)> callback) {
	auto *nam = new QNetworkAccessManager();

	QUrl url(u"https://music.163.com/api/search/get/web"_q);
	QUrlQuery query;
	query.addQueryItem(u"s"_q, (artist + u" "_q + title).trimmed());
	query.addQueryItem(u"type"_q, u"1"_q);
	query.addQueryItem(u"offset"_q, u"0"_q);
	query.addQueryItem(u"total"_q, u"true"_q);
	query.addQueryItem(u"limit"_q, u"1"_q);
	url.setQuery(query);

	QNetworkRequest req(url);
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [this, title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			if (callback) callback(LrcSong{});
			return;
		}

		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isObject()) {
			if (callback) callback(LrcSong{});
			return;
		}

		const auto res = doc.object().value(u"result"_q).toObject();
		const auto songs = res.value(u"songs"_q).toArray();
		if (songs.isEmpty()) {
			if (callback) callback(LrcSong{});
			return;
		}

		const auto songId = songs.first().toObject().value(u"id"_q).toVariant().toLongLong();
		if (!songId) {
			if (callback) callback(LrcSong{});
			return;
		}

		// Second pass: fetch lyrics by songId
		auto *lyricNam = new QNetworkAccessManager();
		QUrl lyricUrl(QString("https://music.163.com/api/song/lyric?os=pc&id=%1&lv=-1&kv=-1&tv=-1").arg(songId));
		QNetworkRequest lyricReq(lyricUrl);
		QNetworkReply *lyricReply = lyricNam->get(lyricReq);

		QObject::connect(lyricReply, &QNetworkReply::finished, [title, artist, callback, lyricNam, lyricReply] {
			lyricReply->deleteLater();
			lyricNam->deleteLater();

			if (lyricReply->error() == QNetworkReply::NoError) {
				const auto lDoc = QJsonDocument::fromJson(lyricReply->readAll());
				if (lDoc.isObject()) {
					const auto lObj = lDoc.object();
					const auto lrcText = lObj.value(u"lrc"_q).toObject().value(u"lyric"_q).toString();
					if (!lrcText.trimmed().isEmpty()) {
						auto s = LrcParser::parse(lrcText, title, artist, QStringLiteral("NetEase"));
						if (callback) callback(s);
						return;
					}
				}
			}

			if (callback) callback(LrcSong{});
		});
	});
}

} // namespace Miogram
