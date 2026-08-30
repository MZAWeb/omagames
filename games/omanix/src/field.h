#pragma once

#include <QPoint>
#include <vector>

// What a cell of the playfield is: solid ground the marker walks on, open sea
// the balls roam, or the marker's unfinished cut through the sea.
enum class Cell : quint8 { Claimed, Open, Trail };

// The grid itself: a claimed frame around an open interior, cells addressed
// by (x, y) or by index `y * width + x`. It knows how to close a trail and
// swallow the regions no ball is in, but nothing about who moves where.
class Field {
public:
    static constexpr int kDefaultWidth = 64;
    static constexpr int kDefaultHeight = 40;
    // One cell deep, the same thickness as the marker and its trail.
    static constexpr int kBorder = 1;

    explicit Field(int width = kDefaultWidth, int height = kDefaultHeight);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int cellCount() const { return m_width * m_height; }
    int index(QPoint p) const { return p.y() * m_width + p.x(); }
    QPoint point(int index) const { return {index % m_width, index / m_width}; }
    bool contains(QPoint p) const;
    bool isBorder(QPoint p) const;
    // A claimed cell touching something that is not claimed. Eight-connected
    // on purpose: it is what makes the frame's corners part of the edge, and
    // so the edge a chaser crawls is one unbroken ring.
    bool isEdge(QPoint p) const;

    Cell at(QPoint p) const { return m_cells[size_t(index(p))]; }
    Cell at(int index) const { return m_cells[size_t(index)]; }
    void set(QPoint p, Cell cell) { setCell(index(p), cell); }

    // Counters a renderer caches against instead of comparing every cell:
    // `revision` moves on any change, `groundRevision` only when claimed
    // ground is won or lost — which is what the painted backdrop is made of.
    int revision() const { return m_revision; }
    int groundRevision() const { return m_groundRevision; }

    // Claimed frame, open interior.
    void reset();

    // Interior cells are the ones that count toward the level goal.
    int interiorCells() const;
    int claimedInterior() const;
    double claimedPercent() const;

    std::vector<int> trailCells() const;
    // Trail back to open sea; returns the cells that were wiped.
    std::vector<int> clearTrail();
    // Closes the trail: it becomes claimed, and so does every open region no
    // ball stands in. Returns every newly claimed cell.
    std::vector<int> claim(const std::vector<QPoint> &balls);

private:
    void setCell(int index, Cell cell);

    int m_width;
    int m_height;
    std::vector<Cell> m_cells;
    int m_revision = 0;
    int m_groundRevision = 0;
};
