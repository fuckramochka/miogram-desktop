// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_cloudvault.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>

#include "ayu/features/miogram/miogram_config.h"
#include "ayu/features/miogram/miogram_vault.h"
#include "core/application.h"

namespace Miogram {

CloudVaultEngine &CloudVaultEngine::instance() {
	static CloudVaultEngine engine;
	return engine;
}

CloudVaultEngine::CloudVaultEngine() {
}

void CloudVaultEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_enabled = MiogramConfig::instance().boolValue(
		ConfigDomain::Privacy,
		u"cloudVault"_q,
		false);
	loadCache();
	QDir().mkpath(storageRoot());
}

bool CloudVaultEngine::isEnabled() const {
	return _enabled;
}

void CloudVaultEngine::setEnabled(bool enabled) {
	_enabled = enabled;
	MiogramConfig::instance().setBoolValue(
		ConfigDomain::Privacy,
		u"cloudVault"_q,
		enabled);
}

std::vector<CloudVaultFile> CloudVaultEngine::files() const {
	return _cache;
}

QString CloudVaultEngine::storageRoot() const {
	const auto dir = cWorkingDir() + u"tdata/mio_cloudvault/"_q;
	return dir;
}

void CloudVaultEngine::refresh(Fn<void(std::vector<CloudVaultFile>)> callback) {
	loadCache();
	if (callback) {
		callback(_cache);
	}
}

void CloudVaultEngine::upload(
		const QString &localPath,
		Fn<void(CloudVaultFile)> callback) {
	QFileInfo info(localPath);
	if (!info.isFile()) {
		if (callback) {
			callback(CloudVaultFile{});
		}
		return;
	}
	QFile src(localPath);
	if (!src.open(QIODevice::ReadOnly)) {
		if (callback) {
			callback(CloudVaultFile{});
		}
		return;
	}
	const auto plain = src.readAll();
	const auto key = ProfileVault::instance().databaseKey();
	CloudVaultFile file;
	file.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	file.name = info.fileName();
	file.size = info.size();
	file.updatedAt = QDateTime::currentMSecsSinceEpoch();
	const auto nonce = AesGcm::randomNonce(12);
	const auto envelope = key.isEmpty()
		? plain
		: AesGcm::encrypt(plain, key, nonce);
	file.encrypted = !key.isEmpty();
	file.localPath = storageRoot() + file.id + u".mio"_q;
	QFile dst(file.localPath);
	if (dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		dst.write(envelope);
	}
	_cache.push_back(file);
	saveCache();
	if (callback) {
		callback(file);
	}
}

void CloudVaultEngine::download(
		const QString &fileId,
		Fn<void(QString localPath)> callback) {
	for (const auto &file : _cache) {
		if (file.id == fileId) {
			QFile src(file.localPath);
			if (!src.open(QIODevice::ReadOnly)) {
				break;
			}
			const auto envelope = src.readAll();
			const auto key = ProfileVault::instance().databaseKey();
			const auto plain = (key.isEmpty() || !file.encrypted)
				? envelope
				: AesGcm::decrypt(envelope, key, QByteArray());
			const auto outPath = storageRoot() + file.name;
			QFile dst(outPath);
			if (dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
				dst.write(plain);
			}
			if (callback) {
				callback(outPath);
			}
			return;
		}
	}
	if (callback) {
		callback(QString());
	}
}

void CloudVaultEngine::remove(const QString &fileId) {
	for (auto it = _cache.begin(); it != _cache.end(); ++it) {
		if (it->id == fileId) {
			QFile::remove(it->localPath);
			_cache.erase(it);
			break;
		}
	}
	saveCache();
}

void CloudVaultEngine::loadCache() {
	QFile f(cWorkingDir() + u"tdata/mio_cloudvault.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isArray()) {
		return;
	}
	_cache.clear();
	for (const auto &v : doc.array()) {
		const auto o = v.toObject();
		CloudVaultFile file;
		file.id = o.value(u"id"_q).toString();
		file.name = o.value(u"name"_q).toString();
		file.mimeType = o.value(u"mime"_q).toString();
		file.size = o.value(u"size"_q).toVariant().toLongLong();
		file.localPath = o.value(u"path"_q).toString();
		file.updatedAt = o.value(u"updated"_q).toVariant().toLongLong();
		file.encrypted = o.value(u"enc"_q).toBool(true);
		if (!file.id.isEmpty()) {
			_cache.push_back(file);
		}
	}
}

void CloudVaultEngine::saveCache() {
	QJsonArray arr;
	for (const auto &file : _cache) {
		QJsonObject o;
		o.insert(u"id"_q, file.id);
		o.insert(u"name"_q, file.name);
		o.insert(u"mime"_q, file.mimeType);
		o.insert(u"size"_q, file.size);
		o.insert(u"path"_q, file.localPath);
		o.insert(u"updated"_q, file.updatedAt);
		o.insert(u"enc"_q, file.encrypted);
		arr.append(o);
	}
	QFile f(cWorkingDir() + u"tdata/mio_cloudvault.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
