// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include "base/basic_types.h"

namespace Miogram {

struct HerokuConfig {
	QString appName;
	QString apiKey;
	QString botToken;
	bool autoDeploy = false;
};

class HerokuManager {
public:
	static HerokuManager &instance();

	void initialize();

	[[nodiscard]] bool isConfigured() const;
	[[nodiscard]] HerokuConfig config() const;
	void setConfig(const HerokuConfig &config);
	void clear();

	void deployStatus(Fn<void(QString status, QString error)> callback);

private:
	HerokuManager();
	void load();
	void save();

	HerokuConfig _config;
	bool _initialized = false;
};

class PerformanceOptimizer {
public:
	static PerformanceOptimizer &instance();

	void initialize();

	[[nodiscard]] bool batterySaverRespected() const;
	void setBatterySaverRespected(bool respected);

	[[nodiscard]] bool animationsEnabled() const;
	void setAnimationsEnabled(bool enabled);

	[[nodiscard]] int maxCachedImages() const;
	void setMaxCachedImages(int count);

private:
	PerformanceOptimizer();

	bool _batterySaver = true;
	bool _animations = true;
	int _imageCache = 256;
	bool _initialized = false;
};

class FpsController {
public:
	static FpsController &instance();

	void initialize();

	[[nodiscard]] int preferredFps() const;
	void setPreferredFps(int fps);

	[[nodiscard]] bool isBatterySaverActive() const;
	[[nodiscard]] int effectiveFps() const;

private:
	FpsController();

	int _preferred = 60;
	bool _initialized = false;
};

} // namespace Miogram
