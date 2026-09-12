// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct MelConfig {
	int sampleRate = 16000;
	int frameLengthMs = 25;
	int frameShiftMs = 10;
	int melBands = 80;
	int mfccCount = 13;
	double preEmphasis = 0.97;
};

class AudioFrontend {
public:
	static AudioFrontend &instance();

	void initialize();

	[[nodiscard]] std::vector<std::vector<float>> logMel(
		const std::vector<float> &pcmMono,
		int sampleRate,
		const MelConfig &config = MelConfig{});
	[[nodiscard]] std::vector<std::vector<float>> mfcc(
		const std::vector<float> &pcmMono,
		int sampleRate,
		const MelConfig &config = MelConfig{});

	[[nodiscard]] std::vector<float> resampleTo16k(
		const std::vector<float> &pcmMono,
		int sampleRate) const;
	[[nodiscard]] std::vector<float> normalize(
		const std::vector<float> &pcm) const;

private:
	AudioFrontend();

	bool _initialized = false;
};

class OnnxWhisperBridge {
public:
	static OnnxWhisperBridge &instance();

	void initialize();

	[[nodiscard]] bool isModelLoaded() const;
	[[nodiscard]] QString modelPath() const;
	void setModelPath(const QString &path);

	void transcribeMel(
		const std::vector<std::vector<float>> &mel,
		Fn<void(QString text, QString error)> callback);

private:
	OnnxWhisperBridge();

	QString _modelPath;
	bool _initialized = false;
};

} // namespace Miogram
