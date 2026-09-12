// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct VaultUnlockResult {
	bool success = false;
	bool duress = false;
	QString error;
};

class AesGcm {
public:
	[[nodiscard]] static QByteArray encrypt(
		const QByteArray &plaintext,
		const QByteArray &key,
		const QByteArray &nonce);
	[[nodiscard]] static QByteArray decrypt(
		const QByteArray &ciphertext,
		const QByteArray &key,
		const QByteArray &nonce);
	[[nodiscard]] static QByteArray randomNonce(int size = 12);
	[[nodiscard]] static QByteArray randomKey(int size = 32);
};

class MiogramKdf {
public:
	[[nodiscard]] static QByteArray derive(
		const QByteArray &password,
		const QByteArray &salt,
		int iterations = 600000,
		int keyLength = 32);
	[[nodiscard]] static QByteArray randomSalt(int size = 16);
	[[nodiscard]] static bool timingSafeEqual(
		const QByteArray &a,
		const QByteArray &b);
};

class ProfileVault {
public:
	static ProfileVault &instance();

	void initialize();

	[[nodiscard]] bool hasVault() const;
	[[nodiscard]] bool isUnlocked() const;
	[[nodiscard]] bool isDuressUnlocked() const;

	VaultUnlockResult setup(
		const QString &pin,
		const QString &duressPin);
	VaultUnlockResult unlock(const QString &pin);
	void lock();
	void zeroizeNow();

	[[nodiscard]] QByteArray databaseKey() const;
	[[nodiscard]] QByteArray metadataKey() const;

	[[nodiscard]] bool verifyPin(const QString &pin) const;
	[[nodiscard]] bool verifyDuressPin(const QString &pin) const;

	void changePin(const QString &oldPin, const QString &newPin);

private:
	ProfileVault();
	void loadMeta();
	void saveMeta();

	QByteArray _salt;
	QByteArray _duressSalt;
	QByteArray _pinHash;
	QByteArray _duressHash;
	QByteArray _wrappedDbKey;
	QByteArray _dbKeyPlain;
	bool _unlocked = false;
	bool _duress = false;
	bool _initialized = false;
	bool _hasVault = false;
};

class MiogramGate {
public:
	static MiogramGate &instance();

	void initialize();

	[[nodiscard]] bool isLocked() const;
	[[nodiscard]] bool isDecoyMode() const;

	bool unlock(const QString &pin, QString *error = nullptr);
	void lock();
	void enterDecoy();

	[[nodiscard]] bool isDialogAllowed(quint64 dialogId) const;
	[[nodiscard]] bool isNotificationAllowed(quint64 dialogId) const;
	[[nodiscard]] bool isSearchAllowed(quint64 dialogId) const;

	void setHiddenDialogs(const QVector<quint64> &ids);
	[[nodiscard]] QVector<quint64> hiddenDialogs() const;

private:
	MiogramGate();
	void loadHidden();

	QVector<quint64> _hidden;
	bool _locked = true;
	bool _decoy = false;
	bool _initialized = false;
};

class DoubleBottomManager {
public:
	static DoubleBottomManager &instance();

	void initialize();

	[[nodiscard]] bool isDoubleBottomEnabled() const;
	void setDoubleBottomEnabled(bool enabled);

	[[nodiscard]] bool isDuressConfigured() const;
	void configureDuress(const QString &pin, const QString &duressPin);

	[[nodiscard]] bool validateAndUnlock(
		const QString &pin,
		bool *outDuress = nullptr);

	void onEnterBackground();
	void onEnterForeground();

private:
	DoubleBottomManager();

	bool _enabled = false;
	bool _initialized = false;
};

void SecureZeroMemory(QByteArray &data);
void SecureZeroString(QString &data);

} // namespace Miogram
