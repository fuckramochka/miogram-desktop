// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include "base/basic_types.h"

namespace Miogram {

enum class RetentionPolicy {
	Forever = 0,
	OneYear = 1,
	SixMonths = 2,
	OneMonth = 3,
	OneWeek = 4,
};

class FileVault {
public:
	static FileVault &instance();

	void initialize();

	bool writeEncrypted(
		const QString &path,
		const QByteArray &plaintext,
		QString *error = nullptr);
	QByteArray readEncrypted(
		const QString &path,
		bool *ok = nullptr);

	bool writePlain(
		const QString &path,
		const QByteArray &data,
		QString *error = nullptr);
	QByteArray readPlain(const QString &path, bool *ok = nullptr);

	[[nodiscard]] bool isEncryptionAvailable() const;

private:
	FileVault();

	bool _initialized = false;
};

class HistoryStoragePolicy {
public:
	static HistoryStoragePolicy &instance();

	void initialize();

	[[nodiscard]] RetentionPolicy policy() const;
	void setPolicy(RetentionPolicy policy);

	[[nodiscard]] bool shouldKeep(qint64 messageDateMs) const;
	[[nodiscard]] QString policyTitle(RetentionPolicy policy) const;

private:
	HistoryStoragePolicy();

	RetentionPolicy _policy = RetentionPolicy::Forever;
	bool _initialized = false;
};

class EncryptedKeyValue {
public:
	static EncryptedKeyValue &instance();

	void initialize();

	void put(const QString &key, const QString &value);
	[[nodiscard]] QString get(const QString &key, const QString &fallback = QString()) const;
	void remove(const QString &key);
	void clear();

private:
	EncryptedKeyValue();
	void load();
	void save();

	QMap<QString, QString> _cache;
	bool _initialized = false;
};

} // namespace Miogram
