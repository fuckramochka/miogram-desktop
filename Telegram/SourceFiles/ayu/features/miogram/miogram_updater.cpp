// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_updater.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QStandardPaths>

#include "core/application.h"

namespace Miogram {

MiogramUpdater &MiogramUpdater::instance() {
	static MiogramUpdater updater;
	return updater;
}

MiogramUpdater::MiogramUpdater() {
}

void MiogramUpdater::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

QString MiogramUpdater::latestReleaseApi() {
	return u"https://api.github.com/repos/fuckramochka/miogram/releases/latest"_q;
}

qint64 MiogramUpdater::checkIntervalMs() {
	return 24LL * 60 * 60 * 1000;
}

QString MiogramUpdater::currentAppVersion() {
	return u"7.0.9"_q;
}

void MiogramUpdater::initAutoUpdate() {
	if (_autoStarted) {
		return;
	}
	_autoStarted = true;
	QFile f(cWorkingDir() + u"tdata/mio_updater.json"_q);
	qint64 lastCheck = 0;
	if (f.open(QIODevice::ReadOnly)) {
		const auto doc = QJsonDocument::fromJson(f.readAll());
		lastCheck = doc.object().value(u"lastCheck"_q).toVariant().toLongLong();
	}
	const auto now = QDateTime::currentMSecsSinceEpoch();
	if (now - lastCheck < checkIntervalMs()) {
		return;
	}
	QFile out(cWorkingDir() + u"tdata/mio_updater.json"_q);
	if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QJsonObject obj;
		obj.insert(u"lastCheck"_q, now);
		out.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
	fetchLatestRelease(nullptr);
}

void MiogramUpdater::checkOnEntry() {
	fetchLatestRelease(nullptr);
}

void MiogramUpdater::checkAndShow(Fn<void(UpdateInfo)> callback, bool manualCheck) {
	Q_UNUSED(manualCheck);
	fetchLatestRelease(callback);
}

void MiogramUpdater::fetchLatestRelease(Fn<void(UpdateInfo)> callback) {
	auto *nam = new QNetworkAccessManager();
	QNetworkRequest req(QUrl(latestReleaseApi()));
	req.setRawHeader("Accept", "application/vnd.github.v3+json");
	req.setRawHeader(
		"User-Agent",
		"Miogram-Desktop/7.0.9 (https://github.com/fuckramochka/miogram-desktop)");
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		UpdateInfo info;
		info.currentVersion = currentAppVersion();
		info.version = info.currentVersion;
		if (reply->error() == QNetworkReply::NoError) {
			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (doc.isObject()) {
				const auto obj = doc.object();
				const auto tag = obj.value(u"tag_name"_q).toString();
				const auto body = obj.value(u"body"_q).toString();
				QString assetUrl;
				for (const auto &v : obj.value(u"assets"_q).toArray()) {
					const auto name = v.toObject().value(u"name"_q).toString();
					if (name.endsWith(u".exe"_q)
						|| name.endsWith(u".zip"_q)
						|| name.endsWith(u".AppImage"_q)) {
						assetUrl = v.toObject().value(u"browser_download_url"_q).toString();
						break;
					}
				}
				const auto clean = tag.trimmed().remove(u'v').remove(u'V').trimmed();
				info.version = clean.isEmpty() ? info.currentVersion : clean;
				info.changelog = body;
				info.apkUrl = assetUrl;
				info.hasUpdate = isNewerVersion(info.currentVersion, info.version, tag, body);
			}
		}
		if (callback) {
			callback(info);
		}
	});
}

bool MiogramUpdater::isNewerVersion(
		const QString &currentVersion,
		const QString &remoteVersion,
		const QString &remoteTag,
		const QString &changelog) {
// Version comparison mirrors the mobile updater exactly: strip v/V
// prefixes, reject identical or prefix-matching builds, ignore a
// remote whose changelog already mentions the running commit hash,
// then compare numeric components one by one, so desktop prompts only
// for genuinely newer releases and never loops on the same build.
	if (remoteVersion.trimmed().isEmpty()) {
		return false;
	}
	auto c = currentVersion;
	c.remove(u'v').remove(u'V');
	c = c.trimmed();
	auto r = remoteVersion.trimmed();
	if (c.compare(r, Qt::CaseInsensitive) == 0) {
		return false;
	}
	if (!c.isEmpty() && !r.isEmpty() && (c.startsWith(r) || r.startsWith(c))) {
		return false;
	}
	if (!changelog.isEmpty() && !c.isEmpty()) {
		const auto parts = c.split(u'-');
		if (parts.size() > 1) {
			const auto hash = parts.last().trimmed();
			if (hash.size() >= 4 && changelog.contains(hash)) {
				return false;
			}
		}
	}
	Q_UNUSED(remoteTag);
	const auto splitNums = [](const QString &v) {
		std::vector<int> out;
		for (const auto &part : v.split(QRegularExpression(u"[^0-9]+"_q))) {
			if (!part.isEmpty()) {
				out.push_back(part.toInt());
			}
		}
		return out;
	};
	const auto cParts = splitNums(c);
	const auto rParts = splitNums(r);
	const auto len = std::max(cParts.size(), rParts.size());
	for (size_t i = 0; i < len; ++i) {
		const auto cVal = (i < cParts.size()) ? cParts[i] : 0;
		const auto rVal = (i < rParts.size()) ? rParts[i] : 0;
		if (rVal > cVal) {
			return true;
		}
		if (rVal < cVal) {
			return false;
		}
	}
	return false;
}

DownloadManager &DownloadManager::instance() {
	static DownloadManager manager;
	return manager;
}

DownloadManager::DownloadManager() {
}

void DownloadManager::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

void DownloadManager::download(
		const QString &url,
		Fn<void(QString localPath, QString error)> callback) {
	if (_busy) {
		if (callback) {
			callback(QString(), u"Busy"_q);
		}
		return;
	}
	_busy = true;
	auto *nam = new QNetworkAccessManager();
	QNetworkRequest req(QUrl(url));
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [this, callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		_busy = false;
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) {
				callback(QString(), reply->errorString());
			}
			return;
		}
		const auto dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		QDir().mkpath(dir);
		const auto path = dir + u"/miogram-update.bin"_q;
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			if (callback) {
				callback(QString(), u"Cannot write file"_q);
			}
			return;
		}
		f.write(reply->readAll());
		if (callback) {
			callback(path, QString());
		}
	});
}

void DownloadManager::cancel() {
	_busy = false;
}

} // namespace Miogram
