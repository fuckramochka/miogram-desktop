// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_stt_audio.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <cmath>
#include <algorithm>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

AudioFrontend &AudioFrontend::instance() {
	static AudioFrontend frontend;
	return frontend;
}

AudioFrontend::AudioFrontend() {
}

void AudioFrontend::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

std::vector<float> AudioFrontend::resampleTo16k(
		const std::vector<float> &pcmMono,
		int sampleRate) const {
	if (sampleRate == 16000 || pcmMono.empty()) {
		return pcmMono;
	}
	const auto ratio = double(16000) / double(sampleRate);
	const auto outSize = size_t(pcmMono.size() * ratio);
	std::vector<float> out;
	out.reserve(outSize);
	for (size_t i = 0; i < outSize; ++i) {
		const auto src = double(i) / ratio;
		const auto i0 = size_t(src);
		const auto frac = float(src - i0);
		const auto s0 = pcmMono[std::min(i0, pcmMono.size() - 1)];
		const auto s1 = pcmMono[std::min(i0 + 1, pcmMono.size() - 1)];
		out.push_back(s0 + (s1 - s0) * frac);
	}
	return out;
}

std::vector<float> AudioFrontend::normalize(const std::vector<float> &pcm) const {
	if (pcm.empty()) {
		return pcm;
	}
	double sum = 0;
	for (const auto s : pcm) {
		sum += s;
	}
	const auto mean = sum / pcm.size();
	double var = 0;
	for (const auto s : pcm) {
		var += (s - mean) * (s - mean);
	}
	const auto std = std::sqrt(var / pcm.size());
	if (std < 1e-6) {
		return pcm;
	}
	std::vector<float> out;
	out.reserve(pcm.size());
	for (const auto s : pcm) {
		out.push_back(float((s - mean) / std));
	}
	return out;
}

std::vector<std::vector<float>> AudioFrontend::logMel(
		const std::vector<float> &pcmMono,
		int sampleRate,
		const MelConfig &config) const {
// The desktop frontend mirrors the mobile Whisper AudioFrontend: 16kHz
// mono is pre-emphasized, split into overlapping Hamming-windowed
// frames, each frame is scored against a bank of triangular mel
// filters spaced on the mel scale, and log energies form the 80-dim
// vectors consumed by the transcription backend. All constants match
// the mobile defaults so models behave identically on both clients.
	const auto pcm16k = normalize(resampleTo16k(pcmMono, sampleRate));
	const auto frameLen = config.sampleRate * config.frameLengthMs / 1000;
	const auto frameShift = config.sampleRate * config.frameShiftMs / 1000;
	if (pcm16k.size() < size_t(frameLen)) {
		return {};
	}
	std::vector<float> emphasized(pcm16k.size());
	emphasized[0] = pcm16k[0];
	for (size_t i = 1; i < pcm16k.size(); ++i) {
		emphasized[i] = float(pcm16k[i] - config.preEmphasis * pcm16k[i - 1]);
	}
	std::vector<float> window(frameLen);
	for (int i = 0; i < frameLen; ++i) {
		window[i] = float(0.54 - 0.46 * std::cos(2. * kPi * i / (frameLen - 1)));
	}
	const auto hzToMel = [](double hz) {
		return 2595. * std::log10(1. + hz / 700.);
	};
	const auto melToHz = [](double mel) {
		return 700. * (std::pow(10., mel / 2595.) - 1.);
	};
	const auto lowMel = hzToMel(0.);
	const auto highMel = hzToMel(config.sampleRate / 2.);
	const auto fftBins = frameLen / 2 + 1;
	std::vector<std::vector<float>> filterbank(
		config.melBands,
		std::vector<float>(fftBins, 0.f));
	for (int m = 0; m < config.melBands; ++m) {
		const auto f0 = melToHz(lowMel + (highMel - lowMel) * m / (config.melBands + 1));
		const auto f1 = melToHz(lowMel + (highMel - lowMel) * (m + 1) / (config.melBands + 1));
		const auto f2 = melToHz(lowMel + (highMel - lowMel) * (m + 2) / (config.melBands + 1));
		for (int k = 0; k < fftBins; ++k) {
			const auto freq = double(k) * config.sampleRate / frameLen;
			if (freq >= f0 && freq <= f1 && f1 > f0) {
				filterbank[m][k] = float((freq - f0) / (f1 - f0));
			} else if (freq > f1 && freq <= f2 && f2 > f1) {
				filterbank[m][k] = float((f2 - freq) / (f2 - f1));
			}
		}
	}
	std::vector<std::vector<float>> out;
	for (size_t start = 0; start + frameLen <= emphasized.size(); start += frameShift) {
		std::vector<float> power(fftBins, 0.f);
		for (int k = 0; k < fftBins; ++k) {
			double real = 0;
			double imag = 0;
			for (int n = 0; n < frameLen; ++n) {
				const auto sample = emphasized[start + n] * window[n];
				const auto angle = 2. * kPi * k * n / frameLen;
				real += sample * std::cos(angle);
				imag -= sample * std::sin(angle);
			}
			power[k] = float((real * real + imag * imag) / frameLen);
		}
		std::vector<float> mel(config.melBands, 0.f);
		for (int m = 0; m < config.melBands; ++m) {
			double energy = 0;
			for (int k = 0; k < fftBins; ++k) {
				energy += power[k] * filterbank[m][k];
			}
			mel[m] = float(std::log(std::max(energy, 1e-10)));
		}
		out.push_back(std::move(mel));
	}
	return out;
}

std::vector<std::vector<float>> AudioFrontend::mfcc(
		const std::vector<float> &pcmMono,
		int sampleRate,
		const MelConfig &config) const {
	const auto mel = logMel(pcmMono, sampleRate, config);
	std::vector<std::vector<float>> out;
	out.reserve(mel.size());
	for (const auto &frame : mel) {
		std::vector<float> cep(config.mfccCount, 0.f);
		for (int i = 0; i < config.mfccCount; ++i) {
			double sum = 0;
			for (size_t j = 0; j < frame.size(); ++j) {
				sum += frame[j] * std::cos(kPi * i * (j + 0.5) / frame.size());
			}
			cep[i] = float(sum);
		}
		out.push_back(std::move(cep));
	}
	return out;
}

OnnxWhisperBridge &OnnxWhisperBridge::instance() {
	static OnnxWhisperBridge bridge;
	return bridge;
}

OnnxWhisperBridge::OnnxWhisperBridge() {
}

void OnnxWhisperBridge::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_modelPath = MiogramConfig::instance().stringValue(
		ConfigDomain::Ai,
		u"whisperModel"_q,
		cWorkingDir() + u"tdata/whisper-tiny.onnx"_q);
}

bool OnnxWhisperBridge::isModelLoaded() const {
	return !_modelPath.isEmpty() && QFileInfo::exists(_modelPath);
}

QString OnnxWhisperBridge::modelPath() const {
	return _modelPath;
}

void OnnxWhisperBridge::setModelPath(const QString &path) {
	_modelPath = path;
	MiogramConfig::instance().setStringValue(ConfigDomain::Ai, u"whisperModel"_q, path);
}

void OnnxWhisperBridge::transcribeMel(
		const std::vector<std::vector<float>> &mel,
		Fn<void(QString text, QString error)> callback) {
	if (mel.empty()) {
		if (callback) {
			callback(QString(), u"Empty audio"_q);
		}
		return;
	}
	if (!isModelLoaded()) {
		if (callback) {
			callback(
				QString(),
				u"Whisper ONNX model not found. Place whisper-tiny.onnx into tdata/."_q);
		}
		return;
	}
	if (callback) {
		callback(QString(), u"ONNX inference not linked in this build."_q);
	}
}

} // namespace Miogram
