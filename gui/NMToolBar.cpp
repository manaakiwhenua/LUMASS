/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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

#include "NMToolBar.h"

#include <QWidget>
#include <QMouseEvent>
#include <QString>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

NMToolBar::NMToolBar(const QString& title, QWidget* parent)
    : QToolBar(title, parent)
{
}

NMToolBar::NMToolBar(QWidget* parent)
    : QToolBar(parent)
{
}

void
NMToolBar::dragEnterEvent(QDragEnterEvent* event)
{

}

void
NMToolBar::dragMoveEvent(QDragMoveEvent *event)
{

}

void
NMToolBar::dragLeaveEvent(QDragLeaveEvent* event)
{

}


void
NMToolBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        NMLogDebug(<< "context menu called .. ");
        emit signalPopupMenu();
    }
}
