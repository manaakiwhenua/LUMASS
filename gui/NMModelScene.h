 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
/*
 * NMModelScene.h
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#ifndef NMMODELSCENE_H_
#define NMMODELSCENE_H_

#include "nmlog.h"
#include <string>
#include <iostream>
#include <qobject.h>
#include <qgraphicsscene.h>
#include <QGraphicsLineItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QRubberBand>

#include "NMProcessComponentItem.h"
#include "NMAggregateComponentItem.h"
#include "NMComponentLinkItem.h"

/*
 * NMAggregateComponentItem = UserType + 10
 * NMProcessComponentItem = UserType + 20
 * NMComponentLinkItem = UserType + 30
 *
 *
 */

class NMComponentLinkItem;
class NMProcessComponentItem;
class NMAggregateComponentItem;
class NMSqlTableView;
class NMLogger;

class NMModelScene: public QGraphicsScene
{
	Q_OBJECT

public:

    enum InteractionMode
    {
        NMS_IDLE,
        NMS_MOVE,
        NMS_LINK,
        NMS_SELECT,
        NMS_ZOOM_IN,
        NMS_ZOOM_OUT,
        NMS_UNDEFINED
    };

	NMModelScene(QObject* parent=0);
	virtual ~NMModelScene();

    void setLogger(NMLogger* logger){mLogger = logger;}

    QGraphicsProxyWidget* getWidgetAt(const QPointF& pos);
	QGraphicsItem* getComponentItem(const QString& name);
	NMComponentLinkItem* getLinkItem(QPointF pos);

	void serialiseItems(QList<QGraphicsItem*> items,
			QDataStream& data);
	qreal getLinkZLevel(void)
		{return this->mLinkZLevel;}

    QPointF getMousePos(void)
        {return mMousePos;}

    void updateComponentItemFlags(QGraphicsItem* item);

    void addParameterTable(NMSqlTableView* tv,
                           NMAggregateComponentItem* ai,
                           NMModelComponent* host);

     InteractionMode getInteractionMode(void){return mMode;}

public slots:
	void toggleLinkToolButton(bool);
	void toggleSelToolButton(bool);
	void toggleMoveToolButton(bool);
    void toggleZoomInTool(bool zin);
    void toggleZoomOutTool(bool zout);
    void idleModeOn(void);

    void clearDragItems(void)
        {this->mDragItemList.clear();}

    void unselectItems(void);

signals:
	void linkItemCreated(NMComponentLinkItem*);
	void processItemCreated(NMProcessComponentItem*,
			const QString&, QPointF scenePos);
	void rootComponentDblClicked();
	void procAggregateCompDblClicked(const QString&);	// name
	void itemRightBtnClicked(QGraphicsSceneMouseEvent *,
			QGraphicsItem *);
    void widgetTitleBarRightClicked(QGraphicsSceneMouseEvent *,
                                    QGraphicsItem *);
    void widgetViewPortRightClicked(QGraphicsSceneMouseEvent *,
                                    QGraphicsItem *);
    void itemDblClicked(QGraphicsSceneMouseEvent *);
    void itemLeftClicked(const QString& itemName);
    void zoom(int delta);
    void signalModelFileDropped(const QString& fileName,
                                const QPointF& scenePos);
    void signalItemCopy(const QList<QGraphicsItem*>& copyList,
                        const QPointF& sourcePos, const QPointF& targetPos);
    void signalItemMove(const QList<QGraphicsItem*>& moveList,
                        const QPointF& sourcePos, const QPointF& targetPos);

protected:
	void dragEnterEvent(QGraphicsSceneDragDropEvent* event);
	void dragMoveEvent(QGraphicsSceneDragDropEvent* event);
	void dropEvent(QGraphicsSceneDragDropEvent* event);
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);


	void setProcCompSelectability(bool selectable);
	void setProcCompMoveability(bool moveable);
	void setLinkCompSelectability(bool selectable);
    void updateCursor();

	void wheelEvent(QGraphicsSceneWheelEvent* event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:

	static const std::string ctx;

	qreal mLinkZLevel;
	qreal mLinkHitTolerance;

    bool mbIdleMove;

	QGraphicsLineItem* mLinkLine;
	InteractionMode mMode;

    QGraphicsRectItem* mRubberBand;
    QPointF mRubberBandOrigin;

    QList<QGraphicsItem*> mTempSelection;
    QList<QGraphicsItem*> mToggleSelection;
    QList<QGraphicsItem*> mDragItemList;
    QMap<QString, QGraphicsItem*> mHiddenModelItems;
    QPointF mDragStartPos;
    QPointF mMousePos;

    NMLogger* mLogger;
};

#endif /* NMMODELSCENE_H_ */
