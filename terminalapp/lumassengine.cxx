
/*
 * lumassengine.cxx
 *
 *  Created on: 21/06/2013
 *      Author: alex
 */

#ifdef BUILD_RASSUPPORT
/// RASDAMAN includes
#ifdef EARLY_TEMPLATE
#define __EXECUTABLE__
#ifdef __GNUG__
#include "raslib/template_inst.hh"
#include "template_rimageio_inst.hh"
#endif
#endif
#endif

#ifdef RMANDEBUG
	int indentLevel;
	bool debugOutput;
#endif

#include "nmlog.h"
#ifdef DEBUG
	int nmlog::nmindent = 1;
#endif

#include "lumassengine.h"
#include <tr1/functional>


#include <QtCore>
#include <QFileInfo>
#include <QThread>
#include <QFuture>

#include "vtkSmartPointer.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkDelimitedTextWriter.h"

#include "MOSORunnable.h"

static const std::string ctx = "LUMASS_engine";

/*
 * \brief Terminal version of LUMASS to run models without graphical user interface
 *
 */

void doMOSO(const QString& losFileName)
{
	NMDebugCtx(ctx, << "...");

	QScopedPointer<NMMosra> mosra(new NMMosra());
	mosra->loadSettings(losFileName);
	if (mosra->doBatch())
	{
		doMOSObatch(losFileName);
		return;
	}


	NMDebugCtx(ctx, << "done!");
}

//void doMOSOrun(const QString& dsfileName,
//		const QString& losSettingsFileName,
//		const QString& perturbItem,
//		float level,
//		int startIdx, int numruns)
//{
//	NMDebugAI( << "run=" << startIdx << endl);
//	QScopedPointer<NMMosra> mosra(new NMMosra());
//
//	NMMsg(<< "Starting run=" << startIdx);
//
//	QFileInfo dsInfo(dsfileName);
//
//	for (int runs=startIdx; runs <= numruns; ++runs)
//	{
//		NMDebugAI(<< endl << "******** PERTURBATION #" << runs << " *************" << endl);
//		// load the file with optimisation settings
//		mosra->loadSettings(losSettingsFileName);
//
//		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
//		reader->SetFileName(dsfileName.toStdString().c_str());
//		reader->Update();
//		vtkPolyData* pd = reader->GetOutput();
//		mosra->setDataSet(pd);
//		mosra->perturbCriterion(perturbItem, level);
//		vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();
//
//		mosra->setTimeOut(mosra->getTimeOut());
//		if (!mosra->solveLp())
//			continue;
//
//		if (!mosra->mapLp())
//			continue;
//
//		vtkSmartPointer<vtkTable> sumres = mosra->sumResults();
//
//		// get rid of admin fields
//		tab->RemoveColumnByName("nm_id");
//		tab->RemoveColumnByName("nm_hole");
//		tab->RemoveColumnByName("nm_sel");
//
//		// let's write a report
//		QString sRepName = QString("%1/report_%2_p%3-%4.txt").arg(dsInfo.path())
//						.arg(dsInfo.baseName()).arg(level).arg(runs);
//		mosra->writeReport(sRepName);
//
//		// now write the input and the result table
//		QString perturbName = QString("%1/%2_p%3-%4.csv").arg(dsInfo.path())
//				.arg(dsInfo.baseName()).arg(level).arg(runs);
//
//		QString resName = QString("%1/res_%2_p%3-%4.csv").arg(dsInfo.path())
//						.arg(dsInfo.baseName()).arg(level).arg(runs);
//
//		vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
//		writer->SetFieldDelimiter(",");
//
//		writer->SetInput(tab);
//		writer->SetFileName(perturbName.toStdString().c_str());
//		writer->Update();
//
//		writer->SetInput(sumres);
//		writer->SetFileName(resName.toStdString().c_str());
//		writer->Update();
//
//		writer->Delete();
//	}
//}

void doMOSObatch(const QString& losFileName)
{
	QScopedPointer<NMMosra> mosra(new NMMosra());
	mosra->loadSettings(losFileName);
	if (!mosra->doBatch())
	{
		NMErr(ctx, << "We don't quite have what we need for batch processing!");
		return;
	}

	QString dsFileName = QString("%1/%2.vtk").arg(mosra->getDataPath())
			.arg(mosra->getLayerName());

	QFileInfo dsinfo(dsFileName);

	if (!dsinfo.isReadable())
	{
		NMErr(ctx, << "Could not read file '" << dsFileName.toStdString() << "'!");
		return;
	}

	int nthreads = 1;//QThread::idealThreadCount();
	int chunksize = mosra->getNumberOfPerturbations() / nthreads;
	int rest = chunksize - (nthreads * chunksize);

	foreach(const QString& item, mosra->getPerturbationItems())
	{
		foreach(const float& level, mosra->getUncertaintyLevels())
		{
			int runstart = 1;
			for (int t=0; t < nthreads; ++t)
			{
				if (t == nthreads-1)
					chunksize += rest;

				MOSORunnable* m = new MOSORunnable();
				m->setData(dsFileName, mosra->getLosFileName(),
						item, level, runstart, chunksize);
				QThreadPool::globalInstance()->start(m);

				runstart += chunksize;
			}
		}
	}
}

void showHelp()
{
    std::cout << std::endl << "LUMASS (lumassengine) 0.9.0" << std::endl << std::endl;
    std::cout << "Usage: lumassengine --moso <settings file (*.los)>" << std::endl << std::endl;
}

int main(int argc, char** argv)
{
	NMDebugCtx(ctx, << "...");

	if (argc < 2)
	{
		showHelp();
		NMDebugCtx(ctx, << "done!");
		return EXIT_SUCCESS;
	}

	enum WhatToDo {
			NM_ENGINE_MOSO,
			NM_ENGINE_SESDM,
			NM_ENGINE_NOPLAN
	};

	WhatToDo todo = NM_ENGINE_NOPLAN;
	QString losFileName = "";

	int arg = 1;
	while (arg < argc-1)
	{
		QString theArg = argv[arg];
		theArg = theArg.toLower();

		if (theArg == "--moso")
		{
			losFileName = argv[arg+1];
			if (losFileName.isNull() || losFileName.isEmpty())
			{
				NMErr(ctx, << "No settings file has been specified!");
				showHelp();
				NMDebugCtx(ctx, << "done!");
				return EXIT_SUCCESS;
			}

			QFileInfo losInfo(losFileName);
			if (!losInfo.isReadable())
			{
				NMErr(ctx, << "Specified file could not be read!");
				NMDebugCtx(ctx, << "done!");
				return EXIT_FAILURE;
			}

			todo = NM_ENGINE_MOSO;
		}

		++arg;
	}


	switch(todo)
	{
	case NM_ENGINE_MOSO:
		doMOSO(losFileName);
		break;
	default:
		NMDebugAI(<< "Can someone tell me what to do?" << std::endl);
		break;
	}

	NMDebugCtx(ctx, << "done!");
	return EXIT_SUCCESS;
}

