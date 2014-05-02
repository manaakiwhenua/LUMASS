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

#include "NMWidgetListView.h"
#include "nmlog.h"
#include <QSizePolicy>

const std::string NMWidgetListView::ctx = "NMWidgetListView";

NMWidgetListView::NMWidgetListView(QWidget *parent) :
    QWidget(parent)
{
    QSizePolicy spSA(QSizePolicy::Expanding,
                        QSizePolicy::Expanding);
    spSA.setHorizontalStretch(0);
    spSA.setVerticalStretch(0);
    spSA.setHeightForWidth(false);
    this->setSizePolicy(spSA);

    mWidgetList.clear();
    delete this->layout();

    mVBoxLayout = new QVBoxLayout(this);
    mVBoxLayout->setSpacing(0);
    mVBoxLayout->setContentsMargins(0, 0, 0, 0);
    mVBoxLayout->setDirection(QBoxLayout::TopToBottom);
    mVBoxLayout->setAlignment(Qt::AlignTop);
}

void
NMWidgetListView::addWidgetItem(QWidget* widget, const QString& btnLabel)
{
    NMDebugCtx(ctx, << "...")

    if (widget == 0)
    {
        NMDebugCtx(ctx, << "done!")
        return;
    }
    // prepare the button
    QSizePolicy spB(QSizePolicy::Expanding,
                    QSizePolicy::Fixed);
    spB.setHorizontalStretch(0);
    spB.setVerticalStretch(0);
    spB.setHeightForWidth(false);

    QPushButton* btn = new QPushButton(this);
    btn->setSizePolicy(spB);
    btn->setText(btnLabel);
    btn->setFlat(false);
    btn->setCheckable(true);
    btn->setChecked(true);
    mVBoxLayout->addWidget(btn, 0, Qt::AlignTop);


    // prepare the widget
    QSizePolicy spW(QSizePolicy::Expanding,
                        QSizePolicy::Expanding);
    spW.setHorizontalStretch(0);
    spW.setVerticalStretch(0);
    spW.setHeightForWidth(false);
    widget->setParent(this);
    widget->setVisible(true);
    widget->setMaximumHeight(QWIDGETSIZE_MAX);
    widget->setSizePolicy(spW);

    mVBoxLayout->addWidget(widget);//, 0, Qt::AlignTop);

    // do admin stuff
    mBtnList.append(btn);
    mWidgetList.insert(btn, widget);
    connect(btn, SIGNAL(clicked()), this, SLOT(btnPushed()));

    NMDebugCtx(ctx, << "done!")
}

void
NMWidgetListView::btnPushed(void)
{
    QPushButton* btn = qobject_cast<QPushButton*>(this->sender());
    if (btn == 0)
    {
        return;
    }

    QMap<QPushButton*, QWidget*>::iterator it = mWidgetList.find(btn);
    if (it != mWidgetList.end())
    {
        bool vis = it.value()->isVisible();
        it.value()->setVisible(!vis);

        btn->setChecked(!vis);
    }

    // brute force
    foreach(QPushButton* b, mBtnList)
    {
        mVBoxLayout->removeWidget(b);
        mVBoxLayout->removeWidget(mWidgetList[b]);
    }

    foreach(QPushButton* b, mBtnList)
    {
        mVBoxLayout->addWidget(b, 0, Qt::AlignTop);
        if (mWidgetList[b]->isVisible())
        {
            mVBoxLayout->addWidget(mWidgetList[b]);//, 0, Qt::AlignTop);
        }
    }
}


