// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct CloudVaultFile {
	QString id;
	QString name;
	QString mimeType;
	qint64 size = 0;
	QString remotePath;
	QString localPath;
	qint64 updatedAt = 0;
	bool encrypted = true;
};

class CloudVaultEngine {
public:
	static CloudVaultEngine &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] std::vector<CloudVaultFile> files() const;
	void refresh(Fn<void(std::vector<CloudVaultFile>)> callback);

	void upload(
		const QString &localPath,
		Fn<void(CloudVaultFile)> callback);
	void download(
		const QString &fileId,
		Fn<void(QString localPath)> callback);
	void remove(const QString &fileId);

	[[nodiscard]] QString storageRoot() const;

private:
	CloudVaultEngine();
	void loadCache();
	void saveCache();

	std::vector<CloudVaultFile> _cache;
	bool _enabled = false;
	bool _initialized = false;
};

} // namespace Miogram
