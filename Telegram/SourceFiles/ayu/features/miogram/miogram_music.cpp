// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_music.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUrlQuery>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Miogram {

MusicSearchEngine &MusicSearchEngine::instance() {
	static MusicSearchEngine engine;
	return engine;
}

MusicSearchEngine::MusicSearchEngine() {
}

void MusicSearchEngine::initialize() {
	if (_initialized) return;
	_initialized = true;
}

QString MusicSearchEngine::normalizeForDedup(const QString &str) {
	static const QRegularExpression nonAlpha(QStringLiteral(R"([^a-zA-Z0-9\x{0400}-\x{04FF}])"));
	QString s = str.toLower();
	s.remove(nonAlpha);
	return s;
}

void MusicSearchEngine::search(const QString &query, Fn<void(std::vector<MusicTrack>)> callback) {
	const auto q = query.trimmed();
	if (q.isEmpty()) {
		if (callback) callback({});
		return;
	}

	struct SearchContext {
		std::vector<MusicTrack> aggregated;
		QSet<QString> seenSignatures;
		int pending = 2;
		Fn<void(std::vector<MusicTrack>)> userCallback;
	};

	auto ctx = std::make_shared<SearchContext>();
	ctx->userCallback = callback;

	auto addTracks = [ctx](const std::vector<MusicTrack> &tracks) {
		for (const auto &t : tracks) {
			const auto sig = normalizeForDedup(t.artist) + u"|"_q + normalizeForDedup(t.title);
			if (!ctx->seenSignatures.contains(sig)) {
				ctx->seenSignatures.insert(sig);
				ctx->aggregated.push_back(t);
			}
		}
		ctx->pending--;
		if (ctx->pending == 0 && ctx->userCallback) {
			ctx->userCallback(ctx->aggregated);
		}
	};

	searchDeezer(q, addTracks);
	searchItunes(q, addTracks);
}

void MusicSearchEngine::searchDeezer(const QString &query, Fn<void(std::vector<MusicTrack>)> callback) {
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://api.deezer.com/search"_q);
	QUrlQuery q;
	q.addQueryItem(u"q"_q, query);
	q.addQueryItem(u"limit"_q, u"25"_q);
	url.setQuery(q);

	QNetworkRequest req(url);
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		std::vector<MusicTrack> out;
		if (reply->error() == QNetworkReply::NoError) {
			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (doc.isObject()) {
				const auto arr = doc.object().value(u"data"_q).toArray();
				out.reserve(arr.size());
				for (const auto &val : arr) {
					if (!val.isObject()) continue;
					const auto obj = val.toObject();

					MusicTrack t;
					t.id = QString("deezer_%1").arg(obj.value(u"id"_q).toVariant().toLongLong());
					t.title = obj.value(u"title"_q).toString();
					t.artist = obj.value(u"artist"_q).toObject().value(u"name"_q).toString(QStringLiteral("Unknown"));
					t.album = obj.value(u"album"_q).toObject().value(u"title"_q).toString();
					t.durationSec = obj.value(u"duration"_q).toInt();
					t.previewAudioUrl = obj.value(u"preview"_q).toString();
					t.coverArtworkUrl = obj.value(u"album"_q).toObject().value(u"cover_medium"_q).toString();
					t.source = QStringLiteral("deezer");

					if (!t.title.isEmpty()) {
						out.push_back(t);
					}
				}
			}
		}
		if (callback) callback(out);
	});
}

void MusicSearchEngine::searchItunes(const QString &query, Fn<void(std::vector<MusicTrack>)> callback) {
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://itunes.apple.com/search"_q);
	QUrlQuery q;
	q.addQueryItem(u"media"_q, u"music"_q);
	q.addQueryItem(u"entity"_q, u"song"_q);
	q.addQueryItem(u"term"_q, query);
	q.addQueryItem(u"limit"_q, u"25"_q);
	url.setQuery(q);

	QNetworkRequest req(url);
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		std::vector<MusicTrack> out;
		if (reply->error() == QNetworkReply::NoError) {
			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (doc.isObject()) {
				const auto arr = doc.object().value(u"results"_q).toArray();
				out.reserve(arr.size());
				for (const auto &val : arr) {
					if (!val.isObject()) continue;
					const auto obj = val.toObject();

					MusicTrack t;
					t.id = QString("itunes_%1").arg(obj.value(u"trackId"_q).toVariant().toLongLong());
					t.title = obj.value(u"trackName"_q).toString();
					t.artist = obj.value(u"artistName"_q).toString(QStringLiteral("Unknown"));
					t.album = obj.value(u"collectionName"_q).toString();
					t.durationSec = obj.value(u"trackTimeMillis"_q).toInt() / 1000;
					t.previewAudioUrl = obj.value(u"previewUrl"_q).toString();
					t.coverArtworkUrl = obj.value(u"artworkUrl100"_q).toString().replace(u"100x100bb"_q, u"600x600bb"_q);
					t.source = QStringLiteral("itunes");

					if (!t.title.isEmpty()) {
						out.push_back(t);
					}
				}
			}
		}
		if (callback) callback(out);
	});
}

} // namespace Miogram
