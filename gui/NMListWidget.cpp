#include "NMListWidget.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

NMListWidget::NMListWidget(QWidget *parent)
   : QListWidget(parent)
{
    this->viewport()->installEventFilter(this);
}

void NMListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mDragStartPosition = event->pos();
    }
}

void NMListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }

    if ((event->pos() - mDragStartPosition).manhattanLength()
             < QApplication::startDragDistance()
       )
    {
        return;
    }

    QListWidgetItem* item = this->itemAt(mDragStartPosition);
    if (item == 0)
    {
        return;
    }

#ifdef QT_HIGHDPI_SUPPORT
    qreal dpr = this->devicePixelRatioF();
#else
    qreal dpr = 1;
#endif

    QSize dragImageSize(16*dpr, 16*dpr);
    QImage dragImage(dragImageSize, QImage::Format_ARGB32_Premultiplied);

    QPainter dragPainter(&dragImage);
    dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
    dragPainter.fillRect(dragImage.rect(), Qt::transparent);

    dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    dragPainter.drawPixmap(0,0,16*dpr,16*dpr, item->icon().pixmap(dragImageSize));
    dragPainter.end();

    QDrag* drag = new QDrag(this);
    drag->setPixmap(QPixmap::fromImage(dragImage));
    drag->setDragCursor(QPixmap(":move-icon.png"), Qt::CopyAction);

    QMimeData* mimeData = new QMimeData;
    mimeData->setText(QString::fromLatin1("_%1_:%2").arg(mDragSourceName).arg(item->text()));
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction, Qt::CopyAction);
}

void NMListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void NMListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void NMListWidget::dropEvent(QDropEvent* event)
{

}

void NMListWidget::dragLeaveEvent(QDragLeaveEvent *event)
{

}





