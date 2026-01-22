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
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

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
    //this->clearLog();
    this->zoomOut(2);

    mLightBlue = QColor::fromRgb(42,121,216);
    mBlue      = QColor::fromRgb(0,0,255);
    mLightRed  = QColor::fromRgb(248,70,75);
    mRed       = QColor::fromRgb(255,0,0);
}

void
NMLogWidget::setDarkMode(bool bDarkMode)
{
    QBrush blueBrush;
    QBrush redBrush;
    if (bDarkMode)
    {
        blueBrush = QBrush(QColor(mLightBlue));
        redBrush = QBrush(QColor(mLightRed));
    }
    else
    {
        blueBrush = QBrush(QColor(mBlue));
        redBrush = QBrush(QColor(mRed));
    }

    if (!this->document()->isEmpty())
    {
        QTextCharFormat blue_text, red_text;
        blue_text.setForeground(blueBrush);
        red_text.setForeground(redBrush);

        this->moveCursor(QTextCursor::Start);
        QTextCursor cursor = this->textCursor();

        while (!cursor.atEnd())
        {
            const int sp = cursor.position();
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            const int ep = cursor.position();
            QString selText = cursor.selectedText();

            if (    selText.compare(QStringLiteral("WARNING"), Qt::CaseSensitive) == 0
                 || NMGlobalHelper::getModelController()->contains(selText)
               )
            {
                cursor.mergeCharFormat(blue_text);
            }
            else if (selText.compare(QStringLiteral("ERROR"), Qt::CaseSensitive) == 0)
            {
                cursor.mergeCharFormat(red_text);
            }

            if (selText.compare(QStringLiteral("(") == 0))
            {
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
            }
            else
            {
                cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
            }
        }
    }
}

void
NMLogWidget::insertHtml(const QString& text)
{
    QString worktext = text;
    QRegularExpression regexp("\\(([a-zA-Z0-9]+)\\)");

    QString colour = "#0000FF";
    if (NMGlobalHelper::getMainWindow()->isInDarkMode())
    {
        colour = "#2A79D8";
    }

    //insertPlainText(QString("%1\n").arg(worktext));
    QStringList captured;
    QStringList tokens = worktext.split(':', Qt::SkipEmptyParts);
    foreach(const QString& tok, tokens)
    {
        QStringList ttkk = tok.split(' ', Qt::SkipEmptyParts);
        foreach(const QString& tt, ttkk)
        {
            QString bracedToken = tt;
            QRegularExpressionMatchIterator mit = regexp.globalMatch(tt);
            if (mit.hasNext())
            {
                bracedToken = mit.next().captured(1);
            }

            if (!captured.contains(bracedToken))
            {
                if (    NMGlobalHelper::getModelController()
                    &&  NMGlobalHelper::getModelController()->contains(bracedToken)
                   )
                {
                    QString anchortext = QString("<a style=\"color: %2\"  href=\"#%1\">%1</a>")
                            .arg(bracedToken).arg(colour);
                    QRegExp re(QString("\\b(%1)\\b").arg(bracedToken));
                    worktext = worktext.replace(re, anchortext);
                    captured << bracedToken;
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
