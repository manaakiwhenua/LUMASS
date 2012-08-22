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
 * NMChartWidget.h
 *
 *  Created on: 7/10/2011
 *      Author: alex
 */

#ifndef NMCHARTWIDGET_H_
#define NMCHARTWIDGET_H_

#include "nmlog.h"
#define __ctxchartwidget "NMChartWidget"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QAbstractItemModel>

#include "vtkTable.h"
#include "vtkQtBarChart.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkQtBarChartOptions.h"

class NMChartWidget : public QWidget
{
	Q_OBJECT

public:
	NMChartWidget(QWidget* parent=0);
	NMChartWidget(vtkTable* charttab, QWidget* parent=0);
	virtual
	~NMChartWidget();

	void setChartTable(vtkTable* charttab);
	void setChartModel(QAbstractItemModel* chartmodel);
	void setWinTitle(QString title)
		{this->mChartWidget->setWindowTitle(title);};
	void show() {this->mChartWidget->show();};
	void setBarChartOptions(vtkQtBarChartOptions& options);
	vtkQtBarChartOptions *getBarChartOptions(void);


protected:

	void initChart();

	vtkQtChartWidget* mChartWidget;
	vtkQtChartLegend* mChartLegend;
	vtkQtBarChart* mBarChart;
	vtkQtChartLegendManager* mChartLegendManager;
	vtkQtChartSeriesSelectionHandler* mChartSeriesSelHandler;
	vtkQtChartTableSeriesModel* mChartTableModel;
	QPointer<vtkQtTableModelAdapter> mModelAdapter;

};

#endif /* NMCHARTWIDGET_H_ */
