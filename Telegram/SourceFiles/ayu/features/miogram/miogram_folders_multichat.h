// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct Subfolder {
	QString id;
	QString title;
	QString emoji;
	std::vector<quint64> dialogIds;
	bool collapsed = false;
};

class SubfolderEngine {
public:
	static SubfolderEngine &instance();

	void initialize();

	[[nodiscard]] std::vector<Subfolder> subfolders() const;
	[[nodiscard]] Subfolder subfolderById(const QString &id) const;

	QString createSubfolder(const QString &title, const QString &emoji = QString());
	void renameSubfolder(const QString &id, const QString &title);
	void deleteSubfolder(const QString &id);
	void setCollapsed(const QString &id, bool collapsed);

	void addDialog(const QString &subfolderId, quint64 dialogId);
	void removeDialog(const QString &subfolderId, quint64 dialogId);
	[[nodiscard]] std::vector<QString> subfoldersForDialog(quint64 dialogId) const;

private:
	SubfolderEngine();
	void load();
	void save();

	std::vector<Subfolder> _subfolders;
	bool _initialized = false;
};

class FloatingChatState {
public:
	static FloatingChatState &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] std::vector<quint64> pinnedDialogs() const;
	void pinDialog(quint64 dialogId);
	void unpinDialog(quint64 dialogId);

private:
	FloatingChatState();

	std::vector<quint64> _pinned;
	bool _enabled = false;
	bool _initialized = false;
};

class SplitChatState {
public:
	static SplitChatState &instance();

	void initialize();

	[[nodiscard]] bool isSplitActive() const;
	void setSplitActive(bool active);

	[[nodiscard]] quint64 leftDialog() const;
	[[nodiscard]] quint64 rightDialog() const;
	void setDialogs(quint64 left, quint64 right);

private:
	SplitChatState();

	quint64 _left = 0;
	quint64 _right = 0;
	bool _active = false;
	bool _initialized = false;
};

} // namespace Miogram
