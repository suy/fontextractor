#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QImage>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRawFont>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include "ui_window.h"

QPainterPath paintFromFont(const QString& name,
                           const QString& text = QString("@"),
                           int size = 128)
{
    QRawFont font(name, size);
    if (!font.isValid()) {
        qCritical() << name << "not parsed properly by QRawFont!";
    }
    Q_ASSERT(font.isValid());

    const QList<quint32> indexes = font.glyphIndexesForString(text);
    Q_ASSERT(!indexes.isEmpty());
    const quint32 index = indexes.first();
    if (index == 0)
        qWarning() << "Font" << name << "has index 0!!";

    return font.pathForGlyph(index);
}

QPixmap pathToPixmap(const QPainterPath& path, QSize size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPointF center = path.boundingRect().center();

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor color = Qt::white;
    painter.setPen(color);
    painter.setBrush(color);
    painter.translate(pixmap.rect().center() - center);
    painter.drawPath(path);
    return pixmap;
}

// void pathToImage(const QPainterPath& path, QSize size, const QString& fileName)
// {
//     QImage image(size, QImage::Format_ARGB32);
//     QPainter painter(&image);
//     painter.drawPath(path);
//     image.save(fileName);
// }

QStringList filesFromDirectory(const QString& path)
{
    QStringList result;
    QDirIterator iterator(path, QStringList() << "*.otf" << "*.ttf",
                          QDir::NoDotAndDotDot|QDir::Files, QDirIterator::Subdirectories);
    // qDebug() << "Looking for fonts at" << path << iterator.hasNext();
    while (iterator.hasNext()) {
        const QString fontName = iterator.nextFileInfo().absoluteFilePath();
        result << fontName;
        // qDebug() << fontName;
    }
    return result;
}

class Window : public QWidget {
    Q_OBJECT
public:
    Window();

    // void paintEvent(QPaintEvent*) {
    //     QPainter painter(this);
    //     // painter.setPen(QColor(Qt::red));
    //     painter.setBrush(QColor(Qt::white));
    //     QPointF center = m_path.boundingRect().center();
    //     painter.translate(rect().center() - center);
    //     painter.drawPath(m_path);
    // }

private:
    QPainterPath m_path;
    Ui::Window m_ui;
};

Window::Window() {
    // m_path = paintFromFont(QString("/home/alex/.local/share/fonts/Hello_Doctor.otf"));
    // qDebug() << "path" << m_path;
    // qDebug() << "    > bounding rect" << m_path.boundingRect();
    // qDebug() << "    > bounding rect center" << m_path.boundingRect().center();
    m_ui.setupUi(this);

#if 0
    auto layout = new QVBoxLayout;
    setLayout(layout);

    auto directoryEdit = new QLineEdit(this);
    directoryEdit->setText(QDir::homePath());
    layout->addWidget(directoryEdit);

    auto directoryChange = new QPushButton(tr("Change"), this);
    layout->addWidget(directoryChange);

    auto fontList = new QListWidget(this);
    layout->addWidget(fontList);

    auto glyphs = new QLineEdit(this);
    glyphs->setText("@");
    layout->addWidget(glyphs);

    auto resolution = new QSpinBox(this);
    resolution->setValue(64);
    layout->addWidget(resolution);

    auto generate = new QPushButton(tr("Generate images"), this);
    layout->addWidget(generate);
#endif


    connect(m_ui.changePath, &QPushButton::clicked, this, [=] {
        const QString oldDirectory = m_ui.path->text();
        const QString newDirectory =
            QFileDialog::getExistingDirectory(this, tr("Choose font directory"),
                                              m_ui.path->text());
        if (!newDirectory.isEmpty() && newDirectory != oldDirectory)
            m_ui.path->setText(newDirectory);
    });

    // connect(m_ui.path, &QLineEdit::editingFinished, this, [=] {
    //     m_ui.fontList->clear();
    //     const QStringList files = filesFromDirectory(m_ui.path->text());
    //     // qDebug() << "files from dir" << files;
    //     m_ui.fontList->addItems(files);
    // });

    connect(m_ui.scan, &QPushButton::clicked, this, [this] {
        m_ui.fontList->clear();
        const QStringList files = filesFromDirectory(m_ui.path->text());
        m_ui.fontList->addItems(files);
        m_ui.results->setText(tr("%1 files found").arg(files.size()));
    });

    auto render = [](const QString& font, const QString& glyph, int side) {
        const QSize size(side, side);
        const auto path = paintFromFont(font, glyph, side);
        const QRectF bounds = path.boundingRect();
        if (bounds.width() > size.width() || bounds.height() > size.height()) {
            const QString base = font.section(QLatin1Char('/'), -1).section(QLatin1Char('.'), 0, 0);
            qWarning() << "Font" << base << "doesn't fit properly";
            // return QPixmap();
        }
        return pathToPixmap(path, size);
    };

    auto preview = [this] (const QString& filePath) {
        const QSize size(m_ui.size->value(), m_ui.size->value());
        for (QChar c : m_ui.glyphs->text()) {
            QString glyph(c);
            // const auto path = paintFromFont(filePath, glyph, size);
            const auto path = paintFromFont(filePath, glyph, m_ui.size->value());
            const QRectF bounds = path.boundingRect();
            if (bounds.width() > size.width() || bounds.height() > size.height()) {
                const QString base = filePath.section(QLatin1Char('/'), -1).section(QLatin1Char('.'), 0, 0);
                qWarning() << "Font" << base << "doesn't fit properly";
            }
            const QPixmap pixmap = pathToPixmap(path, size);
            m_ui.preview->setPixmap(pixmap);
        }
    };

    connect(m_ui.fontList, &QListWidget::itemSelectionChanged, this, [=] {
        auto selected = m_ui.fontList->selectedItems();
        if (selected.isEmpty())
            return;
        auto first = selected.first();
        preview(first->text());
    });

    connect(m_ui.fontList, &QListWidget::clicked, this, [=] (const QModelIndex& index) {
        const QString filePath = index.data().toString();
        preview(filePath);
    });

    auto generateOne = [=](const QString& font) {
        const QString base = font.section(QLatin1Char('/'), -1).section(QLatin1Char('.'), 0, 0);
        for (auto glyph : m_ui.glyphs->text()) {
            const int side = m_ui.size->value();
            const QPixmap pixmap = render(font, glyph, side);
            if (pixmap.isNull())
                return;
            QString path = "output/";
            path.append(QString::number(side));
            path.append("/");
            path.append(glyph);
            path.append("/");
            qDebug() << "creating" << path << "in" << QDir::current().path();
            if (!QDir::current().mkpath(path)) {
                qCritical() << "Could not create output directory! Skipping files";
                return;
            }
            path.append(base);
            path.append(".png");
            qDebug() << "saving" << path;

            const bool success = pixmap.save(path);
            if (!success) {
                qWarning() << "Could not create file";
                break;
            }
        }
    };

    connect(m_ui.generateOne, &QPushButton::clicked, this, [=] {
        auto selected = m_ui.fontList->selectedItems();
        if (selected.isEmpty())
            return;
        auto first = selected.first();
        const QString font = first->text();
        generateOne(font);
    });
}


int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString dirName = args.at(1);
        QDirIterator iterator(dirName, QDir::NoDotAndDotDot|QDir::Files);
        qDebug() << "Looking for fonts at" << dirName << iterator.hasNext();
        while (iterator.hasNext()) {
            const QString fontName = iterator.nextFileInfo().absoluteFilePath();
            qDebug() << fontName;
        }
        return 0;
    }

    QImage image(128, 128, QImage::Format_ARGB32);
    // QPainter painter(&image);
    // painter.drawPath(path);
    // image.save("example.png");

    Window window;
    // window.resize(128, 128);
    window.showMaximized();

    return app.exec();
}

#include "main.moc"
