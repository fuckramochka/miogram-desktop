// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.
//
// C++ counterpart of sdk/rust/miogram-plugin-sdk.
// Guest ABI: miogram_abi_version() -> 1, miogram_alloc,
// miogram_guest_free, miogram_call. See miogram_wasm.h EnvelopeCodec.

#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <functional>
#include <memory>

namespace MiogramPluginSdk {

inline constexpr int kAbiVersion = 1;
inline constexpr int kErrBadFrame = 1;
inline constexpr int kErrHandlerPanicked = 2;
inline constexpr int kErrNoPlugin = 3;

class Plugin {
public:
	virtual ~Plugin() = default;
	virtual QByteArray handle(const QString &op, const QByteArray &payload) = 0;
};

using PluginFactory = std::function<std::unique_ptr<Plugin>()>;

class Registry {
public:
	static Registry &instance();

	void setFactory(PluginFactory factory);
	std::unique_ptr<Plugin> create() const;
	[[nodiscard]] bool hasPlugin() const;

private:
	Registry() = default;

	PluginFactory _factory;
};

} // namespace MiogramPluginSdk

#define MIOGRAM_REGISTER_PLUGIN(PluginType) \
	extern "C" int miogram_abi_version() { \
		return ::MiogramPluginSdk::kAbiVersion; \
	} \
	namespace { \
	struct PluginType##Registrar { \
		PluginType##Registrar() { \
			::MiogramPluginSdk::Registry::instance().setFactory([] { \
				return std::make_unique<PluginType>(); \
			}); \
		} \
	}; \
	static PluginType##Registrar g_miogramRegistrar; \
	}
