#include "NMListWidget.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

NMListWidget::NMListWidget(QWidget *parent)
   : QListWidget(parent)
{
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
