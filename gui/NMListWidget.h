#ifndef NMLISTWIDGET_H
#define NMLISTWIDGET_H

#include <QObject>
#include <QListWidget>

class NMLogger;

class NMListWidget : public QListWidget
{
    Q_OBJECT
public:
    NMListWidget(QWidget* parent=0);

    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);

    void setLogger(NMLogger* logger){mLogger = logger;}

protected:
    NMLogger* mLogger;
};

#endif // NMLISTWIDGET_H
