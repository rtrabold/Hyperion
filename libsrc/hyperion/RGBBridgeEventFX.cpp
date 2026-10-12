#include <hyperion/RGBBridgeEventFX.h>

#include <QDateTime>
#include <QJsonArray>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr int MAX_EVENTS = 8;

float clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

float smooth01(float value)
{
	value = clamp01(value);
	return value * value * (3.0f - 2.0f * value);
}

quint8 mixByte(quint8 from, quint8 to, float amount)
{
	return static_cast<quint8>(float(from) + (float(to) - float(from)) * clamp01(amount) + 0.5f);
}

ColorRgb mixColor(const ColorRgb& from, const ColorRgb& to, float amount)
{
	return { mixByte(from.red, to.red, amount), mixByte(from.green, to.green, amount), mixByte(from.blue, to.blue, amount) };
}

quint32 hash32(quint32 value)
{
	value ^= value >> 16; value *= 0x7feb352dU;
	value ^= value >> 15; value *= 0x846ca68bU;
	return value ^ (value >> 16);
}

ColorRgb readColor(const QJsonValue& value, const ColorRgb& fallback)
{
	const QJsonArray values = value.toArray();
	if (values.size() < 3) return fallback;
	return {
		static_cast<quint8>(std::clamp(values[0].toInt(), 0, 255)),
		static_cast<quint8>(std::clamp(values[1].toInt(), 0, 255)),
		static_cast<quint8>(std::clamp(values[2].toInt(), 0, 255))
	};
}

float projectileAt(float head, int pixel, int pointSize, int tailSize)
{
	const float lead = clamp01(head - float(pixel));
	const float back = clamp01(float(pixel + 1) - (head - pointSize));
	const float point = std::min(lead, back);
	float tail = 0.0f;
	if (tailSize > 0)
	{
		const float behind = (head - pointSize) - (float(pixel) + 0.5f);
		if (behind >= 0.0f && behind < tailSize)
			tail = 0.72f * (1.0f - behind / float(tailSize));
	}
	return std::max(point, tail);
}

struct Pixel
{
	ColorRgb color { 0, 0, 0 };
	float alpha { 0.0f };
	ColorRgb center { 0, 0, 0 };
	float centerAlpha { 0.0f };
};
}

QString RGBBridgeEventFX::trigger(const QJsonObject& command, int ledCount)
{
	if (ledCount <= 0) return QStringLiteral("EventFX requires at least one LED");
	QJsonArray definitions = command.value("effects").toArray();
	if (definitions.isEmpty()) definitions.append(command);
	if (definitions.size() > MAX_EVENTS) return QStringLiteral("RGBBridge EventFX accepts at most 8 effects per event");

	QVector<Event> pending;
	pending.reserve(definitions.size());
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	for (const QJsonValue& value : definitions)
	{
		const QJsonObject json = value.toObject();
		Event event;
		const QString effect = json.value("effect").toString("projectile");
		if (effect == "projectile") event.kind = Kind::Projectile;
		else if (effect == "shockwave") event.kind = Kind::Shockwave;
		else if (effect == "flash") event.kind = Kind::Flash;
		else if (effect == "pulse") event.kind = Kind::Pulse;
		else if (effect == "progress") event.kind = Kind::Progress;
		else if (effect == "sparks") event.kind = Kind::Sparks;
		else return QStringLiteral("Unknown RGBBridge EventFX effect: %1").arg(effect);

		event.durationMs = json.value("duration").toInt(350);
		if (event.durationMs < 50 || event.durationMs > 10000) return QStringLiteral("EventFX duration must be between 50 and 10000 ms");
		event.start = std::clamp(json.value("start").toInt(0), 0, std::max(0, ledCount - 1));
		event.count = json.value("count").toInt(0);
		if (event.count <= 0) event.count = ledCount - event.start;
		event.count = std::clamp(event.count, 1, std::max(1, ledCount - event.start));
		if (json.contains("origin"))
		{
			const QJsonValue origin = json.value("origin");
			event.origin = origin.toInt(-1);
			if (!origin.isDouble() || origin.toDouble() != double(event.origin) ||
				event.origin < event.start || event.origin - event.start >= event.count)
				return QStringLiteral("EventFX origin must be a physical LED within the selected range");
		}
		event.brightness = std::clamp(json.value("brightness").toInt(255), 0, 255);
		event.overlay = json.value("overlay").toBool(true);
		event.background = readColor(json.value("backgroundColor"), event.background);

		const QJsonObject options = json.value("options").toObject();
		event.color = readColor(options.value("color"), event.color);
		event.colorA = readColor(options.value("colorA"), event.colorA);
		event.colorB = readColor(options.value("colorB"), event.colorB);
		event.reverse = options.value("reverse").toBool(false);
		event.secondSparkColor = options.contains("colorB");
		event.pointSize = std::clamp(options.value("pointSize").toInt(3), 1, 32);
		event.tailSize = std::clamp(options.value("tailSize").toInt(5), 0, 64);
		event.waveWidth = std::clamp(options.value("waveWidth").toInt(3), 1, 32);
		event.pulses = std::clamp(options.value("pulses").toInt(2), 1, 20);
		event.holdMs = std::clamp(options.value("holdMs").toInt(40), 0, 10000);
		event.sparkDensity = std::clamp(options.value("sparkDensity").toInt(12), 1, 100);
		event.sparkLifeMs = std::clamp(options.value("sparkLifeMs").toInt(90), 30, 1000);
		event.startedMs = now;
		event.seed = hash32(static_cast<quint32>(now) ^ static_cast<quint32>(_events.size() + pending.size() + 1));
		pending.append(event);
	}

	while (_events.size() + pending.size() > MAX_EVENTS && !_events.isEmpty()) _events.removeFirst();
	_events.append(pending);
	return {};
}

bool RGBBridgeEventFX::apply(QVector<ColorRgb>& colors)
{
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	_events.erase(std::remove_if(_events.begin(), _events.end(), [now](const Event& event) {
		return now - event.startedMs >= event.durationMs;
	}), _events.end());
	if (_events.isEmpty()) return false;

	for (const Event& event : std::as_const(_events))
	{
		const int count = std::min(event.count, static_cast<int>(colors.size()) - event.start);
		if (count <= 0) continue;
		if (event.origin >= event.start + count) continue; // Layout changed during the event.
		if (!event.overlay)
			std::fill(colors.begin() + event.start, colors.begin() + event.start + count, event.background);

		const quint32 elapsed = static_cast<quint32>(now - event.startedMs);
		const float progress = float(elapsed) / float(event.durationMs);
		for (int physical = 0; physical < count; ++physical)
		{
			const bool circular = event.origin >= 0;
			const int relativeOrigin = event.origin - event.start;
			const int led = circular
				? (event.reverse ? relativeOrigin - physical + count : physical - relativeOrigin + count) % count
				: (event.reverse ? count - 1 - physical : physical);
			Pixel pixel;
			pixel.color = event.color;
			switch (event.kind)
			{
			case Kind::Projectile:
				pixel.alpha = projectileAt(progress * float(count + event.pointSize + event.tailSize), led, event.pointSize, event.tailSize);
				break;
			case Kind::Shockwave:
			{
				const float center = (float(count) - 1.0f) * 0.5f;
				const float distance = circular ? float(std::min(led, count - led)) : std::abs(float(led) - center);
				const float width = float(event.waveWidth);
				const float reach = circular ? float(count / 2) : center;
				const float radius = progress * (reach + 1.0f + width);
				const float behind = radius - distance;
				pixel.alpha = clamp01(behind) * clamp01((1.0f + width - behind) / width);
				pixel.color = mixColor(event.colorB, event.colorA, clamp01(behind / width));
				pixel.center = event.colorA;
				pixel.centerAlpha = clamp01(std::exp(-distance / std::max(0.75f, width * 0.7f)) * clamp01(1.0f - progress / 0.30f) * 1.2f);
				break;
			}
			case Kind::Flash:
			{
				const int hold = std::min(event.holdMs, event.durationMs - 1);
				const float x = elapsed <= static_cast<quint32>(hold) ? 0.0f : float(elapsed - hold) / float(event.durationMs - hold);
				pixel.alpha = 1.0f - smooth01(x);
				break;
			}
			case Kind::Pulse:
			{
				const float phase = progress * float(event.pulses);
				pixel.alpha = std::sin(float(M_PI) * (phase - std::floor(phase)));
				break;
			}
			case Kind::Progress:
				pixel.alpha = clamp01(progress * float(count) - float(led));
				break;
			case Kind::Sparks:
			{
				const quint32 interval = static_cast<quint32>(event.sparkLifeMs);
				const quint32 slot = elapsed / interval;
				const quint32 salt = hash32(event.seed ^ (quint32(led + 1) * 0x9e3779b9U));
				float best = 0.0f; quint32 bestHash = 0;
				for (quint32 offset = 0; offset <= 1 && slot >= offset; ++offset)
				{
					const quint32 bucket = slot - offset;
					const quint32 hash = hash32(salt ^ (bucket * 0x85ebca6bU));
					if (hash % 100U >= static_cast<quint32>(event.sparkDensity)) continue;
					const float age = float(elapsed - bucket * interval) / float(interval * 2U);
					const float intensity = 1.0f - smooth01(age);
					if (intensity > best) { best = intensity; bestHash = hash; }
				}
				pixel.alpha = best * clamp01(float(event.durationMs - int(elapsed)) / float(interval));
				if (event.secondSparkColor && (bestHash & 1U)) pixel.color = event.colorB;
				break;
			}
			}

			const float scale = float(event.brightness) / 255.0f;
			ColorRgb& output = colors[event.start + physical];
			output = mixColor(output, pixel.color, clamp01(pixel.alpha) * scale);
			output = mixColor(output, pixel.center, clamp01(pixel.centerAlpha) * scale);
		}
	}
	return true;
}

void RGBBridgeEventFX::clear()
{
	_events.clear();
}
