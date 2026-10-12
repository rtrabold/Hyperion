#pragma once

#include <QJsonObject>
#include <QVector>
#include <QString>
#include <utils/ColorRgb.h>

/// Local RGBBridge event overlay. It never registers a priority: frames are
/// blended over the source selected by Hyperion immediately before output.
class RGBBridgeEventFX
{
public:
	QString trigger(const QJsonObject& command, int ledCount);
	bool apply(QVector<ColorRgb>& colors);
	void clear();
	bool active() const { return !_events.isEmpty(); }

private:
	enum class Kind { Projectile, Shockwave, Flash, Pulse, Progress, Sparks };
	struct Event
	{
		Kind kind { Kind::Projectile };
		qint64 startedMs { 0 };
		int durationMs { 350 };
		int start { 0 };
		int count { 0 };
		int origin { -1 }; // Physical LED; -1 keeps the legacy linear mapping.
		int brightness { 255 };
		ColorRgb color { 255, 0, 0 };
		ColorRgb colorA { 255, 230, 160 };
		ColorRgb colorB { 255, 69, 0 };
		ColorRgb background { 0, 0, 0 };
		bool overlay { true };
		bool reverse { false };
		bool secondSparkColor { false };
		int pointSize { 3 };
		int tailSize { 5 };
		int waveWidth { 3 };
		int pulses { 2 };
		int holdMs { 40 };
		int sparkDensity { 12 };
		int sparkLifeMs { 90 };
		quint32 seed { 0 };
	};

	QVector<Event> _events;
};
