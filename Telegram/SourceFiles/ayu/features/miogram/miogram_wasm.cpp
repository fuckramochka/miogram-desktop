// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_wasm.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDataStream>
#include <algorithm>

namespace Miogram {

WasmRuntime &WasmRuntime::instance() {
	static WasmRuntime runtime;
	return runtime;
}

WasmRuntime::WasmRuntime() {
}

void WasmRuntime::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_available = tryResolveWamr();
}

bool WasmRuntime::isAvailable() const {
	return _available;
}

QString WasmRuntime::version() const {
	return _available ? _version : u"wamr-stub (no native runtime linked)"_q;
}

QString WasmRuntime::backend() const {
	return _available ? u"wamr"_q : u"stub"_q;
}

bool WasmRuntime::tryResolveWamr() {
	const auto candidates = {
		u"iwasm"_q,
		u"libiwasm"_q,
		u"wamr"_q,
		u"libwamr"_q,
	};
	for (const auto &name : candidates) {
		QLibrary lib(name);
		if (lib.load()) {
			_version = u"wamr-system"_q;
			return true;
		}
	}
	return false;
}

bool WasmRuntime::loadModule(const QString &pluginId, const QString &wasmPath) {
	if (!QFileInfo::exists(wasmPath)) {
		_modules.push_back(WasmModuleInfo{
			.pluginId = pluginId,
			.path = wasmPath,
			.loaded = false,
			.error = u"File not found"_q,
		});
		return false;
	}
	const auto loaded = _available;
	_modules.push_back(WasmModuleInfo{
		.pluginId = pluginId,
		.path = wasmPath,
		.loaded = loaded,
		.error = loaded ? QString() : u"WAMR not linked"_q,
	});
	return loaded;
}

void WasmRuntime::unloadModule(const QString &pluginId) {
	_modules.erase(
		std::remove_if(_modules.begin(), _modules.end(), [&](const WasmModuleInfo &m) {
			return m.pluginId == pluginId;
		}),
		_modules.end());
}

void WasmRuntime::unloadAll() {
	_modules.clear();
}

QByteArray WasmRuntime::call(
		const QString &pluginId,
		const QString &op,
		const QByteArray &payload,
		bool *ok) {
	Q_UNUSED(op);
	Q_UNUSED(payload);
	for (const auto &m : _modules) {
		if (m.pluginId == pluginId && m.loaded && _available) {
			if (ok) {
				*ok = false;
			}
			return QByteArray();
		}
	}
	if (ok) {
		*ok = false;
	}
	return QByteArray();
}

std::vector<WasmModuleInfo> WasmRuntime::modules() const {
	return _modules;
}

QByteArray EnvelopeCodec::encode(const QString &op, const QByteArray &payload) {
// Envelope layout mirrors sdk/rust/miogram-plugin-sdk/src/envelope.rs:
// u32 ABI version, u32 op length, op bytes, u32 payload length, payload.
// The fixed header lets both the Qt host and the Rust/WASM guest reject
// mismatched frames before touching plugin memory.
	QByteArray frame;
	{
		QDataStream header(&frame, QIODevice::WriteOnly);
		header.setByteOrder(QDataStream::LittleEndian);
		header << qint32(kMiogramAbiVersion);
		header << qint32(op.toUtf8().size());
	}
	frame.append(op.toUtf8());
	{
		QByteArray tail;
		QDataStream stream(&tail, QIODevice::WriteOnly);
		stream.setByteOrder(QDataStream::LittleEndian);
		stream << qint32(payload.size());
		frame.append(tail);
	}
	frame.append(payload);
	return frame;
}

bool EnvelopeCodec::decode(
		const QByteArray &frame,
		QString *op,
		QByteArray *payload) {
	if (frame.size() < 12) {
		return false;
	}
	QDataStream stream(frame);
	stream.setByteOrder(QDataStream::LittleEndian);
	qint32 abi = 0;
	qint32 opLen = 0;
	stream >> abi >> opLen;
	if (abi != kMiogramAbiVersion || opLen < 0 || frame.size() < 8 + opLen + 4) {
		return false;
	}
	const auto opBytes = frame.mid(8, opLen);
	qint32 payloadLen = 0;
	QDataStream tail(frame.mid(8 + opLen));
	tail.setByteOrder(QDataStream::LittleEndian);
	tail >> payloadLen;
	if (payloadLen < 0 || frame.size() < 8 + opLen + 4 + payloadLen) {
		return false;
	}
	if (op) {
		*op = QString::fromUtf8(opBytes);
	}
	if (payload) {
		*payload = frame.mid(8 + opLen + 4, payloadLen);
	}
	return true;
}

QByteArray EnvelopeCodec::encodeResponse(const QByteArray &payload) {
	return encode(u"ok"_q, payload);
}

QByteArray EnvelopeCodec::encodeError(int code, const QString &message) {
	QJsonObject obj;
	obj.insert(u"code"_q, code);
	obj.insert(u"message"_q, message);
	return encode(
		u"error"_q,
		QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

} // namespace Miogram
