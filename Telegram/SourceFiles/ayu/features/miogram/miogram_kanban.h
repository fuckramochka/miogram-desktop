// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct KanbanItem {
	QString id;
	QString title;
	QString description;
	int column = 0;
	quint64 dialogId = 0;
	int messageId = 0;
	qint64 createdAt = 0;
};

inline constexpr int kKanbanColumns = 4;

class KanbanStorage {
public:
	static KanbanStorage &instance();

	void initialize();

	[[nodiscard]] std::vector<KanbanItem> items() const;
	[[nodiscard]] std::vector<KanbanItem> itemsInColumn(int column) const;

	QString addItem(
		const QString &title,
		const QString &description,
		int column,
		quint64 dialogId = 0,
		int messageId = 0);
	void moveItem(const QString &id, int targetColumn);
	void updateItem(const KanbanItem &item);
	void deleteItem(const QString &id);
	void clearColumn(int column);

	[[nodiscard]] QString columnTitle(int column) const;

private:
	KanbanStorage();
	void load();
	void save();

	std::vector<KanbanItem> _items;
	bool _initialized = false;
};

} // namespace Miogram
