/*
 * MOSORunnable.cpp
 *
 *  Created on: 22/06/2013
 *      Author: alex
 */

#include <QScopedPointer>
#include <QFileInfo>

#include "nmlog.h"
#include "NMMosra.h"
#include "MOSORunnable.h"

#include "vtkSmartPointer.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkDelimitedTextWriter.h"


MOSORunnable::MOSORunnable()
{
	// TODO Auto-generated constructor stub

}

MOSORunnable::~MOSORunnable()
{
	// TODO Auto-generated destructor stub
}

void
MOSORunnable::setData(QString dsfileName,
			QString losSettingsFileName,
			QString perturbItem,
			float level,
			int startIdx, int numruns)
{
	mDsFileName = dsfileName;
	mLosFileName = losSettingsFileName;
	mPerturbItem = perturbItem;
	mLevel = level;
	mStartIdx = startIdx;
	mNumRuns = numruns;
}

void
MOSORunnable::run()
{
	QScopedPointer<NMMosra> mosra(new NMMosra());

	int startIdx = mStartIdx;
	int numruns = mNumRuns;
	float level = mLevel;
	QString dsfileName = mDsFileName;
	QString losSettingsFileName = mLosFileName;
	QString perturbItem = mPerturbItem;

	NMMsg(<< "Starting run=" << startIdx);

	QFileInfo dsInfo(dsfileName);

	for (int runs=startIdx; runs <= numruns; ++runs)
	{
		NMDebugAI(<< endl << "******** PERTURBATION #" << runs << " *************" << endl);
		// load the file with optimisation settings
		mosra->loadSettings(losSettingsFileName);

		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(dsfileName.toStdString().c_str());

		reader->Update();

		vtkPolyData* pd = reader->GetOutput();
		mosra->setDataSet(pd);
		mosra->perturbCriterion(perturbItem, level);
		vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();

		mosra->setTimeOut(mosra->getTimeOut());
		if (!mosra->solveLp())
			continue;

		if (!mosra->mapLp())
			continue;

		vtkSmartPointer<vtkTable> sumres = mosra->sumResults();

		// get rid of admin fields
		tab->RemoveColumnByName("nm_id");
		tab->RemoveColumnByName("nm_hole");
		tab->RemoveColumnByName("nm_sel");

		// let's write a report
		QString sRepName = QString("%1/report_%2_p%3-%4.txt").arg(dsInfo.path())
						.arg(dsInfo.baseName()).arg(level).arg(runs);
		mosra->writeReport(sRepName);

		// now write the input and the result table
		QString perturbName = QString("%1/%2_p%3-%4.csv").arg(dsInfo.path())
				.arg(dsInfo.baseName()).arg(level).arg(runs);

		QString resName = QString("%1/res_%2_p%3-%4.csv").arg(dsInfo.path())
						.arg(dsInfo.baseName()).arg(level).arg(runs);

		vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
		writer->SetFieldDelimiter(",");

        writer->SetInputData(tab);
		writer->SetFileName(perturbName.toStdString().c_str());
		writer->Update();

        writer->SetInputData(sumres);
		writer->SetFileName(resName.toStdString().c_str());
		writer->Update();

		writer->Delete();
	}
}
