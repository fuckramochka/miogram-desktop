// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtWidgets/QWidget>
#include "base/basic_types.h"

namespace Miogram {

class MusorDropOverlay {
public:
	static void show(
		const QString &mediaPath,
		const QString &hintText);
	[[nodiscard]] static bool isShowing();
	static void close();
};

} // namespace Miogram
