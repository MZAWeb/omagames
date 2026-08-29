#include "appsetup.h"

#include "omarchytheme.h"
#include "systemtheme.h"

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>

namespace OmaGames {

OmarchyTheme *setupApplication(QGuiApplication &app, const AppInfo &info) {
    app.setApplicationName(info.name);
    app.setApplicationDisplayName(info.displayName);
    app.setDesktopFileName(info.name);
    app.setWindowIcon(QIcon::fromTheme(info.name));
    app.setOrganizationName(QStringLiteral("Omacom"));
    app.setOrganizationDomain(QStringLiteral("omacom.io"));

    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Bold.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Italic.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-BoldItalic.ttf"));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    auto *theme = new OmarchyTheme(&app);
    const QStringList args = app.arguments();
    const int themeArg = args.indexOf(QStringLiteral("--theme-file"));
    if (themeArg >= 0 && themeArg + 1 < args.size())
        theme->setColorsPath(args.at(themeArg + 1));

    auto *systemTheme = new SystemTheme(&app);
    theme->setDarkMode(systemTheme->darkMode());
    QObject::connect(systemTheme, &SystemTheme::darkModeChanged, theme, &OmarchyTheme::setDarkMode);

    // Fold the desktop text scale into the default font so Qt Quick Controls
    // chrome (menus, dialogs) grows with the desktop; games read
    // theme.textScale for their own layouts.
    const QFont interfaceFont(QStringLiteral("iA Writer Mono S"));
    const qreal basePointSize = interfaceFont.pointSizeF() > 0
        ? interfaceFont.pointSizeF() : app.font().pointSizeF();
    const auto applyFont = [&app, interfaceFont, basePointSize](qreal scale) {
        QFont scaled = interfaceFont;
        scaled.setPointSizeF(basePointSize * scale);
        app.setFont(scaled);
    };
    applyFont(systemTheme->textScale());
    theme->setTextScale(systemTheme->textScale());
    QObject::connect(systemTheme, &SystemTheme::textScaleChanged, theme,
                     [theme, applyFont](qreal scale) {
        applyFont(scale);
        theme->setTextScale(scale);
    });

    return theme;
}

void setupEngine(QQmlApplicationEngine &engine, OmarchyTheme *theme) {
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), theme);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &engine,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings)
            qWarning().noquote() << warning.toString();
    });
}

}
