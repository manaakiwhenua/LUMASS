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
 * NMChartWidget.cpp
 *
 *  Created on: 7/10/2011
 *      Author: alex
 */

#include "NMChartWidget.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartLayer.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisOptions.h"


NMChartWidget::NMChartWidget(QWidget* parent)
	: QWidget(parent)
{
	this->initChart();
}

NMChartWidget::NMChartWidget(vtkTable* charttab, QWidget* parent)
	: QWidget(parent)
{
	this->initChart();
	this->setChartTable(charttab);
}

NMChartWidget::~NMChartWidget()
{
//	NMDebugCtx(__ctxchartwidget, << "...");

	delete this->mChartTableModel;
	if (this->mModelAdapter != 0)
		delete this->mModelAdapter;
	delete this->mChartSeriesSelHandler;
	delete this->mChartLegendManager;
	delete this->mChartLegend;
	delete this->mBarChart;
	delete this->mChartWidget;

//	NMDebugCtx(__ctxchartwidget, << "done!");
}

void NMChartWidget::initChart()
{
	NMDebugCtx(__ctxchartwidget, << "...");

	this->mChartWidget = new vtkQtChartWidget();
	vtkQtChartArea* area = this->mChartWidget->getChartArea();
	vtkQtChartBasicStyleManager* style =
			qobject_cast<vtkQtChartBasicStyleManager*>(area->getStyleManager());
	if (style)
		style->getColors()->setColorScheme(vtkQtChartColors::Spectrum);

	this->mBarChart = new vtkQtBarChart();
	area->insertLayer(area->getAxisLayerIndex(), this->mBarChart);

	this->mChartLegend = new vtkQtChartLegend();
	this->mChartLegendManager = new vtkQtChartLegendManager(this->mChartLegend);
	this->mChartLegendManager->setChartLegend(this->mChartLegend);
	this->mChartLegendManager->setChartArea(area);
	this->mChartWidget->setLegend(this->mChartLegend);

	vtkQtChartMouseSelection* selector =
			vtkQtChartInteractorSetup::createDefault(area);
	this->mChartSeriesSelHandler = new vtkQtChartSeriesSelectionHandler(selector);
	this->mChartSeriesSelHandler->setModeNames("Bar Chart - Series", "Bar Chart - Bars");
	this->mChartSeriesSelHandler->setMousePressModifiers(Qt::ControlModifier,
			Qt::ControlModifier);
	this->mChartSeriesSelHandler->setLayer(this->mBarChart);
	selector->addHandler(this->mChartSeriesSelHandler);
	selector->setSelectionMode("Bar Chart - Bars");
	vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

	vtkQtChartAxisLayer* axisLayer = area->getAxisLayer();
	vtkQtChartAxis* xAxis = axisLayer->getAxis(vtkQtChartAxis::Bottom);
	xAxis->getOptions()->setGridVisible(false);


	this->mChartTableModel = new vtkQtChartTableSeriesModel(0, this->mBarChart);

	NMDebugCtx(__ctxchartwidget, << "done!");
}

void NMChartWidget::setChartTable(vtkTable* charttab)
{
	if (charttab == 0)
		return;

	if (this->mModelAdapter != 0)
		delete this->mModelAdapter;

	this->mModelAdapter = new vtkQtTableModelAdapter();
	this->mModelAdapter->setTable(charttab);
	this->mChartTableModel->setItemModel(this->mModelAdapter);
	this->mBarChart->setModel(this->mChartTableModel);
}

void NMChartWidget::setChartModel(QAbstractItemModel* chartmodel)
{
	NMDebugCtx(__ctxchartwidget, << "...");

	if (chartmodel == 0)
		return;

//	if (this->mModelAdapter != 0)
//		delete this->mModelAdapter;

//NMDebugAI(<< "setting the chartmodel");
	this->mChartTableModel->setItemModel(chartmodel);
	this->mBarChart->setModel(this->mChartTableModel);

	NMDebugCtx(__ctxchartwidget, << "done!");
}

vtkQtBarChartOptions *NMChartWidget::getBarChartOptions()
{
	return this->mBarChart->getOptions();
}

void NMChartWidget::setBarChartOptions(vtkQtBarChartOptions& options)
{
	this->mBarChart->setOptions(options);

}

