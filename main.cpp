#include <QPainterPath>
#include <QtCore>
#include <QApplication>
#include <QRawFont>
#include <QPainter>
#include <QImage>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QListWidget>
#include <QSpinBox>

QPainterPath pathFromFont(const QString& name,
                          const QString& text = QString("@"),
                          int size = 128)
{
    QRawFont font(name, size);
    Q_ASSERT(font.isValid());

    const QList<quint32> indexes = font.glyphIndexesForString(text);
    Q_ASSERT(!indexes.isEmpty());
    const quint32 index = indexes.first();

    return font.pathForGlyph(index);
}

void pathToImage(const QPainterPath& path, QSize size, const QString& fileName)
{
    QImage image(size, QImage::Format_ARGB32);
    QPainter painter(&image);
    painter.drawPath(path);
    image.save(fileName);
}

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
};

Window::Window() {
    // m_path = pathFromFont(QString("/home/alex/.local/share/fonts/Hello_Doctor.otf"));
    // qDebug() << "path" << m_path;
    // qDebug() << "    > bounding rect" << m_path.boundingRect();
    // qDebug() << "    > bounding rect center" << m_path.boundingRect().center();

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

    connect(directoryChange, &QPushButton::clicked, this, [=] {
        const QString oldDirectory = directoryEdit->text();
        const QString newDirectory =
            QFileDialog::getExistingDirectory(this, tr("Choose font directory"),
                                              directoryEdit->text());
        if (!newDirectory.isEmpty() && newDirectory != oldDirectory)
            directoryEdit->setText(newDirectory);
    });

    connect(directoryEdit, &QLineEdit::editingFinished, this, [=] {
        fontList->clear();
        const QStringList files = filesFromDirectory(directoryEdit->text());
        // qDebug() << "files from dir" << files;
        fontList->addItems(files);
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
