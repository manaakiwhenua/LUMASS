#ifndef NMLISTWIDGET_H
#define NMLISTWIDGET_H

#include <QObject>
#include <QListWidget>
#include "NMObject.h"

class NMLogger;

class NMListWidget : public QListWidget, public NMObject
{
    Q_OBJECT
public:
    NMListWidget(QWidget* parent=0);

    void setDragSourceName(const QString& name)
        {mDragSourceName = name;}

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);
    void dragLeaveEvent(QDragLeaveEvent* event);

    QPoint mDragStartPosition;
    QString mDragSourceName;


};

#endif // NMLISTWIDGET_H
