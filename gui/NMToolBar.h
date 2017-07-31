/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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

#ifndef NMTOOLBAR_H
#define NMTOOLBAR_H

#include <QToolBar>

class NMLogger;
class QMouseEvent;
class QWidget;
class QString;

class NMToolBar : public QToolBar
{
    Q_OBJECT
public:
    NMToolBar(const QString& title, QWidget* parent = Q_NULLPTR);
    NMToolBar(QWidget* parent = Q_NULLPTR);

    void setLogger(NMLogger* logger)
        {mLogger = logger;}

    void mousePressEvent(QMouseEvent* event);

signals:
    void signalPopupMenu();

protected slots:
    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent* event);

protected:
    NMLogger* mLogger;


};

#endif // NMTOOLBAR_H
