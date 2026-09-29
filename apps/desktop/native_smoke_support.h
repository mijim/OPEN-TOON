#pragma once

#include <QGuiApplication>
#include <QTimer>
#include <exception>
#include <iostream>
#include <utility>

template <class Work> void scheduleNativeSmoke(QGuiApplication& app, Work&& work) {
    QTimer::singleShot(1200, &app, [&app, task = std::forward<Work>(work)]() mutable {
        try {
            task();
            app.exit(0);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            app.exit(1);
        }
    });
}
