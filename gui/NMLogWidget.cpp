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
#include "NMLogWidget.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QDate>
#include <QTime>

NMLogWidget::NMLogWidget(QWidget *parent) : QTextEdit(parent)
{
    // ===============================
    // initial settings & message
    // ===============================
    this->setAcceptDrops(false);
    this->setReadOnly(true);

    // ... printing the first log message
    this->clearLog();
    this->zoomOut(2);
}

void
NMLogWidget::insertHtml(const QString& text)
{
    QTextEdit::insertHtml(text);
    moveCursorToEnd();
}

void
NMLogWidget::moveCursorToEnd(void)
{
    this->textCursor().movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
}

void
NMLogWidget::mousePressEvent(QMouseEvent* e)
{
    // in case the user clicks somewhere, we
    // want to make sure that text is appended
    // only and not inserted somewhere in the
    // middle of a previous log message ...
    moveCursorToEnd();
}

void
NMLogWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu();
    QAction* clearAct = menu->addAction(tr("Clear"));

    connect(clearAct, SIGNAL(triggered()), this,
            SLOT(clearLog()));

    menu->exec(event->globalPos());
    delete menu;
}

void
NMLogWidget::clearLog(void)
{
    this->clear();

    QString logstart = QString("<h4>LUMASS GUI - %1, %2</h4>")
            .arg(QDate::currentDate().toString())
            .arg(QTime::currentTime().toString());

    this->insertHtml(logstart);

    QTextBlockFormat blFormat = this->textCursor().blockFormat();
    blFormat.setBottomMargin(1);
    blFormat.setTopMargin(1);
    this->textCursor().setBlockFormat(blFormat);
    this->insertPlainText("\n");
    this->ensureCursorVisible();
}
