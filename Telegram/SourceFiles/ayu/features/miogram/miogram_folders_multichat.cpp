// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_folders_multichat.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUuid>
#include <algorithm>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

SubfolderEngine &SubfolderEngine::instance() {
	static SubfolderEngine engine;
	return engine;
}

SubfolderEngine::SubfolderEngine() {
}

void SubfolderEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

std::vector<Subfolder> SubfolderEngine::subfolders() const {
	return _subfolders;
}

Subfolder SubfolderEngine::subfolderById(const QString &id) const {
	for (const auto &folder : _subfolders) {
		if (folder.id == id) {
			return folder;
		}
	}
	return Subfolder{};
}

QString SubfolderEngine::createSubfolder(const QString &title, const QString &emoji) {
	Subfolder folder;
	folder.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	folder.title = title.trimmed().left(64);
	folder.emoji = emoji.left(8);
	_subfolders.push_back(folder);
	save();
	return folder.id;
}

void SubfolderEngine::renameSubfolder(const QString &id, const QString &title) {
	for (auto &folder : _subfolders) {
		if (folder.id == id) {
			folder.title = title.trimmed().left(64);
			break;
		}
	}
	save();
}

void SubfolderEngine::deleteSubfolder(const QString &id) {
	_subfolders.erase(
		std::remove_if(_subfolders.begin(), _subfolders.end(), [&](const Subfolder &f) {
			return f.id == id;
		}),
		_subfolders.end());
	save();
}

void SubfolderEngine::setCollapsed(const QString &id, bool collapsed) {
	for (auto &folder : _subfolders) {
		if (folder.id == id) {
			folder.collapsed = collapsed;
			break;
		}
	}
	save();
}

void SubfolderEngine::addDialog(const QString &subfolderId, quint64 dialogId) {
	for (auto &folder : _subfolders) {
		if (folder.id == subfolderId) {
			if (std::find(folder.dialogIds.begin(), folder.dialogIds.end(), dialogId) == folder.dialogIds.end()) {
				folder.dialogIds.push_back(dialogId);
			}
			break;
		}
	}
	save();
}

void SubfolderEngine::removeDialog(const QString &subfolderId, quint64 dialogId) {
	for (auto &folder : _subfolders) {
		if (folder.id == subfolderId) {
			folder.dialogIds.erase(
				std::remove(folder.dialogIds.begin(), folder.dialogIds.end(), dialogId),
				folder.dialogIds.end());
			break;
		}
	}
	save();
}

std::vector<QString> SubfolderEngine::subfoldersForDialog(quint64 dialogId) const {
	std::vector<QString> out;
	for (const auto &folder : _subfolders) {
		if (std::find(folder.dialogIds.begin(), folder.dialogIds.end(), dialogId) != folder.dialogIds.end()) {
			out.push_back(folder.id);
		}
	}
	return out;
}

void SubfolderEngine::load() {
	QFile f(cWorkingDir() + u"tdata/mio_subfolders.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isArray()) {
		return;
	}
	_subfolders.clear();
	for (const auto &v : doc.array()) {
		const auto o = v.toObject();
		Subfolder folder;
		folder.id = o.value(u"id"_q).toString();
		folder.title = o.value(u"title"_q).toString();
		folder.emoji = o.value(u"emoji"_q).toString();
		folder.collapsed = o.value(u"collapsed"_q).toBool(false);
		for (const auto &d : o.value(u"dialogs"_q).toArray()) {
			folder.dialogIds.push_back(d.toString().toULongLong());
		}
		if (!folder.id.isEmpty()) {
			_subfolders.push_back(folder);
		}
	}
}

void SubfolderEngine::save() {
	QJsonArray arr;
	for (const auto &folder : _subfolders) {
		QJsonObject o;
		o.insert(u"id"_q, folder.id);
		o.insert(u"title"_q, folder.title);
		o.insert(u"emoji"_q, folder.emoji);
		o.insert(u"collapsed"_q, folder.collapsed);
		QJsonArray dialogs;
		for (const auto id : folder.dialogIds) {
			dialogs.append(QString::number(id));
		}
		o.insert(u"dialogs"_q, dialogs);
		arr.append(o);
	}
	QFile f(cWorkingDir() + u"tdata/mio_subfolders.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}
}

FloatingChatState &FloatingChatState::instance() {
	static FloatingChatState state;
	return state;
}

FloatingChatState::FloatingChatState() {
}

void FloatingChatState::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Folders);
	_enabled = domain.value(u"floating"_q).toBool(false);
	for (const auto &v : domain.value(u"floatingPinned"_q).toArray()) {
		_pinned.push_back(v.toString().toULongLong());
	}
}

bool FloatingChatState::isEnabled() const {
	return _enabled;
}

void FloatingChatState::setEnabled(bool enabled) {
	_enabled = enabled;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Folders);
	domain.insert(u"floating"_q, enabled);
	MiogramConfig::instance().setDomain(ConfigDomain::Folders, domain);
}

std::vector<quint64> FloatingChatState::pinnedDialogs() const {
	return _pinned;
}

void FloatingChatState::pinDialog(quint64 dialogId) {
	if (std::find(_pinned.begin(), _pinned.end(), dialogId) == _pinned.end()) {
		_pinned.push_back(dialogId);
		auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Folders);
		QJsonArray arr;
		for (const auto id : _pinned) {
			arr.append(QString::number(id));
		}
		domain.insert(u"floatingPinned"_q, arr);
		MiogramConfig::instance().setDomain(ConfigDomain::Folders, domain);
	}
}

void FloatingChatState::unpinDialog(quint64 dialogId) {
	_pinned.erase(
		std::remove(_pinned.begin(), _pinned.end(), dialogId),
		_pinned.end());
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Folders);
	QJsonArray arr;
	for (const auto id : _pinned) {
		arr.append(QString::number(id));
	}
	domain.insert(u"floatingPinned"_q, arr);
	MiogramConfig::instance().setDomain(ConfigDomain::Folders, domain);
}

SplitChatState &SplitChatState::instance() {
	static SplitChatState state;
	return state;
}

SplitChatState::SplitChatState() {
}

void SplitChatState::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

bool SplitChatState::isSplitActive() const {
	return _active && _left != 0 && _right != 0;
}

void SplitChatState::setSplitActive(bool active) {
	_active = active;
}

quint64 SplitChatState::leftDialog() const {
	return _left;
}

quint64 SplitChatState::rightDialog() const {
	return _right;
}

void SplitChatState::setDialogs(quint64 left, quint64 right) {
	_left = left;
	_right = right;
	_active = (left != 0 && right != 0);
}

} // namespace Miogram
