#include "treedemowindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    // У нас только русская локаль, поэтому перевод загружаем прямо по пути
    QTranslator translator{};
    if(QLocale().language() == QLocale::Language::Russian &&
       translator.load("TreeDemo_ru_RU",
                       QCoreApplication::applicationDirPath())) {
        QCoreApplication::installTranslator(&translator);
    }

    TreeDemoWindow w;
    w.show();
    return a.exec();
}
