// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_lyrics_extra.h"

#include <QtCore/QUrlQuery>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "ayu/features/miogram/miogram_ai.h"
#include "ayu/features/miogram/miogram_config.h"

namespace Miogram {

ExtendedLyricsSources &ExtendedLyricsSources::instance() {
	static ExtendedLyricsSources sources;
	return sources;
}

ExtendedLyricsSources::ExtendedLyricsSources() {
}

void ExtendedLyricsSources::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	LyricsEngine::instance().initialize();
	AiService::instance().initialize();
}

void ExtendedLyricsSources::fetchWithSource(
		LyricsSource source,
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback) {
	switch (source) {
	case LyricsSource::Lrclib:
	case LyricsSource::NetEase:
		LyricsEngine::instance().fetchLyrics(title, artist, durationSec, callback);
		return;
	case LyricsSource::Yandex:
		fetchFromYandex(title, artist, callback);
		return;
	case LyricsSource::Genius:
		fetchFromGenius(title, artist, callback);
		return;
	case LyricsSource::YouTube:
		fetchFromYouTube(title, artist, callback);
		return;
	case LyricsSource::Ai:
		fetchFromAi(title, artist, callback);
		return;
	case LyricsSource::Server:
		fetchFromServer(title, artist, durationSec, callback);
		return;
	case LyricsSource::Auto:
	default:
		fetchChained(title, artist, durationSec, callback);
		return;
	}
}

void ExtendedLyricsSources::fetchChained(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback) {
	LyricsEngine::instance().fetchLyrics(
		title,
		artist,
		durationSec,
		[this, title, artist, durationSec, callback](const LrcSong &song) {
			if (!song.isEmpty()) {
				if (callback) {
					callback(song);
				}
				return;
			}
			fetchFromServer(title, artist, durationSec, [this, title, artist, callback](const LrcSong &server) {
				if (!server.isEmpty()) {
					if (callback) {
						callback(server);
					}
					return;
				}
				fetchFromYandex(title, artist, [this, title, artist, callback](const LrcSong &yandex) {
					if (!yandex.isEmpty()) {
						if (callback) {
							callback(yandex);
						}
						return;
					}
					fetchFromGenius(title, artist, [this, title, artist, callback](const LrcSong &genius) {
						if (!genius.isEmpty()) {
							if (callback) {
								callback(genius);
							}
							return;
						}
						fetchFromYouTube(title, artist, [this, title, artist, callback](const LrcSong &yt) {
							if (!yt.isEmpty()) {
								if (callback) {
									callback(yt);
								}
								return;
							}
							fetchFromAi(title, artist, callback);
						});
					});
				});
			});
		});
}

void ExtendedLyricsSources::fetchFromYandex(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback) {
// Yandex Music has no public unauthenticated lyrics endpoint, so this
// source performs a best-effort track search and returns empty when the
// API requires OAuth. The empty result keeps the Auto chain moving to
// Genius/YouTube/AI instead of stalling or fabricating lyrics.
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://music.yandex.ru/handlers/music-search.jsx"_q);
	QUrlQuery query;
	query.addQueryItem(u"text"_q, (artist + u" "_q + title).trimmed());
	query.addQueryItem(u"type"_q, u"tracks"_q);
	url.setQuery(query);
	QNetworkRequest req(url);
	req.setRawHeader("User-Agent", "Miogram-Desktop/7.0.9");
	req.setRawHeader("Accept", "application/json");
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		if (callback) {
			callback(LrcSong{});
		}
	});
}

void ExtendedLyricsSources::fetchFromGenius(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback) {
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Ai);
	const auto token = domain.value(u"geniusToken"_q).toString().trimmed();
	if (token.isEmpty()) {
		if (callback) {
			callback(LrcSong{});
		}
		return;
	}
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://api.genius.com/search"_q);
	QUrlQuery query;
	query.addQueryItem(u"q"_q, (artist + u" "_q + title).trimmed());
	url.setQuery(query);
	QNetworkRequest req(url);
	req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		const auto doc = QJsonDocument::fromJson(reply->readAll());
		const auto hits = doc.object()
			.value(u"response"_q).toObject()
			.value(u"hits"_q).toArray();
		if (hits.isEmpty()) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		LrcSong song;
		song.title = title;
		song.artist = artist;
		song.source = u"Genius"_q;
		song.plainLyrics = hits.first().toObject()
			.value(u"result"_q).toObject()
			.value(u"url"_q).toString();
		if (callback) {
			callback(song);
		}
	});
}

void ExtendedLyricsSources::fetchFromYouTube(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback) {
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://www.youtube.com/results"_q);
	QUrlQuery query;
	query.addQueryItem(u"search_query"_q, (artist + u" "_q + title + u" lyrics"_q).trimmed());
	url.setQuery(query);
	QNetworkRequest req(url);
	req.setRawHeader("User-Agent", "Miogram-Desktop/7.0.9");
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		const auto html = QString::fromUtf8(reply->readAll());
		static const QRegularExpression videoRe(u"\"videoId\":\"([a-zA-Z0-9_-]{11})\""_q);
		const auto match = videoRe.match(html);
		if (!match.hasMatch()) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		const auto videoId = match.captured(1);
		auto *capNam = new QNetworkAccessManager();
		QUrl capUrl(u"https://video.google.com/timedtext"_q);
		QUrlQuery capQuery;
		capQuery.addQueryItem(u"v"_q, videoId);
		capQuery.addQueryItem(u"lang"_q, u"en"_q);
		capUrl.setQuery(capQuery);
		QNetworkRequest capReq(capUrl);
		QNetworkReply *capReply = capNam->get(capReq);
		QObject::connect(capReply, &QNetworkReply::finished, [title, artist, callback, capNam, capReply] {
			capReply->deleteLater();
			capNam->deleteLater();
			if (capReply->error() != QNetworkReply::NoError) {
				if (callback) {
					callback(LrcSong{});
				}
				return;
			}
			const auto xml = QString::fromUtf8(capReply->readAll());
			if (!xml.contains(u"<text"_q)) {
				if (callback) {
					callback(LrcSong{});
				}
				return;
			}
			static const QRegularExpression lineRe(
				u"<text start=\"([0-9.]+)\" dur=\"[0-9.]+\">([^<]*)</text>"_q);
			QString lrc;
			auto it = lineRe.globalMatch(xml);
			while (it.hasNext()) {
				const auto m = it.next();
				const auto startSec = m.captured(1).toDouble();
				const auto ms = int(startSec * 1000);
				const auto min = ms / 60000;
				const auto sec = (ms % 60000) / 1000;
				const auto cent = (ms % 1000) / 10;
				lrc += QString(u"[%1:%2.%3]%4\n"_q)
					.arg(min, 2, 10, QChar(u'0'))
					.arg(sec, 2, 10, QChar(u'0'))
					.arg(cent, 2, 10, QChar(u'0'))
					.arg(m.captured(2).trimmed());
			}
			if (lrc.trimmed().isEmpty()) {
				if (callback) {
					callback(LrcSong{});
				}
				return;
			}
			if (callback) {
				callback(LrcParser::parse(lrc, title, artist, u"YouTube"_q));
			}
		});
	});
}

void ExtendedLyricsSources::fetchFromAi(
		const QString &title,
		const QString &artist,
		Fn<void(const LrcSong &)> callback) {
	if (!AiService::instance().hasApiKey()) {
		if (callback) {
			callback(LrcSong{});
		}
		return;
	}
	const auto prompt = u"Відтвори точний текст пісні \""_q + title
		+ u"\" виконавця \""_q + artist
		+ u"\". Якщо не знаєш — поверни порожньо, не вигадуй."_q;
	AiService::instance().generate(
		prompt,
		QString(),
		[title, artist, callback](const QString &result, const QString &error) {
			if (!error.isEmpty() || result.trimmed().isEmpty()) {
				if (callback) {
					callback(LrcSong{});
				}
				return;
			}
			LrcSong song;
			song.title = title;
			song.artist = artist;
			song.source = u"AI"_q;
			song.plainLyrics = result.trimmed().left(20000);
			if (callback) {
				callback(song);
			}
		});
}

void ExtendedLyricsSources::fetchFromServer(
		const QString &title,
		const QString &artist,
		int durationSec,
		Fn<void(const LrcSong &)> callback) {
	Q_UNUSED(durationSec);
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://miogram.app/api/lyrics"_q);
	QUrlQuery query;
	query.addQueryItem(u"title"_q, title);
	query.addQueryItem(u"artist"_q, artist);
	url.setQuery(query);
	QNetworkRequest req(url);
	req.setTransferTimeout(6000);
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [title, artist, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isObject()) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		const auto obj = doc.object();
		const auto synced = obj.value(u"synced"_q).toString();
		const auto plain = obj.value(u"plain"_q).toString();
		if (synced.trimmed().isEmpty() && plain.trimmed().isEmpty()) {
			if (callback) {
				callback(LrcSong{});
			}
			return;
		}
		if (!synced.trimmed().isEmpty()) {
			if (callback) {
				callback(LrcParser::parse(synced, title, artist, u"Server"_q));
			}
			return;
		}
		LrcSong song;
		song.title = title;
		song.artist = artist;
		song.source = u"Server"_q;
		song.plainLyrics = plain;
		if (callback) {
			callback(song);
		}
	});
}

} // namespace Miogram
