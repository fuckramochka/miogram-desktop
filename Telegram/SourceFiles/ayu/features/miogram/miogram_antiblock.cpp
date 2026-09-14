// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_antiblock.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "core/application.h"
#include "core/core_settings.h"
#include "core/core_settings_proxy.h"
#include "mtproto/mtproto_proxy_data.h"

namespace Miogram {

namespace {

QString BypassSettingsPath() {
	return cWorkingDir() + u"tdata/mio_bypass.json"_q;
}

} // namespace

AntiBlockEngine &AntiBlockEngine::instance() {
	static AntiBlockEngine engine;
	return engine;
}

AntiBlockEngine::AntiBlockEngine() {
}

void AntiBlockEngine::initialize() {
	if (_initialized) return;
	_initialized = true;

	initBuiltinServers();
	loadSettings();

	// Throttle check timer
	auto *timer = new QTimer(&Core::App());
	QObject::connect(timer, &QTimer::timeout, [this] {
		checkThrottling();
	});
	timer->start(5000); // 5 sec interval

	fetchRemotePoolAsync();
}

void AntiBlockEngine::initBuiltinServers() {
	_servers.clear();

	BypassServer s1;
	s1.id = u"builtin_ya_1"_q;
	s1.name = QStringLiteral("Яндекс РФ #1 (ya.ru Fake-TLS)");
	s1.host = u"bonus.growthtrade.eu"_q;
	s1.port = 443;
	s1.secret = u"ee0fff11aae73289dec968ad854b3ffe0279612e7275"_q;
	s1.sniDomain = u"ya.ru"_q;
	s1.isCustom = false;
	_servers.push_back(s1);

	BypassServer s2;
	s2.id = u"builtin_ya_2"_q;
	s2.name = QStringLiteral("Яндекс РФ #2 (ya.ru Fake-TLS)");
	s2.host = u"proxy.growthtrade.eu"_q;
	s2.port = 443;
	s2.secret = u"ee54e6e41e680ad7765a0679a5a24eb49b79612e7275"_q;
	s2.sniDomain = u"ya.ru"_q;
	s2.isCustom = false;
	_servers.push_back(s2);
}

bool AntiBlockEngine::isAutoBypassEnabled() const {
	return _autoBypass;
}

void AntiBlockEngine::setAutoBypassEnabled(bool enabled) {
	_autoBypass = enabled;
	saveSettings();
}

bool AntiBlockEngine::isPrioritizeYandexEnabled() const {
	return _prioritizeYandex;
}

void AntiBlockEngine::setPrioritizeYandexEnabled(bool enabled) {
	_prioritizeYandex = enabled;
	saveSettings();
}

bool AntiBlockEngine::isAutoRotateEnabled() const {
	return _autoRotate;
}

void AntiBlockEngine::setAutoRotateEnabled(bool enabled) {
	_autoRotate = enabled;
	saveSettings();
}

std::vector<BypassServer> AntiBlockEngine::servers() const {
	return _servers;
}

void AntiBlockEngine::addCustomServer(const BypassServer &server) {
	_servers.push_back(server);
	saveSettings();
}

void AntiBlockEngine::removeServer(const QString &id) {
	_servers.erase(std::remove_if(_servers.begin(), _servers.end(), [&](const BypassServer &s) {
		return s.id == id && s.isCustom;
	}), _servers.end());
	saveSettings();
}

void AntiBlockEngine::engageFastestBypassServer() {
	if (_servers.empty()) return;

	// Pick Yandex if prioritized
	size_t targetIdx = 0;
	if (_prioritizeYandex) {
		for (size_t i = 0; i < _servers.size(); ++i) {
			if (_servers[i].isYandexSni()) {
				targetIdx = i;
				break;
			}
		}
	}

	const auto &server = _servers[targetIdx];
	_currentIndex = static_cast<int>(targetIdx);

	MTP::ProxyData proxy;
	proxy.type = MTP::ProxyData::Type::Mtproto;
	proxy.host = server.host;
	proxy.port = server.port;
	proxy.password = server.secret;

	Core::App().settings().proxy().setSelected(proxy);
	Core::App().settings().proxy().setSettings(MTP::ProxyData::Settings::Enabled);
}

void AntiBlockEngine::rotateToNextServer() {
	if (_servers.empty()) return;

	_currentIndex = (_currentIndex + 1) % static_cast<int>(_servers.size());
	const auto &server = _servers[_currentIndex];

	MTP::ProxyData proxy;
	proxy.type = MTP::ProxyData::Type::Mtproto;
	proxy.host = server.host;
	proxy.port = server.port;
	proxy.password = server.secret;

	Core::App().settings().proxy().setSelected(proxy);
	Core::App().settings().proxy().setSettings(MTP::ProxyData::Settings::Enabled);
}

void AntiBlockEngine::checkThrottling() {
	if (!_autoBypass) return;

	// If proxy is already enabled and working, nothing to do
	if (Core::App().settings().proxy().isEnabled()) {
		return;
	}

	// If user is direct connecting and throttled, engage bypass
	// (Telegram core will handle failover, but this pro-actively kicks in)
}

void AntiBlockEngine::fetchRemotePoolAsync() {
	auto *nam = new QNetworkAccessManager();
	QUrl url(u"https://raw.githubusercontent.com/fuckramochka/miogram/main/proxies.json"_q);
	QNetworkRequest req(url);

	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [this, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			return;
		}

		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isArray()) return;

		const auto arr = doc.array();
		for (const auto &v : arr) {
			if (!v.isObject()) continue;
			const auto o = v.toObject();

			BypassServer s;
			s.id = o.value(u"id"_q).toString(u"remote_"_q + QString::number(rand()));
			s.name = o.value(u"name"_q).toString(QStringLiteral("Fake-TLS Node"));
			s.host = o.value(u"host"_q).toString();
			s.port = static_cast<uint32>(o.value(u"port"_q).toInt(443));
			s.secret = o.value(u"secret"_q).toString();
			s.sniDomain = o.value(u"sni"_q).toString(u"ya.ru"_q);
			s.isCustom = false;

			if (!s.host.isEmpty() && !s.secret.isEmpty()) {
				// Avoid duplicates
				bool exists = false;
				for (const auto &cur : _servers) {
					if (cur.host == s.host && cur.port == s.port) {
						exists = true;
						break;
					}
				}
				if (!exists) {
					_servers.push_back(s);
				}
			}
		}
	});
}

void AntiBlockEngine::loadSettings() {
	QFile f(BypassSettingsPath());
	if (!f.open(QIODevice::ReadOnly)) return;

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return;

	const auto obj = doc.object();
	_autoBypass = obj.value(u"auto_bypass"_q).toBool(true);
	_prioritizeYandex = obj.value(u"prioritize_yandex"_q).toBool(true);
	_autoRotate = obj.value(u"auto_rotate"_q).toBool(true);
}

void AntiBlockEngine::saveSettings() {
	QJsonObject root;
	root[u"auto_bypass"_q] = _autoBypass;
	root[u"prioritize_yandex"_q] = _prioritizeYandex;
	root[u"auto_rotate"_q] = _autoRotate;

	QFile f(BypassSettingsPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
