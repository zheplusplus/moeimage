#include <QPainter>
#include <algorithm>
#include "canvas.h"

using namespace ui;

Canvas::Canvas(QWidget* parent)
    : QWidget(parent)
    , selectedPointFlags(NULL)
    , tolerance(256)
    , selected(false)
{
    resetFlags(QSize(0, 0));
}

Canvas::~Canvas()
{
    delete[] selectedPointFlags;
}

void Canvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawImage(0, 0, displayImage);
}

QSize Canvas::sizeHint() const
{
    return coreImage.size();
}

void Canvas::setImage(QString fileName)
{
    coreImage = QImage(fileName);
    displayImage = coreImage.copy(0, 0, coreImage.width(), coreImage.height());
    resetFlags(coreImage.size());
    emit Painted();
}

void Canvas::saveImage(QString fileName) const
{
    coreImage.save(fileName);
}

void Canvas::resetDisplay()
{
    displayImage = coreImage.copy(0, 0, coreImage.width(), coreImage.height());
    for (int i = 0; i < (coreImage.size().width() + 2) * (coreImage.size().height() + 2); ++i)
    {
        selectedPointFlags[i] = false;
    }
    selected = false;
    selectedPoints.clear();
    emit Painted();
}

void Canvas::saveModify()
{
    for (int i = 0; i < selectedPoints.size(); ++i)
    {
        coreImage.setPixel(selectedPoints[i], displayImage.pixel(selectedPoints[i]));
    }
    resetDisplay();
}

void Canvas::setColorTolerance(int t)
{
    tolerance = t;
}

void Canvas::selectPoint(QPoint point)
{
    lastSelectPoint = point;
    selected = true;
    refreshDisplayImage();
}

void Canvas::resetFlags(QSize size)
{
    delete[] selectedPointFlags;
    selectedPointFlags = new bool[(size.width() + 2) * (size.height() + 2)];
    for (int i = 0; i < (size.width() + 2) * (size.height() + 2); ++i)
    {
        selectedPointFlags[i] = false;
    }
}

void Canvas::refreshDisplayImage()
{
    if (!selected)
    {
        return;
    }
    displayImage = coreImage.copy(0, 0, coreImage.width(), coreImage.height());
    find4Directions(lastSelectPoint, lastSelectPoint);
    for (int y = 0; y < coreImage.height(); ++y)
    {
        for (int x = 0; x < coreImage.width(); ++x)
        {
            QPoint point(x, y);
            if (!selectedPointFlags[pointToIndex(point)])
            {
                displayImage.setPixel(point, QColor(coreImage.pixel(point)).darker().rgb());
            }
        }
    }
    update();
}

void Canvas::find4Directions(QPoint p, QPoint reference)
{
    if (p.x() < 0 || p.y() < 0 || p.x() >= coreImage.width() || p.y() >= coreImage.height())
    {
        return;
    }
    if (selectedPointFlags[pointToIndex(p)])
    {
        return;
    }
    if (!(selectedPointFlags[pointToIndex(p)] = maskAsSelected(p, reference)))
    {
        return;
    }
    selectedPoints.push_back(p);
    find4Directions(p + QPoint(-1, 0), p);
    find4Directions(p + QPoint(1, 0), p);
    find4Directions(p + QPoint(0, 1), p);
    find4Directions(p + QPoint(0, -1), p);
}

bool Canvas::maskAsSelected(QPoint p, QPoint reference) const
{
    if (p.x() < 0 || p.y() < 0 || p.x() >= coreImage.width() || p.y() >= coreImage.height())
    {
        return true;
    }
    QRgb rgb = coreImage.pixel(p);
    QRgb rgb0 = coreImage.pixel(reference);
    return ((qRed(rgb) - qRed(rgb0)) * (qRed(rgb) - qRed(rgb0))
          + (qBlue(rgb) - qBlue(rgb0)) * (qBlue(rgb) - qBlue(rgb0))
          + (qGreen(rgb) - qGreen(rgb0)) * (qGreen(rgb) - qGreen(rgb0))
           ) < tolerance;
}

int Canvas::pointToIndex(QPoint p)
{
    return (p.x() + 1) * coreImage.height() + (p.y() + 1);
}

void Canvas::mousePressEvent(QMouseEvent* event)
{
    selectPoint(QPoint(event->x(), event->y()));
}

template <typename _PixelMaker>
void Canvas::SwapColor(_PixelMaker pm)
{
    for (int i = 0; i < selectedPoints.size(); ++i)
    {
        displayImage.setPixel(selectedPoints[i], pm(displayImage.pixel(selectedPoints[i])));
    }
}

struct RBSwp
{
    QRgb operator()(QRgb const& rgb) const
    {
        return QColor::fromRgb(qBlue(rgb), qGreen(rgb), qRed(rgb)).rgb();
    }
};

struct GBSwp
{
    QRgb operator()(QRgb const& rgb) const
    {
        return QColor::fromRgb(qRed(rgb), qBlue(rgb), qGreen(rgb)).rgb();
    }
};

struct RGSwp
{
    QRgb operator()(QRgb const& rgb) const
    {
        return QColor::fromRgb(qGreen(rgb), qRed(rgb), qBlue(rgb)).rgb();
    }
};

void Canvas::SwapRB()
{
    SwapColor(RBSwp());
    emit Painted();
}

void Canvas::SwapGB()
{
    SwapColor(GBSwp());
    emit Painted();
}

void Canvas::SwapRG()
{
    SwapColor(RGSwp());
    emit Painted();
}