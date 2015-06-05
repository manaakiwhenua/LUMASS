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

        // let's write a report
        QString sRepName = QString("%1/report_%2_p%3-%4.txt").arg(dsInfo.path())
                        .arg(dsInfo.baseName()).arg(level).arg(runs);

        // let's write the actual optimisation problem in lp format
        QString lpName = QString("%1/lp_%2_p%3-%4.lp").arg(dsInfo.path())
                .arg(dsInfo.baseName()).arg(level).arg(runs);

		mosra->setTimeOut(mosra->getTimeOut());
		if (!mosra->solveLp())
        {
            mosra->writeReport(sRepName);
			continue;
        }
        mosra->writeReport(sRepName);
        mosra->getLp()->WriteLp(lpName.toStdString());

		if (!mosra->mapLp())
			continue;

        vtkSmartPointer<vtkTable> chngmatrix;
        vtkSmartPointer<vtkTable> sumres = mosra->sumResults(chngmatrix);

		// now write the input and the result table
		QString perturbName = QString("%1/%2_p%3-%4.csv").arg(dsInfo.path())
				.arg(dsInfo.baseName()).arg(level).arg(runs);

		QString resName = QString("%1/res_%2_p%3-%4.csv").arg(dsInfo.path())
						.arg(dsInfo.baseName()).arg(level).arg(runs);

        QString chngName = QString("%1/chng_%2_p%3-%4.csv").arg(dsInfo.path())
                .arg(dsInfo.baseName()).arg(level).arg(runs);

        QString relName = QString("%1/rel_%2_p%3-%4.csv").arg(dsInfo.path())
                .arg(dsInfo.baseName()).arg(level).arg(runs);

        QString totName = QString("%1/tot_%2_p%3-%4.csv").arg(dsInfo.path())
                .arg(dsInfo.baseName()).arg(level).arg(runs);


		vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
		writer->SetFieldDelimiter(",");

        // ------------------------------------------------------
        // export attribute table
        vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
                    tab->GetColumnByName("nm_hole"));

        // filter 'hole' polygons
        int nrows = tab->GetNumberOfRows();
        int r = 0;
        while(r < nrows)
        {
            if (hole->GetValue(r))
            {
                tab->RemoveRow(r);
                --nrows;
            }
            else
            {
                ++r;
            }
        }

        // get rid of admin fields
        tab->RemoveColumnByName("nm_id");
        tab->RemoveColumnByName("nm_hole");
        tab->RemoveColumnByName("nm_sel");


        writer->SetInputData(tab);
		writer->SetFileName(perturbName.toStdString().c_str());
		writer->Update();

        // ---------------------------------------------------------
        // export summaries and land use change matrix

        writer->SetInputData(sumres);
		writer->SetFileName(resName.toStdString().c_str());
		writer->Update();

        writer->SetInputData(chngmatrix);
        writer->SetFileName(chngName.toStdString().c_str());
        writer->Update();

        //create 'totals only table'
        int ncols = sumres->GetNumberOfColumns();
        int numCri = (ncols-1)/4;

        // create table only containing the absolute performance
        // after this optimisation run
        vtkSmartPointer<vtkTable> totals = vtkSmartPointer<vtkTable>::New();
        totals->AddColumn(sumres->GetColumn(0));
        for (int i=0, off=2; i < numCri; ++i, off += 4)
        {
            totals->AddColumn(sumres->GetColumn(off));
        }

        writer->SetInputData(totals);
        writer->SetFileName(totName.toStdString().c_str());
        writer->Update();

        // create table only containing relative changes (%)
        // from original result table
        for (int i=0; i < numCri; ++i)
        {
            for (int re=0; re < 3; ++re)
            {
                sumres->RemoveColumn(i+1);
            }
        }

        writer->SetInputData(sumres);
        writer->SetFileName(relName.toStdString().c_str());
        writer->Update();


		writer->Delete();
	}
}
