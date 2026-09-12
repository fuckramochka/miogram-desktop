// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

inline constexpr int kMiogramAbiVersion = 1;
inline constexpr int kErrBadFrame = 1;
inline constexpr int kErrHandlerPanicked = 2;
inline constexpr int kErrNoPlugin = 3;

struct WasmModuleInfo {
	QString pluginId;
	QString path;
	bool loaded = false;
	QString error;
};

class WasmRuntime {
public:
	static WasmRuntime &instance();

	void initialize();

	[[nodiscard]] bool isAvailable() const;
	[[nodiscard]] QString version() const;
	[[nodiscard]] QString backend() const;

	bool loadModule(const QString &pluginId, const QString &wasmPath);
	void unloadModule(const QString &pluginId);
	void unloadAll();

	QByteArray call(
		const QString &pluginId,
		const QString &op,
		const QByteArray &payload,
		bool *ok = nullptr);

	[[nodiscard]] std::vector<WasmModuleInfo> modules() const;

private:
	WasmRuntime();
	bool tryResolveWamr();

	bool _available = false;
	QString _version;
	std::vector<WasmModuleInfo> _modules;
	bool _initialized = false;
};

class EnvelopeCodec {
public:
	[[nodiscard]] static QByteArray encode(
		const QString &op,
		const QByteArray &payload);
	[[nodiscard]] static bool decode(
		const QByteArray &frame,
		QString *op,
		QByteArray *payload);
	[[nodiscard]] static QByteArray encodeResponse(const QByteArray &payload);
	[[nodiscard]] static QByteArray encodeError(int code, const QString &message);
};

} // namespace Miogram
