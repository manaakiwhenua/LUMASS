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
 * NMModelViewWidget.h
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#ifndef NMMODELVIEWWIDGET_H_
#define NMMODELVIEWWIDGET_H_

#include "nmlog.h"
#include <string>
#include <iostream>

#include <qwidget.h>
#include <QThread>
#include <QMenu>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QGraphicsView>
#include <QRectF>

#include "NMProcessComponentItem.h"
#include "NMComponentLinkItem.h"
#include "NMAggregateComponentItem.h"
#include "NMModelComponent.h"
#include "NMIterableComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMModelScene.h"
#include "NMModelController.h"
#include "NMEditModelComponentDialog.h"
#include "NMComponentEditor.h"

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

class QDomDocument;
class OtbModellerWin;
class NMComponentLinkItem;

class NMModelViewWidget: public QWidget
{
	Q_OBJECT

public:
	NMModelViewWidget(QWidget* parent=0, Qt::WindowFlags f=0);
	virtual ~NMModelViewWidget();

    OtbModellerWin* getMainWindow(void);
    void addWidget(QWidget* w);

public slots:

	void callEditComponentDialog(const QString &);
	void linkProcessComponents(NMComponentLinkItem* link);
	void createProcessComponent(NMProcessComponentItem* procItem,
			const QString& procName, QPointF scenePos);
	void createAggregateComponent(const QString& compType);
	void createSequentialIterComponent();
	void createConditionalIterComponent();
	void ungroupComponents();
    void setGroupTimeLevel();
    void addDeltaTimeLevel();


	void editRootComponent();
    //void compProcChanged();

	void executeModel(void);
	void resetModel(void);
    void zoomIn() {zoom(1);}
    void zoomOut() {zoom(-1);}
    void zoom(int delta);
    void zoomToContent(void);
    void updateTreeEditor(const QString&);
    void changeFont(void);
    void changeColour(void);
    void importModel(QDataStream& lmv, const QMap<QString, QString>& nameRegister,
                     bool move);
    void copyComponents(const QList<QGraphicsItem*>& copyList,
                        const QPointF & source, const QPointF & target);
    void moveComponents(const QList<QGraphicsItem*>& moveList,
                        const QPointF & source, const QPointF & target);

	void callItemContextMenu(QGraphicsSceneMouseEvent* event,
			QGraphicsItem* item);

	/*! Reflects changes of input components for the first
	 *  iteration in the model view (i.e. deletes, draws
	 *  links between components)*/
	void processProcInputChanged(QList<QStringList> inputs);



signals:
	void linkToolToggled(bool);
	void selToolToggled(bool);
	void moveToolToggled(bool);
	void requestModelExecution(const QString& compName);
	void requestModelReset(const QString& compName);
	void requestModelAbortion(void);
	void widgetIsExiting(void);


protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dropEvent(QDropEvent* event);

protected slots:
	void removeObjFromOpenEditsList(QObject* obj);
	void deleteItem();
	void deleteLinkComponentItem(NMComponentLinkItem* linkItem);
	void deleteProcessComponentItem(NMProcessComponentItem* procItem);
	void deleteAggregateComponentItem(NMAggregateComponentItem* aggrItem);
	void deleteEmptyComponent(NMModelComponent* comp);
	int shareLevel(QList<QGraphicsItem*> list);
	NMModelComponent* componentFromItem(QGraphicsItem* item);
	QString getComponentItemTitle(QGraphicsItem* item);
	void saveItems();
    void loadItems(const QString& fileName);
	void reportIsModelControllerBusy(bool);
    void callLoadItems()
        {loadItems(QString());}

    void exportModel(const QList<QGraphicsItem*>& items,
                     QIODevice& device,
                     QDomDocument& doc,
                     bool bSaveRoot);


	void getSubComps(NMModelComponent* comp, QStringList& subs);
	void connectProcessItem(NMProcess* proc, NMProcessComponentItem* procItem);
    bool eventFilter(QObject* obj, QEvent* e);

private:
	void initItemContextMenu();
    // identifies members of any one list, which are not present
    // in all of the other lists (i.e. inputs, which change over
    // iteration steps)
    QStringList dynamicInputs(QList<QStringList>& inputs);


    QRectF unionRects(const QRectF& r1, const QRectF& r2)
    {
        return QRectF(
           QPointF(
              r1.topLeft().x() < r2.topLeft().x() ? r1.topLeft().x() : r2.topLeft().x(),
              r1.topLeft().y() < r2.topLeft().y() ? r1.topLeft().y() : r2.topLeft().y()
                   ),
           QPointF(
              r1.bottomRight().x() > r2.bottomRight().x() ? r1.bottomRight().x() : r2.bottomRight().x(),
              r1.bottomRight().y() > r2.bottomRight().y() ? r1.bottomRight().y() : r2.bottomRight().y()
            )
        );
    }

    // debug func ------------------
    std::string reportRect(const QRectF& rect, const char* msg);
    std::string reportPoint(const QPointF& pt, const char* msg);
    std::string reportLine(const QLineF& line, const char* msg);
    // ----------------------

    qreal mScaleFactor;

	bool mbControllerIsBusy;

	QGraphicsView* mModelView;
	NMModelScene* mModelScene;
	QMenu* mItemContextMenu;

	QThread* mModelRunThread;

	QPointF mLastScenePos;
    QPointF mSceneDropPos;
	QGraphicsItem* mLastItem;
	NMModelController* mModelController;

#ifdef BUILD_RASSUPPORT	
	NMRasdamanConnectorWrapper* mRasConn;
	RasdamanConnector* mPureRasConn;
#endif
	
	NMIterableComponent* mRootComponent;

    QMap<NMModelComponent*,
        NMEditModelComponentDialog*> mOpenEditors;

    NMComponentEditor* mTreeCompEditor;

	QMap<QString, QAction*> mActionMap;

    static const std::string ctx;

};

#endif /* NMMODELVIEWWIDGET_H_ */
