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

#include "NMModelController.h"
#include "NMGlobalHelper.h"

NMLogWidget::NMLogWidget(QWidget *parent) : QTextBrowser(parent)
{
    // ===============================
    // initial settings & message
    // ===============================
    this->setAcceptDrops(false);
    this->setReadOnly(true);
    this->setOpenLinks(false);
    this->setTextInteractionFlags(Qt::NoTextInteraction);

    // ... printing the first log message
    this->clearLog();
    this->zoomOut(2);

}


void
NMLogWidget::insertHtml(const QString& text)
{
    QString worktext = text;

    //insertPlainText(QString("%1\n").arg(worktext));
    QStringList captured;
    QStringList tokens = worktext.split(':', Qt::SkipEmptyParts);
    foreach(const QString& tok, tokens)
    {
        QStringList ttkk = tok.split(' ', Qt::SkipEmptyParts);
        foreach(const QString& tt, ttkk)
        {
            if (!captured.contains(tt))
            {
                if (    NMGlobalHelper::getModelController()
                    &&  NMGlobalHelper::getModelController()->contains(tt)
                   )
                {
                    QString anchortext = QString("<a href=\"#%1\">%1</a>")
                            .arg(tt);
                    QRegExp re(QString("\\b(%1)\\b").arg(tt));
                    worktext = worktext.replace(re, anchortext);
                    captured << tt;
                    //insertPlainText(QString("%1 ==> %2 \n").arg(tt).arg(anchortext));
                }
            }
        }
    }

    QTextCursor cur(this->document());
    cur.movePosition(QTextCursor::End);
    cur.beginEditBlock();
    cur.insertHtml(worktext);
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    cur.endEditBlock();
    this->ensureCursorVisible();
}

void
NMLogWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = new QMenu(this);//createStandardContextMenu();
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
    this->insertPlainText("\n");
}
