// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_userbot_performance.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "ayu/features/miogram/miogram_config.h"
#include "ayu/features/miogram/miogram_vault.h"
#include "core/application.h"

namespace Miogram {

HerokuManager &HerokuManager::instance() {
	static HerokuManager manager;
	return manager;
}

HerokuManager::HerokuManager() {
}

void HerokuManager::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

bool HerokuManager::isConfigured() const {
	return !_config.appName.trimmed().isEmpty() && !_config.apiKey.isEmpty();
}

HerokuConfig HerokuManager::config() const {
	return _config;
}

void HerokuManager::setConfig(const HerokuConfig &config) {
	_config = config;
	save();
}

void HerokuManager::clear() {
	_config = HerokuConfig{};
	save();
}

void HerokuManager::deployStatus(Fn<void(QString status, QString error)> callback) {
	if (!isConfigured()) {
		if (callback) {
			callback(QString(), u"Heroku not configured"_q);
		}
		return;
	}
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://api.heroku.com/apps/"_q + _config.appName);
	QNetworkRequest req(url);
	req.setRawHeader("Accept", "application/vnd.heroku+json; version=3");
	req.setRawHeader("Authorization", ("Bearer " + _config.apiKey).toUtf8());
	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) {
				callback(QString(), reply->errorString());
			}
			return;
		}
		const auto doc = QJsonDocument::fromJson(reply->readAll());
		const auto status = doc.object().value(u"name"_q).toString();
		if (callback) {
			callback(status.isEmpty() ? u"ok"_q : status, QString());
		}
	});
}

void HerokuManager::load() {
	QFile f(cWorkingDir() + u"tdata/mio_heroku.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	_config.appName = obj.value(u"app"_q).toString();
	_config.autoDeploy = obj.value(u"auto"_q).toBool(false);
	const auto encKey = QByteArray::fromBase64(obj.value(u"key"_q).toString().toLatin1());
	const auto encToken = QByteArray::fromBase64(obj.value(u"token"_q).toString().toLatin1());
	const auto dbKey = ProfileVault::instance().databaseKey();
	if (!encKey.isEmpty()) {
		_config.apiKey = QString::fromUtf8(
			dbKey.isEmpty() ? encKey : AesGcm::decrypt(encKey, dbKey, QByteArray()));
	}
	if (!encToken.isEmpty()) {
		_config.botToken = QString::fromUtf8(
			dbKey.isEmpty() ? encToken : AesGcm::decrypt(encToken, dbKey, QByteArray()));
	}
}

void HerokuManager::save() {
	const auto dbKey = ProfileVault::instance().databaseKey();
	QJsonObject obj;
	obj.insert(u"app"_q, _config.appName);
	obj.insert(u"auto"_q, _config.autoDeploy);
	const auto encKey = dbKey.isEmpty()
		? _config.apiKey.toUtf8()
		: AesGcm::encrypt(_config.apiKey.toUtf8(), dbKey, AesGcm::randomNonce(12));
	const auto encToken = dbKey.isEmpty()
		? _config.botToken.toUtf8()
		: AesGcm::encrypt(_config.botToken.toUtf8(), dbKey, AesGcm::randomNonce(12));
	obj.insert(u"key"_q, QString::fromLatin1(encKey.toBase64()));
	obj.insert(u"token"_q, QString::fromLatin1(encToken.toBase64()));
	QFile f(cWorkingDir() + u"tdata/mio_heroku.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
}

PerformanceOptimizer &PerformanceOptimizer::instance() {
	static PerformanceOptimizer optimizer;
	return optimizer;
}

PerformanceOptimizer::PerformanceOptimizer() {
}

void PerformanceOptimizer::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Performance);
	_batterySaver = domain.value(u"batterySaver"_q).toBool(true);
	_animations = domain.value(u"animations"_q).toBool(true);
	_imageCache = domain.value(u"imageCache"_q).toInt(256);
}

bool PerformanceOptimizer::batterySaverRespected() const {
	return _batterySaver;
}

void PerformanceOptimizer::setBatterySaverRespected(bool respected) {
	_batterySaver = respected;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Performance);
	domain.insert(u"batterySaver"_q, respected);
	MiogramConfig::instance().setDomain(ConfigDomain::Performance, domain);
}

bool PerformanceOptimizer::animationsEnabled() const {
	return _animations;
}

void PerformanceOptimizer::setAnimationsEnabled(bool enabled) {
	_animations = enabled;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Performance);
	domain.insert(u"animations"_q, enabled);
	MiogramConfig::instance().setDomain(ConfigDomain::Performance, domain);
}

int PerformanceOptimizer::maxCachedImages() const {
	return _imageCache;
}

void PerformanceOptimizer::setMaxCachedImages(int count) {
	_imageCache = std::clamp(count, 32, 2048);
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Performance);
	domain.insert(u"imageCache"_q, _imageCache);
	MiogramConfig::instance().setDomain(ConfigDomain::Performance, domain);
}

FpsController &FpsController::instance() {
	static FpsController controller;
	return controller;
}

FpsController::FpsController() {
}

void FpsController::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_preferred = MiogramConfig::instance().intValue(
		ConfigDomain::Performance,
		u"fps"_q,
		60);
}

int FpsController::preferredFps() const {
	return _preferred;
}

void FpsController::setPreferredFps(int fps) {
	_preferred = std::clamp(fps, 30, 144);
	MiogramConfig::instance().setIntValue(
		ConfigDomain::Performance,
		u"fps"_q,
		_preferred);
}

bool FpsController::isBatterySaverActive() const {
	return false;
}

int FpsController::effectiveFps() const {
	if (PerformanceOptimizer::instance().batterySaverRespected() && isBatterySaverActive()) {
		return std::min(_preferred, 60);
	}
	return _preferred;
}

} // namespace Miogram
