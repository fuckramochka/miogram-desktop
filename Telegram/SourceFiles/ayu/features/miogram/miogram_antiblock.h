// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct BypassServer {
	QString id;
	QString name;
	QString host;
	uint32 port = 443;
	QString secret;
	QString sniDomain;
	int ping = 0;
	bool available = false;
	bool isCustom = false;

	[[nodiscard]] bool isYandexSni() const {
		const auto lower = sniDomain.toLower();
		return lower.contains(u"yandex"_q) || lower.contains(u"ya.ru"_q);
	}
};

class AntiBlockEngine {
public:
	static AntiBlockEngine &instance();

	void initialize();

	[[nodiscard]] bool isAutoBypassEnabled() const;
	void setAutoBypassEnabled(bool enabled);

	[[nodiscard]] bool isPrioritizeYandexEnabled() const;
	void setPrioritizeYandexEnabled(bool enabled);

	[[nodiscard]] bool isAutoRotateEnabled() const;
	void setAutoRotateEnabled(bool enabled);

	void engageFastestBypassServer();
	void rotateToNextServer();
	void fetchRemotePoolAsync();

	[[nodiscard]] std::vector<BypassServer> servers() const;
	void addCustomServer(const BypassServer &server);
	void removeServer(const QString &id);

private:
	AntiBlockEngine();
	void initBuiltinServers();
	void loadSettings();
	void saveSettings();
	void checkThrottling();

	std::vector<BypassServer> _servers;
	bool _autoBypass = true;
	bool _prioritizeYandex = true;
	bool _autoRotate = true;
	int _currentIndex = 0;
	bool _initialized = false;
};

} // namespace Miogram
