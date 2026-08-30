#pragma once

#include <QString>

class Table;

// What the Decision Coach has to say: the play to make ("Hit") and the spot it
// applies to ("16 against a 10"), both empty when there is nothing to decide.
struct Advice {
    QString action;
    QString situation;
};

// Basic strategy, worded the way a player would say it out loud.
namespace Coach {

Advice adviceFor(const Table &table);

}
