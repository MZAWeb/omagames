#pragma once

#include <QJsonObject>
#include <QString>
#include <QVariantList>

// How the controls feel, as opposed to what the rules are: delayed auto shift,
// the auto-repeat rate and the soft drop speed. They are the player's to tune,
// so they live apart from Rules and outlive every run; the defaults are the
// values Omatris always played with. Times are ticks of the sixty-a-second
// clock, the only unit the game counts in.
struct Handling {
    enum Setting { Das, Arr, SoftDrop };
    static constexpr int kSettingCount = 3;

    static constexpr int kMinDasTicks = 1;
    static constexpr int kMaxDasTicks = 20;
    // Zero repeat ticks is "instant": past the delay the piece goes straight
    // to the wall.
    static constexpr int kMinArrTicks = 0;
    static constexpr int kMaxArrTicks = 10;
    // A soft drop factor of zero falls to the floor in one tick without
    // locking, the fastest a soft drop can be.
    static constexpr int kInstantSoftDrop = 0;

    int dasTicks;
    int arrTicks;
    int softDropFactor;

    // What a fresh install plays with.
    static Handling defaults();
    bool isDefault() const;
    bool operator==(const Handling &other) const;
    bool operator!=(const Handling &other) const { return !(*this == other); }

    // One step up (+1) or down (-1) the setting's range; false, and nothing
    // changed, at the end of it.
    bool step(Setting setting, int delta);
    bool canStep(Setting setting, int delta) const;

    // Anything missing or out of range falls back to its default, so a hand-
    // edited or older config can never make the game unplayable.
    QJsonObject toJson() const;
    static Handling fromJson(const QJsonObject &json);
    // QSettings under `handling/v1`. Only a choice that differs from the
    // defaults is written down, so a player who never touched it, or reset
    // it, follows whatever the defaults become.
    static Handling load();
    void save() const;

    // {id, label, description, value, canLower, canRaise, isDefault} per
    // setting, in panel order.
    QVariantList rows() const;

    static QString id(Setting setting);
    static bool fromId(const QString &wanted, Setting *setting);
    static int ticksToMs(int ticks);
};
