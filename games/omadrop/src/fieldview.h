#pragma once

#include <QColor>
#include <QQuickPaintedItem>

class DropEngine;
class OmadropGame;

class DropFieldView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QColor fieldColor MEMBER m_fieldColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor borderColor MEMBER m_borderColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor guideColor MEMBER m_guideColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor ballColor MEMBER m_ballColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor textColor MEMBER m_textColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor faintColor MEMBER m_faintColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor oneColor MEMBER m_oneColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor twoColor MEMBER m_twoColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor threeColor MEMBER m_threeColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor fourColor MEMBER m_fourColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor fiveColor MEMBER m_fiveColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor sixColor MEMBER m_sixColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor sevenColor MEMBER m_sevenColor NOTIFY colorsChanged)

public:
    explicit DropFieldView(QQuickItem *parent = nullptr);

    QObject *source() const;
    void setSource(QObject *source);
    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void colorsChanged();

private:
    const DropEngine *engine() const;
    QColor pegColor(int hits) const;
    QPointF point(QPointF logical) const;
    double scale() const;
    void paintGuide(QPainter *painter, const DropEngine &game);
    void paintPegs(QPainter *painter, const DropEngine &game);
    void paintBall(QPainter *painter, const DropEngine &game);

    OmadropGame *m_source = nullptr;
    QColor m_fieldColor;
    QColor m_borderColor;
    QColor m_guideColor;
    QColor m_ballColor;
    QColor m_textColor;
    QColor m_faintColor;
    QColor m_oneColor;
    QColor m_twoColor;
    QColor m_threeColor;
    QColor m_fourColor;
    QColor m_fiveColor;
    QColor m_sixColor;
    QColor m_sevenColor;
};
