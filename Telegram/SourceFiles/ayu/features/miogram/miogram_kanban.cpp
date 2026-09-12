// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_kanban.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUuid>
#include <QtCore/QDateTime>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

KanbanStorage &KanbanStorage::instance() {
	static KanbanStorage storage;
	return storage;
}

KanbanStorage::KanbanStorage() {
}

void KanbanStorage::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

std::vector<KanbanItem> KanbanStorage::items() const {
	return _items;
}

std::vector<KanbanItem> KanbanStorage::itemsInColumn(int column) const {
	std::vector<KanbanItem> out;
	for (const auto &item : _items) {
		if (item.column == column) {
			out.push_back(item);
		}
	}
	return out;
}

QString KanbanStorage::addItem(
		const QString &title,
		const QString &description,
		int column,
		quint64 dialogId,
		int messageId) {
	KanbanItem item;
	item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	item.title = title.trimmed().left(200);
	item.description = description.trimmed().left(2000);
	item.column = std::clamp(column, 0, kKanbanColumns - 1);
	item.dialogId = dialogId;
	item.messageId = messageId;
	item.createdAt = QDateTime::currentMSecsSinceEpoch();
	_items.insert(_items.begin(), item);
	save();
	return item.id;
}

void KanbanStorage::moveItem(const QString &id, int targetColumn) {
	for (auto &item : _items) {
		if (item.id == id) {
			item.column = std::clamp(targetColumn, 0, kKanbanColumns - 1);
			break;
		}
	}
	save();
}

void KanbanStorage::updateItem(const KanbanItem &item) {
	for (auto &existing : _items) {
		if (existing.id == item.id) {
			existing = item;
			break;
		}
	}
	save();
}

void KanbanStorage::deleteItem(const QString &id) {
	_items.erase(
		std::remove_if(_items.begin(), _items.end(), [&](const KanbanItem &item) {
			return item.id == id;
		}),
		_items.end());
	save();
}

void KanbanStorage::clearColumn(int column) {
	_items.erase(
		std::remove_if(_items.begin(), _items.end(), [&](const KanbanItem &item) {
			return item.column == column;
		}),
		_items.end());
	save();
}

QString KanbanStorage::columnTitle(int column) const {
	switch (column) {
	case 0: return u"Inbox"_q;
	case 1: return u"In Progress"_q;
	case 2: return u"Important"_q;
	case 3: return u"Done"_q;
	}
	return u"Inbox"_q;
}

void KanbanStorage::load() {
	QFile f(cWorkingDir() + u"tdata/mio_kanban.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isArray()) {
		return;
	}
	_items.clear();
	for (const auto &v : doc.array()) {
		const auto o = v.toObject();
		KanbanItem item;
		item.id = o.value(u"id"_q).toString();
		item.title = o.value(u"title"_q).toString();
		item.description = o.value(u"desc"_q).toString();
		item.column = std::clamp(o.value(u"col"_q).toInt(0), 0, kKanbanColumns - 1);
		item.dialogId = o.value(u"dialog"_q).toString().toULongLong();
		item.messageId = o.value(u"msg"_q).toInt(0);
		item.createdAt = o.value(u"at"_q).toVariant().toLongLong();
		if (!item.id.isEmpty()) {
			_items.push_back(item);
		}
	}
}

void KanbanStorage::save() {
	QJsonArray arr;
	for (const auto &item : _items) {
		QJsonObject o;
		o.insert(u"id"_q, item.id);
		o.insert(u"title"_q, item.title);
		o.insert(u"desc"_q, item.description);
		o.insert(u"col"_q, item.column);
		o.insert(u"dialog"_q, QString::number(item.dialogId));
		o.insert(u"msg"_q, item.messageId);
		o.insert(u"at"_q, item.createdAt);
		arr.append(o);
	}
	QFile f(cWorkingDir() + u"tdata/mio_kanban.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Kanban);
	domain.insert(u"count"_q, int(_items.size()));
	MiogramConfig::instance().setDomain(ConfigDomain::Kanban, domain);
}

} // namespace Miogram
