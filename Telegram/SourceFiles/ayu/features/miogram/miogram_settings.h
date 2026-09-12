// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct SettingsSection {
	QString id;
	QString titleUk;
	QString titleRu;
	QString titleEn;
	std::vector<QString> controlIds;
};

class MiogramSettingsModel {
public:
	static MiogramSettingsModel &instance();

	void initialize();

	[[nodiscard]] std::vector<SettingsSection> sections() const;
	[[nodiscard]] SettingsSection sectionById(const QString &id) const;

	[[nodiscard]] QString localizedTitle(const SettingsSection &section) const;

private:
	MiogramSettingsModel();

	std::vector<SettingsSection> _sections;
	bool _initialized = false;
};

void InitAllMiogramModules();

} // namespace Miogram
