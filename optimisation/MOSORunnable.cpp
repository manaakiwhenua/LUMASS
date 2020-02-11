/*
 * MOSORunnable.cpp
 *
 *  Created on: 22/06/2013
 *      Author: alex
 */


#include <QFileInfo>
#include <QDir>
#include <QObject>
#include <QScopedPointer>

#include "nmlog.h"
#include "NMMosra.h"
#include "MOSORunnable.h"

#include "vtkSmartPointer.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "NMvtkDelimitedTextWriter.h"

#include "otbSQLiteTable.h"

//#include "lumassengine.h"

MOSORunnable::MOSORunnable()
	: mLogger(nullptr)
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
            const QList<float> &levels,
            int startIdx, int numruns, 
			QString losSettings)
{
	mDsFileName = dsfileName;
	mLosFileName = losSettingsFileName;
	mLosSettings = losSettings;
	mPerturbItem = perturbItem;
    mflLevels = levels;
	mStartIdx = startIdx;
	mNumRuns = numruns;
}

bool
MOSORunnable::loadDataSet(NMMosra* mosra)
{
    if (mDsFileName.isEmpty())
    {
        return false;
    }

    QFileInfo dsInfo(mDsFileName);
    if (!dsInfo.isReadable())
    {
        return false;
    }

    QStringList dbFormats;
    dbFormats << "ldb" << "db" << "sqlite";

    const QString suffix = dsInfo.suffix();
    if (suffix.compare("vtk") == 0)
    {
        vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
        reader->SetFileName(mDsFileName.toStdString().c_str());

        reader->Update();

        vtkPolyData* pd = reader->GetOutput();
        if (pd == nullptr)
        {
            return false;
        }

        mosra->setDataSet(pd);
    }
    else if (dbFormats.contains(suffix, Qt::CaseInsensitive))
    {
        otb::SQLiteTable::Pointer stab = otb::SQLiteTable::New();
        if (!stab->openAsInMemDb(mDsFileName.toStdString(), ""))
        {
            return false;
        }

        mosra->setDataSet(stab.GetPointer());
    }

    return true;
}

void
MOSORunnable::run()
{
	QScopedPointer<NMMosra> mosra(new NMMosra());
	if (mLogger != nullptr)
	{
		mosra->setLogger(mLogger);
	}

	int startIdx = mStartIdx;
	int numruns = mNumRuns;
    // join items levels by '+'
    QString level;
    for (int l=0; l < mflLevels.size(); ++l)
    {
        if (l == 0)
        {
            level = QString("%1").arg(mflLevels.at(l));
        }
        else
        {
            level = QString("%1+%2").arg(level).arg(mflLevels.at(l));
        }
    }


	QString dsfileName = mDsFileName;
    QFileInfo dsInfo(dsfileName);

    QString losSettingsFileName = mLosFileName;
	QString perturbItem = mPerturbItem;

    NMMsg(<< "Starting run=" << startIdx);


    for (int runs=startIdx; runs <= (numruns+startIdx-1); ++runs)
	{
		NMDebugAI(<< endl << "******** PERTURBATION #" << runs << " *************" << endl);
		
		// check whether we've got string of settings already, 
		// if not, load settings from file
		if (!mLosSettings.isEmpty())
		{
			mosra->setLosSettings(mLosSettings);
			mosra->parseStringSettings(mLosSettings);
		}
		else
		{
			mosra->loadSettings(losSettingsFileName);
		}
        
		if (!loadDataSet(mosra.data()))
        {
            return;
        }

        if (!mPerturbItem.isEmpty())
        {
            // bail out, if perturbation doesn't go to plan!
            if (!mosra->perturbCriterion(perturbItem, mflLevels))
            {
                return;
            }
        }
		vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();

        // --------------------------------------------------------------------------
        // names for output files
        QString sRepName, lpName, perturbName, resName, chngName, relName, totName;

        if (mosra->doBatch())
        {
            // let's write a report
            sRepName = QString("%1/report_%2_p%3-%4.txt").arg(dsInfo.path())
                            .arg(dsInfo.baseName()).arg(level).arg(runs);

            // let's write the actual optimisation problem in lp format
            lpName = QString("%1/lp_%2_p%3-%4.lp").arg(dsInfo.path())
                    .arg(dsInfo.baseName()).arg(level).arg(runs);


            perturbName = QString("%1/tab_%2_p%3-%4.csv").arg(dsInfo.path())
                    .arg(dsInfo.baseName()).arg(level).arg(runs);

            resName = QString("%1/res_%2_p%3-%4.csv").arg(dsInfo.path())
                            .arg(dsInfo.baseName()).arg(level).arg(runs);

            chngName = QString("%1/chg_%2_p%3-%4.csv").arg(dsInfo.path())
                    .arg(dsInfo.baseName()).arg(level).arg(runs);

            relName = QString("%1/rel_%2_p%3-%4.csv").arg(dsInfo.path())
                    .arg(dsInfo.baseName()).arg(level).arg(runs);

            totName = QString("%1/tot_%2_p%3-%4.csv").arg(dsInfo.path())
                    .arg(dsInfo.baseName()).arg(level).arg(runs);
        }
        else
        {
            QDir parentDir(dsInfo.path());
            QFileInfo losFileInfo(mLosFileName);
            QString losFileName = losFileInfo.baseName();

            QStringList dirList;
            dirList << "opt_report" << "opt_lucmatrix" << "opt_result"
                    << "opt_relative" << "opt_total" << "opt_lp" << "opt_tab";

            for (int d=0; d < dirList.size(); ++d)
            {
                QString dir = dirList.at(d);
                NMDebugAI(<< dirList.at(d).toStdString() << " -> ");
                if (!parentDir.exists(dir))
                {
                    if (!parentDir.mkdir(dir))
                    {
                        dir = dsInfo.dir().dirName();
                        dirList.replace(d, dir);
                    }
                }
                NMDebug(<< dirList.at(d).toStdString() << endl );
            }


            sRepName = QString("%1/%2/report_%3.txt").arg(dsInfo.path())
                    .arg(dirList.at(0))
                    .arg(losFileName);

            lpName = QString("%1/%2/lp_%3.lp").arg(dsInfo.path())
                    .arg(dirList.at(5))
                    .arg(losFileName);

            perturbName = QString("%1/%2/tab_%3.csv").arg(dsInfo.path())
                    .arg(dirList.at(6))
                    .arg(losFileName);

            resName = QString("%1/%2/res_%3.csv").arg(dsInfo.path())
                    .arg(dirList.at(2))
                    .arg(losFileName);

            chngName = QString("%1/%2/chg_%3.csv").arg(dsInfo.path())
                    .arg(dirList.at(1))
                    .arg(losFileName);

            relName = relName = QString("%1/%2/rel_%3.csv").arg(dsInfo.path())
                    .arg(dirList.at(3))
                    .arg(losFileName);

            totName = QString("%1/%2/tot_%3.csv").arg(dsInfo.path())
                    .arg(dirList.at(4))
                    .arg(losFileName);
        }

        // --------------------------------------------------------------------------

        mosra->setTimeOut(mosra->getTimeOut());
        if (!mosra->solveLp())
        {
            mosra->writeReport(sRepName);
			continue;
        }
        mosra->getLp()->WriteLp(lpName.toStdString());
        mosra->writeReport(sRepName);

		if (!mosra->mapLp())
			continue;

        vtkSmartPointer<vtkTable> chngmatrix;
        vtkSmartPointer<vtkTable> sumres = mosra->sumResults(chngmatrix);

        NMvtkDelimitedTextWriter* writer = NMvtkDelimitedTextWriter::New();
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
            if (hole != nullptr && hole->GetValue(r))
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
