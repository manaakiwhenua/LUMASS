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

#include "NMHoverEditTree.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QMenu>

NMHoverEditTree::NMHoverEditTree(QWidget *parent)
    : QTreeWidget(parent)
{
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setDropIndicatorShown(true);
    this->setDragDropOverwriteMode(false);
    this->setAcceptDrops(true);
    this->setMouseTracking(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
            this, SLOT(updateCurIndices(QTreeWidgetItem*,int)));

    mContextMenu = new QMenu(this);


    mInsParamAction = mContextMenu->addAction("Insert parameter here");
    mContextMenu->addSeparator();
    mAddListAction = mContextMenu->addAction("Add list");
    mInsListAction = mContextMenu->addAction("Insert list here");
    mContextMenu->addSeparator();
    mDelAction = mContextMenu->addAction("Delete");

    connect(mDelAction, &QAction::triggered, this, &NMHoverEditTree::deleteItem);
    connect(mInsParamAction, &QAction::triggered, this, &NMHoverEditTree::insertParameter);
    connect(mInsListAction, &QAction::triggered, this, &NMHoverEditTree::insertList);
    connect(mAddListAction, &QAction::triggered, this, &NMHoverEditTree::addList);
}



void
NMHoverEditTree::deleteItem()
{
    changeStructure(mCurIndices, 0);
    QTreeWidgetItem* pi = mLastPressedItem->parent();
    if (pi)
    {
        mCurIndices = itemIndices(pi);
    }
    else
    {
        mCurIndices.clear();
    }

    delete mLastPressedItem;
    mLastPressedItem = 0;
    updateTree();
}

void
NMHoverEditTree::changeStructure(const QList<int> &indices, int mode)
{
    switch (mMaxLevel)
    {
    case 1:
        if (indices.size() == 1)
        {
            QStringList sl = mModel.toStringList();
            if (mode)
            {
                sl.insert(indices.at(0), QString(""));
            }
            else
            {
                sl.removeAt(indices.at(0));
            }
            mModel = QVariant::fromValue(sl);
        }
        break;

    case 2:
        {
            QList<QStringList> lsl = mModel.value<QList<QStringList> >();
            if (indices.size() == 1)
            {
                if (mode)
                {
                    QStringList nsl;
                    lsl.insert(indices.at(0), nsl);
                }
                else
                {
                    lsl.removeAt(indices.at(0));
                }
            }
            else if (indices.size() == 2)
            {
                QStringList sl = lsl.at(indices.at(0));
                if (mode)
                {
                    sl.insert(indices.at(1), QString(""));
                }
                else
                {
                    sl.removeAt(indices.at(1));
                }
                lsl.replace(indices.at(0), sl);
            }
            mModel = QVariant::fromValue(lsl);
        }
        break;

    case 3:
        {
            QList<QList<QStringList> > llsl = mModel.value<QList<QList<QStringList> > >();
            if (indices.size() == 1)
            {
                if (mode)
                {
                    QList<QStringList> nlsl;
                    llsl.insert(indices.at(0), nlsl);
                }
                else
                {
                    llsl.removeAt(indices.at(0));
                }
            }
            else if (indices.size() == 2)
            {
                QList<QStringList> lsl = llsl.at(indices.at(0));
                if (mode)
                {
                    QStringList nsl;
                    lsl.insert(indices.at(1), nsl);
                }
                else
                {
                    lsl.removeAt(indices.at(1));
                }
                llsl.replace(indices.at(0), lsl);
            }
            else if (indices.size() == 3)
            {
                QList<QStringList> lsl = llsl.at(indices.at(0));
                QStringList sl = lsl.at(indices.at(1));
                if (mode)
                {
                    sl.insert(indices.at(2), QString(""));
                }
                else
                {
                    sl.removeAt(indices.at(2));
                }
                lsl.replace(indices.at(1), sl);
                llsl.replace(indices.at(0), lsl);
            }
            mModel = QVariant::fromValue(llsl);
        }
    }
}

QTreeWidgetItem*
NMHoverEditTree::itemFromIndices(const QList<int> &indices)
{
    QTreeWidgetItem* item = 0;
    if (indices.count() > 0)
    {
        item = this->topLevelItem(indices.at(0));
        for (int i=1; i < indices.size() && item != 0; ++i)
        {
            item = item->child(indices.at(i));
        }
    }
    return item;
}

void
NMHoverEditTree::addList()
{
    QList<int> indices = mCurIndices;
    indices << mLastPressedItem->childCount();
    changeStructure(indices, 1);
    mCurIndices = indices;
    updateTree();
}

void
NMHoverEditTree::insertList()
{
    changeStructure(mCurIndices, 1);
    updateTree();
}

void
NMHoverEditTree::insertParameter()
{
    QList<int> indices = mCurIndices;

    if (indices.size() == mMaxLevel-1)
    {
        indices << mLastPressedItem->childCount();
    }
    changeStructure(indices, 1);
    mCurIndices = indices;

    updateTree();
}

void
NMHoverEditTree::callContextMenu(const QPoint& pos)
{
    QString delType = mCurIndices.size() == mMaxLevel ? "parameter" : "list";

    QTreeWidgetItem* item = itemFromIndices(mCurIndices);
    if (mCurIndices.size() == mMaxLevel)
    {
        mInsParamAction->setEnabled(true);
        mInsParamAction->setText("Insert parameter here");
        mInsListAction->setEnabled(false);
        mAddListAction->setEnabled(false);
    }
    else if (mCurIndices.size() == mMaxLevel-1)
    {

        mInsParamAction->setEnabled(true);
        mInsParamAction->setText(QString("Add parameter to %1").arg(extractString(item, 0)));
        mAddListAction->setEnabled(false);
        mInsListAction->setEnabled(true);
    }
    else
    {
        mInsParamAction->setEnabled(false);
        mAddListAction->setEnabled(true);
        mAddListAction->setText(QString("Add list to %1").arg(extractString(item, 0)));
        mInsListAction->setEnabled(true);
    }

    mDelAction->setText(QString("Delete %1 %2").arg(delType).arg(extractString(mLastPressedItem, 0)));
    mContextMenu->exec(pos);
}

void
NMHoverEditTree::mousePressEvent(QMouseEvent *event)
{
    QTreeWidget::mousePressEvent(event);

    if (event->button() == Qt::RightButton)
    {
        callContextMenu(event->globalPos());
    }
}

void
NMHoverEditTree::dragEnterEvent(QDragEnterEvent* event)
{
    if (    (   event->source() != 0
             && event->source()->objectName().compare("treeWidget") == 0
            )
         && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")
       )
    {
        QTreeWidget::dragEnterEvent(event);
        return;
    }

    if (    event->mimeData()->hasFormat("text/uri-list")
         || event->mimeData()->hasFormat("text/plain")
       )
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void
NMHoverEditTree::setTreeModel(const QVariant &model)
{
    mLastPressedItem = 0;
    mModel = model;
    if (!mModel.isValid())
    {
        mCurIndices.clear();
        this->clear();
        emit maxTreeLevel(0);
    }
    updateTree();
}

void
NMHoverEditTree::updateTree()
{
    this->clear();
    this->setColumnCount(1);
    this->setHeaderLabels(QStringList("#Iteration"));

    QString editString;
    QTreeWidgetItem* initItem = 0;
    if (QString("QString").compare(mModel.typeName()) == 0)
    {
        editString = mModel.value<QString>();
        this->setVisible(false);
        mMaxLevel = 0;
    }
    else if (QString("QStringList").compare(mModel.typeName()) == 0)
    {
        QStringList sL = mModel.value<QStringList>();
        QList<QTreeWidgetItem*> items;
        for (int i=0; i < sL.size(); ++i)
        {
            items << (new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#%1: %2").arg(i+1).arg(sL.at(i)))));
            if (i == 0)
            {
                editString = sL.at(i);
                initItem = items.last();
            }
        }
        this->insertTopLevelItems(0, items);
        mMaxLevel = 1;
    }
    else if (QString("QList<QStringList>").compare(mModel.typeName()) == 0)
    {
        QList<QStringList> lsl = mModel.value<QList<QStringList> >();
        QList<QTreeWidgetItem*> stepItems;
        for (int t=0; t < lsl.size(); ++t)
        {
            QStringList stepItemText;
            stepItemText << QString("#%1").arg(t+1);
            QTreeWidgetItem* stepItem = new QTreeWidgetItem((QTreeWidget*)0, stepItemText);
            QStringList sl = lsl.at(t);
            for (int s=0; s < sl.size(); ++s)
            {
                QStringList level1Text;
                level1Text << QString("%1.%2: %3").arg(t+1).arg(s+1).arg(sl.at(s));
                stepItem->addChild(new QTreeWidgetItem((QTreeWidget*)0, level1Text));
                if (t==0 && s==0)
                {
                    editString = sl.at(s);
                    initItem = stepItem->child(stepItem->childCount()-1);
                }
            }
            stepItems << stepItem;
        }
        this->insertTopLevelItems(0, stepItems);
        mMaxLevel = 2;
    }
    else if (QString("QList<QList<QStringList> >").compare(mModel.typeName()) == 0)
    {
        QList<QList<QStringList> > llsl = mModel.value<QList<QList<QStringList> > >();
        QList<QTreeWidgetItem*> stepItems;
        for (int t=0; t < llsl.size(); ++t)
        {
            QTreeWidgetItem* stepItem = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#%1").arg(t+1)));
            QList<QStringList> lsl = llsl.at(t);
            for (int ls=0; ls < lsl.size(); ++ls)
            {
                QStringList level1Text;
                level1Text << QString("List %1.%2").arg(t+1).arg(ls+1);
                QTreeWidgetItem* level1Item = new QTreeWidgetItem((QTreeWidget*)0, level1Text);
                QStringList sl = lsl.at(ls);
                for (int s=0; s < sl.size(); ++s)
                {
                    QStringList level2Text;
                    level2Text << QString("%1.%2.%3: %4").arg(t+1).arg(ls+1).arg(s+1).arg(sl.at(s));
                    level1Item->addChild(new QTreeWidgetItem((QTreeWidget*)0, level2Text));
                    if (t==0 && ls==0 && s==0)
                    {
                        editString = sl.at(s);
                        initItem = level1Item->child(level1Item->childCount()-1);
                    }
                }
                stepItem->addChild(level1Item);
            }
            stepItems << stepItem;
        }
        this->insertTopLevelItems(0, stepItems);
        mMaxLevel = 3;
    }
    emit maxTreeLevel(mMaxLevel);

    if (mCurIndices.size() > 0)
    {
        initItem = itemFromIndices(mCurIndices);
    }
    showoffItem(initItem);
}

void
NMHoverEditTree::updateModel()
{
    mModel.clear();
    if (topLevelItemCount() == 0)
    {
        return;
    }

    // establish the model 'type' by checking the
    // maximum depth of the tree; note: check for
    // each toplevel item to account for possibly
    // empty (sub-)branches
    int maxlevel = 0;
    for (int v=0; v < topLevelItemCount(); ++v)
    {
        QTreeWidgetItem* ci = topLevelItem(v);
        int cnt=0;
        while (ci->childCount() > 0)
        {
            ci = ci->child(0);
            ++cnt;
            if (cnt > maxlevel)
            {
                maxlevel = cnt;
            }
        }
    }

    if (maxlevel == 0)
    {
        QStringList sl;
        for (int i=0; i < topLevelItemCount(); ++i)
        {
            sl << extractString(topLevelItem(i));
        }
        mModel = QVariant::fromValue(sl);
    }
    else if (maxlevel == 1)
    {
        QList<QStringList> lsl;
        for (int i=0; i < topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* ti = topLevelItem(i);
            QStringList sl;
            for (int c=0; c < ti->childCount(); ++c)
            {
                sl << extractString(ti->child(c));
            }
            lsl << sl;
        }
        mModel = QVariant::fromValue(lsl);
    }
    else if (maxlevel == 2)
    {
        QList<QList<QStringList> > llsl;
        for (int i=0; i < topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* topItem = topLevelItem(i);
            QList<QStringList> lsl;
            for (int c=0; c < topItem->childCount(); ++c)
            {
                QTreeWidgetItem* level1Item = topItem->child(c);
                QStringList sl;
                for (int k=0; k < level1Item->childCount(); ++k)
                {
                    sl << extractString(level1Item->child(k));
                }
                lsl << sl;
            }
            llsl << lsl;
        }
        mModel = QVariant::fromValue(llsl);
    }
}

QString
NMHoverEditTree::extractString(QTreeWidgetItem *item, int pos)
{
    if (item == 0)
    {
        return QString();
    }

    if (pos < 0 || pos > 1)
    {
        pos = 1;
    }

    QString s = item->text(0);
    QStringList sl = s.split(':', QString::SkipEmptyParts);
    if (sl.size() >= 2)
    {
        s = sl.at(pos).simplified();
    }
    else
    {
        s = s.simplified();
    }
    return s;
}

QList<int>
NMHoverEditTree::itemIndices(QTreeWidgetItem *item)
{
    QList<int> indices;
    QTreeWidgetItem* ci = item;
    if (ci)
    {
        indices.clear();
        QTreeWidgetItem* pi = ci->parent();
        while (pi != 0)
        {
            indices.prepend(pi->indexOfChild(ci));
            ci = pi;
            pi = ci->parent();
        }
        indices.prepend(indexOfTopLevelItem(ci));
    }
    return indices;
}


void
NMHoverEditTree::updateCurIndices(QTreeWidgetItem* item, int col)
{
    mCurIndices = itemIndices(item);
    mLastPressedItem = item;
}

void NMHoverEditTree::dragMoveEvent(QDragMoveEvent* event)
{
    // call base class implementation to get the drop indicator painted
    QPoint movePos = event->pos();
    QTreeWidget::dragMoveEvent(event);

    // ====================================================================
    //          INTERNAL DROPS (MOVE & COPY)
    // ====================================================================
    if (    (   event->source() != 0
             && event->source()->objectName().compare("treeWidget") == 0
            )
         && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")
       )
    {
        // indicate whether the current position is a supported drop position
        QTreeWidgetItem* passOverItem = this->itemAt(event->pos());
        int lastLevel = itemLevel(mLastPressedItem);
        if (passOverItem)
        {
            int passLevel = itemLevel(passOverItem);
            if (    (    passLevel == lastLevel
                      || lastLevel-1 == passLevel
                    )
                 && mLastPressedItem->text(0).compare(passOverItem->text(0)) != 0
               )
            {
                event->acceptProposedAction();
            }
            else
            {
                event->ignore();
            }
        }
        else if (lastLevel == 0)
        {
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
        return;
    }

    if (    event->mimeData()->hasFormat("text/uri-list")
         || event->mimeData()->hasFormat("text/plain")
       )
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

int
NMHoverEditTree::itemLevel(const QTreeWidgetItem *item)
{
    if (item == 0)
    {
        return 0;
    }

    QTreeWidgetItem* pi = item->parent();
    int level = 0;
    while (pi != 0)
    {
        pi = pi->parent();\
        ++level;
    }
    return level;
}


void NMHoverEditTree::dropEvent(QDropEvent* event)
{
    QPoint dropPos = event->pos();
    QTreeWidgetItem* dropItem = this->itemAt(event->pos());
    Qt::DropAction dropAction = event->proposedAction();

    // ====================================================================
    //          INTERNAL DROPS (MOVE & COPY)
    // ====================================================================
    if (    (   event->source() != 0
             && event->source()->objectName().compare("treeWidget") == 0
            )
         && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")
       )
    {
        // determine drop item and its associated tree index
        if (dropItem == 0)
        {
            dropItem = topLevelItem(topLevelItemCount()-1);
        }
        QList<int> dropIndices = itemIndices(dropItem);

        // are we dropping below the last item in the tree?
        QRect dropRect = visualItemRect(dropItem);
        if (dropPos.y() >= dropRect.bottom()-2)
        {
            dropIndices.last()++;
        }

        // determine the new item's parent
        QTreeWidgetItem* insertParent = 0;
        int dropDepth = dropIndices.size() == mCurIndices.size()-1 ? dropIndices.size() : dropIndices.size()-1;
        for (int l=0; l < dropDepth; ++l)
        {
            if (l == 0)
            {
                insertParent = topLevelItem(dropIndices.at(l));
            }
            else
            {
                insertParent = insertParent->child(dropIndices.at(l));
            }
        }

        // adjust dropIndices, if we're dropping at the item's parent's level
        if (dropIndices.size() < mCurIndices.size())
        {
            dropIndices << (insertParent ? insertParent->childCount() : topLevelItemCount());
        }

        // ... finally insert the dropped item at the identified position
        if (insertParent)
        {
            insertParent->insertChild(dropIndices.at(dropIndices.size()-1), mLastPressedItem->clone());
        }
        else
        {
            insertTopLevelItem(dropIndices.at(dropIndices.size()-1), mLastPressedItem->clone());
        }

        // delete the drag item if this is a move operation
        if (dropAction == Qt::MoveAction)
        {
            // adjust the indices of the newly inserted item
            // accounting for the removal of the drag item at its old position
            int cnt=0;
            while (mCurIndices.at(cnt) == dropIndices.at(cnt))
            {
                ++cnt;
            }
            if (cnt == dropIndices.size()-1)
            {
                if (mCurIndices.at(cnt) < dropIndices.at(cnt))
                {
                    dropIndices[cnt]--;
                }
            }

            delete mLastPressedItem;
        }

        // update everything ...
        // ... new model from current tree item texts (after preamble)
        updateModel();
        // ... update the tree model to regenerate the preamble text
        //     after creating a new order
        updateTree();

        // ... the new indices of the current item, for easy access
        //     of the current model item
        QTreeWidgetItem* curItem = 0;
        for (int l=0; l < dropIndices.size(); ++l)
        {
            if (l == 0)
            {
                curItem = topLevelItem(dropIndices.at(l));
            }
            else
            {
                curItem = curItem->child(dropIndices.at(l));
            }
        }

        // update the current item and make sure
        // the dropped item is visible
        showoffItem(curItem);
        return;
    }

    // ====================================================================
    //          EXTERNAL DROPS (INSERT)
    // ====================================================================
    if (    event->mimeData()->hasFormat("text/uri-list")
         || event->mimeData()->hasFormat("text/plain")
       )
    {
        const QMimeData* mdata = event->mimeData();
        QStringList dformats = mdata->formats();
        qDebug() << "formats: " << dformats;
        qDebug() << "mdata.text: " << mdata->text();
    }
}


void
NMHoverEditTree::showoffItem(QTreeWidgetItem *item)
{
    if (item == 0)
    {
        return;
    }

    QTreeWidgetItem* pi = item->parent();
    while (pi != 0)
    {
        expandItem(pi);
        pi = pi->parent();
    }
    expandItem(item);
    scrollToItem(item);

    setCurrentItem(item);
    updateCurIndices(item, 0);

    emit itemClicked(item, 0);
}

void NMHoverEditTree::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->ignore();
}

