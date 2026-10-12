// Compile against the actual engine with a deterministic clock; no LED hardware involved.
#include <QDateTime>
#include <cassert>
struct EventFxTestClock {
    static inline qint64 now = 10000;
    static qint64 currentMSecsSinceEpoch() { return now; }
};
#define QDateTime EventFxTestClock
#include "../../libsrc/hyperion/RGBBridgeEventFX.cpp"
#undef QDateTime

int main()
{
    const ColorRgb base{5, 9, 13};
    for (int count : {1, 2, 9, 10, 68})
    for (const char* effect : {"projectile", "shockwave", "flash", "pulse", "progress", "sparks"})
    for (bool reverse : {false, true})
    {
        const int start = 3, shift = count - 1;
        QJsonObject json{{"effect", effect}, {"start", start}, {"count", count},
            {"origin", start}, {"duration", 1000}, {"options", QJsonObject{{"reverse", reverse}}}};
        RGBBridgeEventFX original, rotated;
        EventFxTestClock::now = 10000;
        assert(original.trigger(json, count + 6).isEmpty());
        json["origin"] = start + shift;
        assert(rotated.trigger(json, count + 6).isEmpty());
        for (int elapsed : {0, 25, 125, 400, 750, 999}) {
            EventFxTestClock::now = 10000 + elapsed;
            QVector<ColorRgb> a(count + 6, base), b = a;
            assert(original.apply(a) && rotated.apply(b));
            for (int i = 0; i < count; ++i) assert(a[start + i] == b[start + (i + shift) % count]);
            assert(a[0] == base && b.back() == base);
            if (QString(effect) == "shockwave")
                for (int i = 1; i < count; ++i) assert(a[start + i] == a[start + count - i]);
        }
        EventFxTestClock::now = 11000;
        QVector<ColorRgb> restored(count + 6, base);
        assert(!rotated.apply(restored));
        for (const auto& color : restored) assert(color == base);
    }
    RGBBridgeEventFX engine;
    assert(!engine.trigger(QJsonObject{{"origin", 99}}, 10).isEmpty());
    assert(!engine.trigger(QJsonObject{{"origin", 1.5}}, 10).isEmpty());
    assert(!engine.trigger(QJsonObject{{"origin", -1}}, 10).isEmpty());
    assert(!engine.trigger(QJsonObject{{"origin", 0}}, 0).isEmpty());
    // Legacy Shockwave keeps its original center when origin is omitted.
    EventFxTestClock::now = 20000;
    assert(engine.trigger(QJsonObject{{"effect", "shockwave"}}, 9).isEmpty());
    QVector<ColorRgb> legacy(9, ColorRgb{0,0,0});
    engine.apply(legacy);
    assert(legacy[4].red > legacy[0].red);
    std::cout << "PASS: all effects, both directions, rotation, circular symmetry, range isolation, expiry, validation and legacy origin\n";
}
