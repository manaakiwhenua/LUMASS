/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef NMLOGWIDGET_H
#define NMLOGWIDGET_H

#include <QObject>
#include <QTextEdit>

class NMLogWidget : public QTextEdit
{
    Q_OBJECT

public:
    NMLogWidget(QWidget* parent=0);

public slots:
    void insertHtml(const QString& text);


protected slots:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* e);

    void clearLog(void);
    void moveCursorToEnd(void);

};

#endif // NMLOGWIDGET_H
