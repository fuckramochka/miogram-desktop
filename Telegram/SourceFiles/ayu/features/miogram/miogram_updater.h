// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include "base/basic_types.h"

namespace Miogram {

struct UpdateInfo {
	bool hasUpdate = false;
	QString version;
	QString changelog;
	QString apkUrl;
	QString currentVersion;
};

class MiogramUpdater {
public:
	static MiogramUpdater &instance();

	void initialize();

	void initAutoUpdate();
	void checkOnEntry();
	void checkAndShow(
		Fn<void(UpdateInfo)> callback,
		bool manualCheck = false);

	[[nodiscard]] static QString currentAppVersion();
	[[nodiscard]] static bool isNewerVersion(
		const QString &currentVersion,
		const QString &remoteVersion,
		const QString &remoteTag,
		const QString &changelog);

	[[nodiscard]] static QString latestReleaseApi();
	[[nodiscard]] static qint64 checkIntervalMs();

private:
	MiogramUpdater();
	void fetchLatestRelease(Fn<void(UpdateInfo)> callback);

	bool _autoStarted = false;
	bool _initialized = false;
};

class DownloadManager {
public:
	static DownloadManager &instance();

	void initialize();
	void download(
		const QString &url,
		Fn<void(QString localPath, QString error)> callback);
	void cancel();

private:
	DownloadManager();

	bool _busy = false;
	bool _initialized = false;
};

} // namespace Miogram
