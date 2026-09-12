// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_fun_box.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>

namespace Miogram {

namespace {

class BlackoutWidget final : public QWidget {
public:
	BlackoutWidget(const QString &text, QWidget *parent = nullptr)
	: QWidget(parent) {
		setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
		setAttribute(Qt::WA_DeleteOnClose);
		setStyleSheet(u"background: black;"_q);
		auto *layout = new QVBoxLayout(this);
		layout->setAlignment(Qt::AlignCenter);
		auto *label = new QLabel(text, this);
		label->setStyleSheet(u"color: white; font-size: 18px;"_q);
		label->setAlignment(Qt::AlignCenter);
		label->setWordWrap(true);
		layout->addWidget(label);
		QTimer::singleShot(3500, this, &QWidget::close);
	}

protected:
	void mousePressEvent(QMouseEvent *) override {
		close();
	}
	void keyPressEvent(QKeyEvent *e) override {
		if (e->key() == Qt::Key_Escape) {
			close();
		}
	}
};

BlackoutWidget *_activeOverlay = nullptr;

} // namespace

void MusorDropOverlay::show(const QString &mediaPath, const QString &hintText) {
	close();
	const auto text = mediaPath.isEmpty()
		? hintText
		: (u"🗑️ MUSORDROP 🗑️\n"_q + mediaPath);
	_activeOverlay = new BlackoutWidget(text);
	_activeOverlay->showFullScreen();
	QObject::connect(
		_activeOverlay,
		&QObject::destroyed,
		[] { _activeOverlay = nullptr; });
}

bool MusorDropOverlay::isShowing() {
	return _activeOverlay != nullptr;
}

void MusorDropOverlay::close() {
	if (_activeOverlay) {
		_activeOverlay->close();
		_activeOverlay = nullptr;
	}
}

} // namespace Miogram
