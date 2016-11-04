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
    this->setLayout(mVBoxLayout);
}


void
NMWidgetListView::addWidgetItem(QWidget* widget, const QString& btnLabel)
{
    //NMDebugCtx(ctx, << "...")

    if (widget == 0)
    {
        //NMDebugCtx(ctx, << "done!")
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
    btn->setVisible(true);
    mVBoxLayout->addWidget(btn);//, 0, Qt::AlignTop);


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
    connect(btn, SIGNAL(clicked(bool)), this, SLOT(btnPushed(bool)));
    mBtnList.append(btn);
    mWidgetList.insert(btn, widget);

//    NMDebugAI(<< "added: " << mWidgetList[btn]->objectName().toStdString()
//              << " isVisible: " << mWidgetList[btn]->isVisible() << std::endl);

    //NMDebugCtx(ctx, << "done!")
}

void
NMWidgetListView::btnPushed(bool checked)
{
    QPushButton* btn = qobject_cast<QPushButton*>(this->sender());
    if (btn == 0)
    {
        return;
    }

    QMap<QPushButton*, QWidget*>::iterator it = mWidgetList.find(btn);
    if (it != mWidgetList.end())
    {
        it.value()->setVisible(checked);
        btn->setChecked(checked);
    }
    this->updateWidgets();
}

void
NMWidgetListView::setWidgetItemVisible(int index, bool visible)
{
    //NMDebugCtx(ctx, << "...");

    if (index < 0 || index >= mBtnList.size())
    {
        NMErr(ctx, << "Index out of range!")
        return;
    }

    QPushButton* btn = mBtnList.at(index);
    if (btn == 0)
    {
        return;
    }
    btn->setChecked(visible);
    mWidgetList.value(btn)->setVisible(visible);

    //NMDebugAI(<< "what are the widgets doing?" << std::endl);

    //    foreach(QPushButton* b, mWidgetList.keys())
    //    {
    //        NMDebugAI(<< "   widget: " << mWidgetList[b]->objectName().toStdString()
    //                  << " | visibility: " << mWidgetList[b]->isVisible() << std::endl);
    //    }



    this->updateWidgets();

   // NMDebugCtx(ctx, << "done!");
}

void
NMWidgetListView::setWidgetItemVisible(const QString& name, bool visible)
{
    QWidget* w = this->getWidgetItem(name);
    if (w == 0)
        return;

    w->setVisible(visible);

    QPushButton* btn = mWidgetList.key(w);
    if (btn == 0)
    {
        return;
    }
    btn->setChecked(visible);

    this->updateWidgets();
}



void NMWidgetListView::updateWidgets(void)
{
    //NMDebugCtx(ctx, << "...");

    // brute force
    foreach(QPushButton* b, mBtnList)
    {
        mVBoxLayout->removeWidget(b);
        mVBoxLayout->removeWidget(mWidgetList[b]);
    }

    foreach(QPushButton* b, mBtnList)
    {
        mVBoxLayout->addWidget(b);//, 0, Qt::AlignTop);
        if (mWidgetList[b]->isVisible())
        {
            //NMDebugAI(<< "added: " << mWidgetList[b]->objectName().toStdString() << std::endl);
            b->setChecked(true);
            mVBoxLayout->addWidget(mWidgetList[b]);//, 0, Qt::AlignTop);
        }
    }

    //NMDebugCtx(ctx, << "done!");
}

void
NMWidgetListView::removeWidgetItem(int index)
{
    if (index < 0 || index >= mBtnList.size())
    {
        NMWarn(ctx, << "Couldn't find Widget at index " << index);
        return;
    }

    QPushButton* btn = mBtnList.takeAt(index);
    QWidget* w = mWidgetList.value(btn);
    mWidgetList.remove(btn);

    mVBoxLayout->removeWidget(w);
    mVBoxLayout->removeWidget(btn);

    delete btn;
    delete w;
}

void
NMWidgetListView::removeWidgetItem(const QString& name)
{
    QWidget* w = this->getWidgetItem(name);
    if (w == 0)
        return;

    QPushButton* btn = mWidgetList.key(w);
    int idx = 0;
    foreach(QPushButton* b, mBtnList)
    {
        if (b == btn)
            break;
        ++idx;
    }

    mBtnList.removeAt(idx);
    mWidgetList.remove(btn);

    mVBoxLayout->removeWidget(w);
    mVBoxLayout->removeWidget(btn);

    delete btn;
    delete w;
}

QWidget*
NMWidgetListView::getWidgetItem(int index)
{
    QWidget* ret = 0;

    if (index < 0 || index >= mBtnList.size())
    {
        NMWarn(ctx, << "Couldn't find a widget at index " << index);
        return ret;
    }

    return mWidgetList.value(mBtnList.at(index));
}

QWidget*
NMWidgetListView::getWidgetItem(const QString& name)
{
    QWidget* ret = 0;

    QMap<QPushButton*, QWidget*>::const_iterator it = mWidgetList.cbegin();
    while (it != mWidgetList.cend())
    {
        if (name.compare(it.value()->objectName()) == 0)
        {
            ret = it.value();
            break;
        }
        ++it;
    }

    return ret;
}












