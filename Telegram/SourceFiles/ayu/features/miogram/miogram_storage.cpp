// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_storage.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDateTime>

#include "ayu/features/miogram/miogram_config.h"
#include "ayu/features/miogram/miogram_vault.h"
#include "core/application.h"

namespace Miogram {

FileVault &FileVault::instance() {
	static FileVault vault;
	return vault;
}

FileVault::FileVault() {
}

void FileVault::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

bool FileVault::isEncryptionAvailable() const {
	return !ProfileVault::instance().databaseKey().isEmpty();
}

bool FileVault::writeEncrypted(
		const QString &path,
		const QByteArray &plaintext,
		QString *error) {
	const auto key = ProfileVault::instance().databaseKey();
	if (key.isEmpty()) {
		return writePlain(path, plaintext, error);
	}
	const auto nonce = AesGcm::randomNonce(12);
	const auto envelope = AesGcm::encrypt(plaintext, key, nonce);
	if (envelope.isEmpty()) {
		if (error) {
			*error = u"Encrypt failed"_q;
		}
		return false;
	}
	return writePlain(path, envelope, error);
}

QByteArray FileVault::readEncrypted(const QString &path, bool *ok) {
	bool plainOk = false;
	const auto raw = readPlain(path, &plainOk);
	if (!plainOk) {
		if (ok) {
			*ok = false;
		}
		return QByteArray();
	}
	const auto key = ProfileVault::instance().databaseKey();
	if (key.isEmpty()) {
		if (ok) {
			*ok = true;
		}
		return raw;
	}
	const auto plain = AesGcm::decrypt(raw, key, QByteArray());
	if (ok) {
		*ok = !plain.isEmpty() || raw.size() == 44;
	}
	return plain;
}

bool FileVault::writePlain(
		const QString &path,
		const QByteArray &data,
		QString *error) {
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		if (error) {
			*error = f.errorString();
		}
		return false;
	}
	f.write(data);
	return true;
}

QByteArray FileVault::readPlain(const QString &path, bool *ok) {
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		if (ok) {
			*ok = false;
		}
		return QByteArray();
	}
	if (ok) {
		*ok = true;
	}
	return f.readAll();
}

HistoryStoragePolicy &HistoryStoragePolicy::instance() {
	static HistoryStoragePolicy policy;
	return policy;
}

HistoryStoragePolicy::HistoryStoragePolicy() {
}

void HistoryStoragePolicy::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_policy = static_cast<RetentionPolicy>(MiogramConfig::instance().intValue(
		ConfigDomain::Privacy,
		u"retention"_q,
		0));
}

RetentionPolicy HistoryStoragePolicy::policy() const {
	return _policy;
}

void HistoryStoragePolicy::setPolicy(RetentionPolicy policy) {
	_policy = policy;
	MiogramConfig::instance().setIntValue(
		ConfigDomain::Privacy,
		u"retention"_q,
		static_cast<int>(policy));
}

bool HistoryStoragePolicy::shouldKeep(qint64 messageDateMs) const {
	if (_policy == RetentionPolicy::Forever) {
		return true;
	}
	const auto now = QDateTime::currentMSecsSinceEpoch();
	const auto ageDays = double(now - messageDateMs) / 86400000.;
	switch (_policy) {
	case RetentionPolicy::OneYear: return ageDays <= 365;
	case RetentionPolicy::SixMonths: return ageDays <= 183;
	case RetentionPolicy::OneMonth: return ageDays <= 31;
	case RetentionPolicy::OneWeek: return ageDays <= 7;
	case RetentionPolicy::Forever:
	default: return true;
	}
}

QString HistoryStoragePolicy::policyTitle(RetentionPolicy policy) const {
	switch (policy) {
	case RetentionPolicy::Forever: return u"Forever"_q;
	case RetentionPolicy::OneYear: return u"1 year"_q;
	case RetentionPolicy::SixMonths: return u"6 months"_q;
	case RetentionPolicy::OneMonth: return u"1 month"_q;
	case RetentionPolicy::OneWeek: return u"1 week"_q;
	}
	return u"Forever"_q;
}

EncryptedKeyValue &EncryptedKeyValue::instance() {
	static EncryptedKeyValue store;
	return store;
}

EncryptedKeyValue::EncryptedKeyValue() {
}

void EncryptedKeyValue::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

void EncryptedKeyValue::put(const QString &key, const QString &value) {
	if (key.trimmed().isEmpty()) {
		return;
	}
	_cache.insert(key, value);
	save();
}

QString EncryptedKeyValue::get(const QString &key, const QString &fallback) const {
	const auto it = _cache.find(key);
	return (it != _cache.end()) ? it.value() : fallback;
}

void EncryptedKeyValue::remove(const QString &key) {
	_cache.remove(key);
	save();
}

void EncryptedKeyValue::clear() {
	_cache.clear();
	save();
}

void EncryptedKeyValue::load() {
	bool ok = false;
	const auto raw = FileVault::instance().readEncrypted(
		cWorkingDir() + u"tdata/mio_kv.bin"_q,
		&ok);
	if (!ok || raw.isEmpty()) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(raw);
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	for (auto it = obj.begin(); it != obj.end(); ++it) {
		_cache.insert(it.key(), it.value().toString());
	}
}

void EncryptedKeyValue::save() {
	QJsonObject obj;
	for (auto it = _cache.begin(); it != _cache.end(); ++it) {
		obj.insert(it.key(), it.value());
	}
	FileVault::instance().writeEncrypted(
		cWorkingDir() + u"tdata/mio_kv.bin"_q,
		QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

} // namespace Miogram
