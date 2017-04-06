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

#ifndef NMHOVEREDITTREE_H
#define NMHOVEREDITTREE_H

#include <QTreeWidget>

class NMHoverEditTree : public QTreeWidget
{
    Q_OBJECT
public:
    NMHoverEditTree(QWidget* parent=0);

    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dragLeaveEvent(QDragLeaveEvent* event);
    void dropEvent(QDropEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void setTreeModel(const QVariant& model);
    QVariant getTreeModel(void){return mModel;}

    void clearCurrentIndices(void){mCurIndices.clear();}
    QList<int> getCurrentIndices(void) {return mCurIndices;}


signals:
    void maxTreeLevel(int);

protected:
    /*! Level on which \a item sits on in the parent-child tree;
     *  top level items sit on level 0 (zero); their children
     *  on level 1 etc.;
     *  The function returns -1 if item is NULL.
    */
    int itemLevel(const QTreeWidgetItem* item);
    QList<int> itemIndices(QTreeWidgetItem* item);
    QTreeWidgetItem* itemFromIndices(const QList<int>& indices);
    QString extractString(QTreeWidgetItem* item, int pos=1);

    /*!
     * \brief inserts or deletes a model item; note: this
     *        function does not reflect changes to the tree
     * \param indices tree index where change occurs
     * \param mode 0=remove; 1=add
     * \param for mode == 1, the respective element, set up
     *        according to the mMaxLevel of the model and
     *        the given indices, is inserted into the model
     */
    void changeStructure(const QList<int>& indices, int mode,
                         QVariant=QVariant());

    void callContextMenu(const QPoint& pos);
    void showoffItem(QTreeWidgetItem* item);
    void updateTree();
    void updateModel();


protected slots:
    void updateCurIndices(QTreeWidgetItem* item, int col);

    void deleteItem();
    void insertParameter();
    void insertList();
    void addList();


private:
    QVariant mModel;
    int mMaxLevel;
    QList<int> mCurIndices;
    QTreeWidgetItem* mLastPressedItem;
    QMenu* mContextMenu;
    QAction* mDelAction;
    QAction* mInsParamAction;
    QAction* mInsListAction;
    QAction* mAddListAction;
};

#endif // NMHOVEREDITTREE_H
