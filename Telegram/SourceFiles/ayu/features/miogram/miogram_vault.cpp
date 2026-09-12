// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_vault.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QRandomGenerator>
#include <QtCore/QMessageAuthenticationCode>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

namespace {

QString VaultMetaPath() {
	return cWorkingDir() + u"tdata/mio_vault.json"_q;
}

QByteArray XorWithKey(const QByteArray &data, const QByteArray &key) {
	auto out = data;
	for (int i = 0; i < out.size(); ++i) {
		out[i] = char(out[i] ^ key[i % key.size()]);
	}
	return out;
}

} // namespace

void SecureZeroMemory(QByteArray &data) {
	data.fill(0);
	data.clear();
	data.squeeze();
}

void SecureZeroString(QString &data) {
	data.fill(QChar(0));
	data.clear();
	data.squeeze();
}

QByteArray AesGcm::randomNonce(int size) {
	QByteArray out;
	out.resize(size);
	for (int i = 0; i < size; ++i) {
		out[i] = char(QRandomGenerator::system()->bounded(256));
	}
	return out;
}

QByteArray AesGcm::randomKey(int size) {
	return randomNonce(size);
}

QByteArray AesGcm::encrypt(
		const QByteArray &plaintext,
		const QByteArray &key,
		const QByteArray &nonce) {
// The desktop vault envelope uses HMAC-SHA256 authentication over
// nonce || ciphertext, where the ciphertext itself is a SHA256-CTR
// keystream XOR. The nonce is prepended to the output together with
// a 32-byte tag, so decrypt() can verify integrity in constant time
// before releasing any plaintext, and a tampered file never unlocks.
	if (key.isEmpty() || plaintext.isEmpty()) {
		return QByteArray();
	}
	QByteArray keystreamInput = nonce + key;
	QByteArray keystream;
	keystream.reserve(plaintext.size());
	int counter = 0;
	while (keystream.size() < plaintext.size()) {
		const auto block = QCryptographicHash::hash(
			keystreamInput + QByteArray::number(counter),
			QCryptographicHash::Sha256);
		keystream.append(block);
		++counter;
	}
	QByteArray cipher(plaintext.size(), 0);
	for (int i = 0; i < plaintext.size(); ++i) {
		cipher[i] = char(plaintext[i] ^ keystream[i]);
	}
	const auto tag = QMessageAuthenticationCode::hash(
		nonce + cipher,
		key,
		QCryptographicHash::Sha256);
	return nonce + tag + cipher;
}

QByteArray AesGcm::decrypt(
		const QByteArray &envelope,
		const QByteArray &key,
		const QByteArray &nonce) {
	if (key.isEmpty() || envelope.size() < 12 + 32) {
		return QByteArray();
	}
	const auto usedNonce = envelope.mid(0, 12);
	const auto tag = envelope.mid(12, 32);
	const auto cipher = envelope.mid(44);
	if (!nonce.isEmpty() && usedNonce != nonce) {
		return QByteArray();
	}
	const auto expected = QMessageAuthenticationCode::hash(
		usedNonce + cipher,
		key,
		QCryptographicHash::Sha256);
	if (!MiogramKdf::timingSafeEqual(tag, expected)) {
		return QByteArray();
	}
	QByteArray keystreamInput = usedNonce + key;
	QByteArray keystream;
	keystream.reserve(cipher.size());
	int counter = 0;
	while (keystream.size() < cipher.size()) {
		const auto block = QCryptographicHash::hash(
			keystreamInput + QByteArray::number(counter),
			QCryptographicHash::Sha256);
		keystream.append(block);
		++counter;
	}
	QByteArray plain(cipher.size(), 0);
	for (int i = 0; i < cipher.size(); ++i) {
		plain[i] = char(cipher[i] ^ keystream[i]);
	}
	return plain;
}

QByteArray MiogramKdf::randomSalt(int size) {
	QByteArray out;
	out.resize(size);
	for (int i = 0; i < size; ++i) {
		out[i] = char(QRandomGenerator::system()->bounded(256));
	}
	return out;
}

QByteArray MiogramKdf::derive(
		const QByteArray &password,
		const QByteArray &salt,
		int iterations,
		int keyLength) {
// PBKDF2-HMAC-SHA256 with a high default iteration count is used as
// the portable desktop counterpart of the mobile Argon2id KDF. Each
// block mixes salt || counter through repeated HMAC rounds, XORing
// every intermediate digest, which forces an attacker to pay the full
// iteration cost per guess while verification stays fast enough for
// interactive unlock. Salts are always random per vault.
	QByteArray out;
	int blockIndex = 1;
	while (out.size() < keyLength) {
		auto u = QMessageAuthenticationCode::hash(
			salt + QByteArray::fromRawData(
				reinterpret_cast<const char*>(&blockIndex),
				sizeof(blockIndex)),
			password,
			QCryptographicHash::Sha256);
		auto acc = u;
		for (int i = 1; i < iterations; ++i) {
			u = QMessageAuthenticationCode::hash(u, password, QCryptographicHash::Sha256);
			for (int j = 0; j < acc.size(); ++j) {
				acc[j] = char(acc[j] ^ u[j]);
			}
		}
		out.append(acc);
		++blockIndex;
	}
	return out.left(keyLength);
}

bool MiogramKdf::timingSafeEqual(const QByteArray &a, const QByteArray &b) {
	if (a.size() != b.size()) {
		return false;
	}
	volatile unsigned char diff = 0;
	for (int i = 0; i < a.size(); ++i) {
		diff |= static_cast<unsigned char>(a[i] ^ b[i]);
	}
	return diff == 0;
}

ProfileVault &ProfileVault::instance() {
	static ProfileVault vault;
	return vault;
}

ProfileVault::ProfileVault() {
}

void ProfileVault::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	loadMeta();
}

bool ProfileVault::hasVault() const {
	return _hasVault;
}

bool ProfileVault::isUnlocked() const {
	return _unlocked && !_dbKeyPlain.isEmpty();
}

bool ProfileVault::isDuressUnlocked() const {
	return _unlocked && _duress;
}

VaultUnlockResult ProfileVault::setup(const QString &pin, const QString &duressPin) {
	if (pin.trimmed().size() < 4) {
		return { false, false, u"PIN too short"_q };
	}
	if (!duressPin.isEmpty() && duressPin.trimmed() == pin.trimmed()) {
		return { false, false, u"Duress PIN must differ"_q };
	}
	_salt = MiogramKdf::randomSalt(16);
	_duressSalt = MiogramKdf::randomSalt(16);
	const auto pinKey = MiogramKdf::derive(pin.toUtf8(), _salt);
	const auto dbKey = AesGcm::randomKey(32);
	_wrappedDbKey = XorWithKey(dbKey, pinKey);
	_pinHash = QCryptographicHash::hash(pinKey, QCryptographicHash::Sha256);
	if (!duressPin.isEmpty()) {
		const auto duressKey = MiogramKdf::derive(duressPin.toUtf8(), _duressSalt);
		_duressHash = QCryptographicHash::hash(duressKey, QCryptographicHash::Sha256);
	} else {
		_duressHash = QByteArray();
	}
	_dbKeyPlain = dbKey;
	_unlocked = true;
	_duress = false;
	_hasVault = true;
	SecureZeroMemory(const_cast<QByteArray&>(pinKey));
	SecureZeroMemory(dbKey);
	saveMeta();
	return { true, false, QString() };
}

VaultUnlockResult ProfileVault::unlock(const QString &pin) {
	if (!_hasVault) {
		return { false, false, u"No vault"_q };
	}
	const auto pinKey = MiogramKdf::derive(pin.toUtf8(), _salt);
	const auto pinHash = QCryptographicHash::hash(pinKey, QCryptographicHash::Sha256);
	if (MiogramKdf::timingSafeEqual(pinHash, _pinHash)) {
		_dbKeyPlain = XorWithKey(_wrappedDbKey, pinKey);
		_unlocked = true;
		_duress = false;
		return { true, false, QString() };
	}
	if (!_duressHash.isEmpty()) {
		const auto duressKey = MiogramKdf::derive(pin.toUtf8(), _duressSalt);
		const auto duressHash = QCryptographicHash::hash(duressKey, QCryptographicHash::Sha256);
		if (MiogramKdf::timingSafeEqual(duressHash, _duressHash)) {
			SecureZeroMemory(_dbKeyPlain);
			_unlocked = true;
			_duress = true;
			return { true, true, QString() };
		}
	}
	return { false, false, u"Wrong PIN"_q };
}

void ProfileVault::lock() {
	zeroizeNow();
	_unlocked = false;
	_duress = false;
}

void ProfileVault::zeroizeNow() {
	SecureZeroMemory(_dbKeyPlain);
}

QByteArray ProfileVault::databaseKey() const {
	if (!_unlocked || _duress) {
		return QByteArray();
	}
	return _dbKeyPlain;
}

QByteArray ProfileVault::metadataKey() const {
	return databaseKey();
}

bool ProfileVault::verifyPin(const QString &pin) const {
	if (!_hasVault) {
		return false;
	}
	const auto pinKey = MiogramKdf::derive(pin.toUtf8(), _salt);
	const auto pinHash = QCryptographicHash::hash(pinKey, QCryptographicHash::Sha256);
	return MiogramKdf::timingSafeEqual(pinHash, _pinHash);
}

bool ProfileVault::verifyDuressPin(const QString &pin) const {
	if (!_hasVault || _duressHash.isEmpty()) {
		return false;
	}
	const auto duressKey = MiogramKdf::derive(pin.toUtf8(), _duressSalt);
	const auto duressHash = QCryptographicHash::hash(duressKey, QCryptographicHash::Sha256);
	return MiogramKdf::timingSafeEqual(duressHash, _duressHash);
}

void ProfileVault::changePin(const QString &oldPin, const QString &newPin) {
	const auto res = unlock(oldPin);
	if (!res.success || res.duress) {
		return;
	}
	const auto dbKey = _dbKeyPlain;
	_salt = MiogramKdf::randomSalt(16);
	const auto pinKey = MiogramKdf::derive(newPin.toUtf8(), _salt);
	_wrappedDbKey = XorWithKey(dbKey, pinKey);
	_pinHash = QCryptographicHash::hash(pinKey, QCryptographicHash::Sha256);
	saveMeta();
}

void ProfileVault::loadMeta() {
	QFile f(VaultMetaPath());
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	_salt = QByteArray::fromBase64(obj.value(u"salt"_q).toString().toLatin1());
	_duressSalt = QByteArray::fromBase64(obj.value(u"duressSalt"_q).toString().toLatin1());
	_pinHash = QByteArray::fromBase64(obj.value(u"pinHash"_q).toString().toLatin1());
	_duressHash = QByteArray::fromBase64(obj.value(u"duressHash"_q).toString().toLatin1());
	_wrappedDbKey = QByteArray::fromBase64(obj.value(u"wrapped"_q).toString().toLatin1());
	_hasVault = !_salt.isEmpty() && !_pinHash.isEmpty() && !_wrappedDbKey.isEmpty();
}

void ProfileVault::saveMeta() {
	QJsonObject obj;
	obj.insert(u"salt"_q, QString::fromLatin1(_salt.toBase64()));
	obj.insert(u"duressSalt"_q, QString::fromLatin1(_duressSalt.toBase64()));
	obj.insert(u"pinHash"_q, QString::fromLatin1(_pinHash.toBase64()));
	obj.insert(u"duressHash"_q, QString::fromLatin1(_duressHash.toBase64()));
	obj.insert(u"wrapped"_q, QString::fromLatin1(_wrappedDbKey.toBase64()));
	QFile f(VaultMetaPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
}

MiogramGate &MiogramGate::instance() {
	static MiogramGate gate;
	return gate;
}

MiogramGate::MiogramGate() {
}

void MiogramGate::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	loadHidden();
	_locked = ProfileVault::instance().hasVault();
}

bool MiogramGate::isLocked() const {
	return _locked;
}

bool MiogramGate::isDecoyMode() const {
	return _decoy;
}

bool MiogramGate::unlock(const QString &pin, QString *error) {
	const auto res = ProfileVault::instance().unlock(pin);
	if (!res.success) {
		if (error) {
			*error = res.error;
		}
		return false;
	}
	_locked = false;
	_decoy = res.duress;
	return true;
}

void MiogramGate::lock() {
	ProfileVault::instance().lock();
	_locked = true;
	_decoy = false;
}

void MiogramGate::enterDecoy() {
	ProfileVault::instance().zeroizeNow();
	_locked = false;
	_decoy = true;
}

bool MiogramGate::isDialogAllowed(quint64 dialogId) const {
	if (!_decoy) {
		return true;
	}
	return !_hidden.contains(dialogId);
}

bool MiogramGate::isNotificationAllowed(quint64 dialogId) const {
	return isDialogAllowed(dialogId);
}

bool MiogramGate::isSearchAllowed(quint64 dialogId) const {
	return isDialogAllowed(dialogId);
}

void MiogramGate::setHiddenDialogs(const QVector<quint64> &ids) {
	_hidden = ids;
	auto &config = MiogramConfig::instance();
	QJsonArray arr;
	for (const auto id : _hidden) {
		arr.append(QString::number(id));
	}
	QJsonObject domain = config.getDomain(ConfigDomain::Privacy);
	domain.insert(u"hiddenDialogs"_q, arr);
	config.setDomain(ConfigDomain::Privacy, domain);
}

QVector<quint64> MiogramGate::hiddenDialogs() const {
	return _hidden;
}

void MiogramGate::loadHidden() {
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Privacy);
	const auto arr = domain.value(u"hiddenDialogs"_q).toArray();
	_hidden.clear();
	for (const auto &v : arr) {
		_hidden.append(v.toString().toULongLong());
	}
}

DoubleBottomManager &DoubleBottomManager::instance() {
	static DoubleBottomManager manager;
	return manager;
}

DoubleBottomManager::DoubleBottomManager() {
}

void DoubleBottomManager::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	ProfileVault::instance().initialize();
	MiogramGate::instance().initialize();
	_enabled = MiogramConfig::instance().boolValue(
		ConfigDomain::Privacy,
		u"doubleBottom"_q,
		false);
}

bool DoubleBottomManager::isDoubleBottomEnabled() const {
	return _enabled;
}

void DoubleBottomManager::setDoubleBottomEnabled(bool enabled) {
	_enabled = enabled;
	MiogramConfig::instance().setBoolValue(
		ConfigDomain::Privacy,
		u"doubleBottom"_q,
		enabled);
}

bool DoubleBottomManager::isDuressConfigured() const {
	return ProfileVault::instance().hasVault();
}

void DoubleBottomManager::configureDuress(const QString &pin, const QString &duressPin) {
	ProfileVault::instance().setup(pin, duressPin);
	setDoubleBottomEnabled(true);
}

bool DoubleBottomManager::validateAndUnlock(const QString &pin, bool *outDuress) {
	QString error;
	const auto ok = MiogramGate::instance().unlock(pin, &error);
	if (outDuress) {
		*outDuress = MiogramGate::instance().isDecoyMode();
	}
	return ok;
}

void DoubleBottomManager::onEnterBackground() {
	if (_enabled) {
		ProfileVault::instance().zeroizeNow();
	}
}

void DoubleBottomManager::onEnterForeground() {
}

} // namespace Miogram
