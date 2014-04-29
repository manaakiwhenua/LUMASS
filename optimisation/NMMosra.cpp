/*
 * NMMosra., cpp
 *
 *  Created on: 23/03/2011
 *      Author: alex
 */

#define NMDebug(arg)
#define NMDebugAI(arg)
#define NMDebugInd(level, arg)
#define NMDebugTime(arg)
#define NMDebugTimeInd(level, arg)
#define NMDebugCtx(ctx, arg)
#define NMDebugTimeCtx(ctx, arg)


#include "NMMosra.h"
//#include "NMTableCalculator.h"
//#include "NMMfwException.h"
#include <string>
#include <iostream>
#include <sstream>

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QList>
#include <QMap>
#include <QVector>
#include <QPointer>
#include <QStandardItem>
#include <QScopedPointer>
#include <QFileInfo>

#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkDelimitedTextWriter.h"

#include "lp_lib.h"

NMMosra::NMMosra(QObject* parent) //: QObject(parent)
{
	NMDebugCtx(ctxNMMosra, << "...");

	this->setParent(parent);
	this->mLp = new HLpHelper();
	this->reset();

	NMDebugCtx(ctxNMMosra, << "done!");
}

NMMosra::~NMMosra()
{
	NMDebugCtx(ctxNMMosra, << "...");

	if (this->mLp != 0)
	{
		this->mLp->DeleteLp();
		delete this->mLp;
	}

	NMDebugCtx(ctxNMMosra, << "done!");
}

void
NMMosra::setDataSet(const vtkDataSet* dataset)
{
	if (dataset != 0)
		this->mDataSet = const_cast<vtkDataSet*>(dataset);
	else
	{
		NMErr(ctxNMMosra, << "data set is NULL!");
	}
}


void NMMosra::reset(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	this->mLp->DeleteLp();
	this->msLosFileName.clear();
	this->mDataSet = 0;

	this->mlNumDVar = 0;
	this->mlNumOptFeat = 0;
	this->mlCriCons = 0;
	this->mlLpCols = 0;
	this->mdAreaSelected = 0;
	this->mdAreaTotal = 0;

	this->muiTimeOut = 60;
	this->miNumOptions = 0;
	this->meDVType = NMMosra::NM_MOSO_REAL;
	this->meScalMeth = NMMosra::NM_MOSO_WSUM;

	this->mmslAreaCons.clear();
	this->mmslCriCons.clear();
	this->mmslCriteria.clear();
	this->mmslEvalFields.clear();
	this->mmslObjCons.clear();
	this->mmslObjectives.clear();

	this->mbCanceled = false;

	msDataPath = std::getenv("HOME");
	mslPerturbItems.clear();
	mflUncertainties.clear();
	mlReps=1;

	NMDebugCtx(ctxNMMosra, << "done!");
}

bool
NMMosra::doBatch()
{
	bool ret = false;

	if (   !msDataPath.isEmpty()
		&& mslPerturbItems.size() > 0
		&& mflUncertainties.size() > 0
	   )
	{
		ret = true;
	}

	return ret;
}

int NMMosra::loadSettings(QString fileName)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// reset this objects vars
	this->reset();

	QFile los(fileName);
	if (!los.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		NMErr(ctxNMMosra, << "failed reading settings file!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}
	this->msLosFileName = fileName;

	QTextStream str(&los);

	QString rep;
	QTextStream sReport(&rep);
	QString sLine, sVarName, sValueStr;
	int length, sep;
	long lnumOpt=0;
	long numline = 0;

	enum ParSec {
		problem,
		criteria,
		objectives,
		arealcons,
		cricons,
		objcons,
		batch,
		nosection
	};

	ParSec section = nosection;

	sReport << "Import Report - '" << fileName << "'" << endl << endl;
	NMDebugAI( << "parsing settings file ..." << endl);
	while (!str.atEnd())
	{
		sLine = str.readLine();

		//increment the line number
		numline++;

		//eliminate leading and trailing blanks
		sLine = sLine.simplified();

		// skip empty lines and comments
		if (sLine.isEmpty() || !sLine.left(1).compare(tr("#")))
			continue;

		//detect processing mode
		if (!sLine.compare(tr("<PROBLEM>"), Qt::CaseInsensitive))
		{
			section = problem;
			continue;
		}
		else if (!sLine.compare(tr("<CRITERIA>"), Qt::CaseInsensitive))
		{
			section = criteria;
			continue;
		}
		else if (!sLine.compare(tr("<OBJECTIVES>"), Qt::CaseInsensitive))
		{
			section = objectives;
			continue;
		}
		else if (!sLine.compare(tr("<AREAL_CONSTRAINTS>"), Qt::CaseInsensitive))
		{
			section = arealcons;
			continue;
		}
		else if (!sLine.compare(tr("<CRITERIA_CONSTRAINTS>"), Qt::CaseInsensitive))
		{
			section = cricons;
			continue;
		}
		else if (!sLine.compare(tr("<OBJECTIVE_CONSTRAINTS>"), Qt::CaseInsensitive))
		{
			section = objcons;
			continue;
		}
		else if (!sLine.compare(tr("<BATCH_SETTINGS>"), Qt::CaseInsensitive))
		{
			section = batch;
			continue;
		}
		else if (!sLine.right(5).compare(tr("_END>"), Qt::CaseInsensitive))
		{
			section = nosection;
			continue;
		}

		//check, if we are dealing with a valid data section
		if (section == nosection)
		{
			sReport << "Line " << numline << " does not belong to a valid data section" << endl;
			continue;
		}

		//look for an equal sign to indicate data assignment
		sep = sLine.indexOf('=');

		//if there is no equal sign skip processing of this line
		if (sep == -1)
		{
			sReport << "Line " << numline << " contains invalid data" << endl;
			continue;
		}

		//split the line string into the variable name and the value string
		length = sLine.size();
		sVarName = sLine.left(sep);
		sValueStr = sLine.right(length-1-sep);


		//process the line depending on the current data section
		//if (problem)	//----------------------------------------------PROBLEM-------------
		switch(section)
		{
		case problem:
		{
			if (sVarName.compare(tr("DVTYPE"), Qt::CaseInsensitive) == 0)
			{
				if (sValueStr.indexOf(tr("DVTYPE_BINARY"), Qt::CaseInsensitive) != -1)
					this->meDVType = NMMosra::NM_MOSO_BINARY;
				else if (sValueStr.indexOf(tr("DVTYPE_INTEGER"), Qt::CaseInsensitive) != -1)
					this->meDVType = NMMosra::NM_MOSO_INT;
				else // DVTYPE_CONTINUOUS
					this->meDVType = NMMosra::NM_MOSO_REAL;

				NMDebugAI( << "DEVTYPE: " << this->meDVType << endl);
			}
			else if (sVarName.compare(tr("CRITERION_LAYER"), Qt::CaseInsensitive) == 0)
			{
				if (this->msLayerName.compare(sValueStr, Qt::CaseInsensitive) != 0)
					this->mDataSet = 0;

				this->msLayerName = sValueStr;
				NMDebugAI( << "LayerName: " << this->msLayerName.toStdString() << endl);
			}
			else if (sVarName.compare(tr("LAND_USE_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msLandUseField = sValueStr;
				NMDebugAI( << "LandUseField: " << this->msLandUseField.toStdString() << endl);
			}
			else if (sVarName.compare(tr("AREA_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msAreaField = sValueStr;
				NMDebugAI( << "AreaField: " << this->msAreaField.toStdString() << endl);
			}
		}
		break;
		//else if (criteria)	//---------------------------------------------CRITERIA-----------
		case criteria:
		{
			if (sVarName.compare(tr("NUM_OPTIONS"), Qt::CaseInsensitive) == 0)
			{
				bool ok;
				long lo = sValueStr.toLong(&ok);
				if (ok)
					this->miNumOptions = lo;
				else
					sReport << "Line " << numline << " contains an invalid number" << endl;

				NMDebugAI( << "no options: " << this->miNumOptions << endl);
			}
			else if (sVarName.compare(tr("OPTIONS"), Qt::CaseInsensitive) == 0)
			{
				QStringList tmpList = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (tmpList.size() == this->miNumOptions)
				{
					this->mslOptions.clear();
					this->mslOptions << tmpList;
				}
				else
				{
					sReport << "Line " << numline << " contains an invalid number of options" << endl;
				}

				NMDebugAI( << "options: " << this->mslOptions.join(tr(" ")).toStdString() << endl);

			}
			else if (sVarName.indexOf(tr("CRI_"), Qt::CaseInsensitive) != -1)
			{
				QStringList criFieldNames = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (criFieldNames.size() == this->miNumOptions + 1)
				{
					QString scri = criFieldNames.at(0);
					criFieldNames.removeAt(0);
					this->mmslCriteria.insert(scri, criFieldNames);

					NMDebugAI( << "criterion: " << scri.toStdString() << " " << this->mmslCriteria.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
					sReport << "Line " << numline << " contains an invalid number of criteria" << endl;
				}
			}
			else if (sVarName.indexOf(tr("EVAL_"), Qt::CaseInsensitive) != -1)
			{
				QStringList evalFieldNames = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (evalFieldNames.size() == this->miNumOptions + 1)
				{
					QString scri = evalFieldNames.at(0);
					evalFieldNames.removeAt(0);
					this->mmslEvalFields.insert(scri, evalFieldNames);

					NMDebugAI( << "criterion evaluation fields: " << scri.toStdString() << " " << this->mmslEvalFields.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
					sReport << "Line " << numline << " contains an invalid number of criterion evaluation fields" << endl;
				}
			}

		}
		break;
		//else if (objectives)	//----------------------------------------OBJECTIVES----------
		case objectives:
		{
			QString sAggrMeth;
			if (sVarName.compare(tr("AGGR_METHOD"), Qt::CaseInsensitive) == 0)
			{
				if (sValueStr.compare(tr("WSUM")) == 0)
				{
					this->meScalMeth = NMMosra::NM_MOSO_WSUM;
					sAggrMeth = tr("Weighted Sum");
				}
				else
				{
					this->meScalMeth = NMMosra::NM_MOSO_INTERACTIVE;
					sAggrMeth = tr("Interactive");
				}
				NMDebugAI( << "Scalarisation method is '" << sAggrMeth.toStdString() << "'" << endl);
			}
			else if (sVarName.indexOf(tr("OBJ_"), Qt::CaseInsensitive) != -1)
			{
				 QStringList objs = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				 if (objs.size() != 0)
				 {
					 QString obj = objs.takeAt(1);
					 this->mmslObjectives.insert(obj, objs);
					 NMDebugAI( << "obj: " << obj.toStdString() << ": "
							 << this->mmslObjectives.find(obj).value().join(tr(" ")).toStdString() << endl);
				 }
			}
		}
		break;
		//else if (arealcons)	//-----------------------------------------AREAL_CONS------------
		case arealcons:
		{
			if (sVarName.indexOf(tr("AREAL_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList arCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (arCons.size() != 0)
				{
					this->mmslAreaCons.insert(sVarName, arCons);
					NMDebugAI( << "areal cons: " << sVarName.toStdString() << ": "
							<< this->mmslAreaCons.find(sVarName).value().join(tr(" ")).toStdString() << endl);

					// check, whether we've got a zoning constraint here and if so, initialise the
					// zones area with 0
					QString luopt = arCons.at(0);
					if (luopt.contains(':', Qt::CaseInsensitive))
					{
						QStringList luoptlist = luopt.split(tr(":"), QString::SkipEmptyParts);
						QString zonefield = luoptlist.at(1);
						QString opt = luoptlist.at(0);

						// check, whether we've got already a map created for this zonefield
						QMap<QString, QMap<QString, double> >::iterator zonesIt;
						QMap<QString, QMap<QString, int> >::iterator lengthIt;
						zonesIt = this->mmslZoneAreas.find(zonefield);
						lengthIt = this->mmslZoneLength.find(zonefield);
						if (zonesIt == this->mmslZoneAreas.end())
						{
							QMap<QString, double> zoneArea;
							QMap<QString, int> zoneLength;
							zoneArea.insert(opt, 0);
							zoneLength.insert(opt, 0);
							this->mmslZoneAreas.insert(zonefield, zoneArea);
							this->mmslZoneLength.insert(zonefield, zoneLength);
						}
						else
						{
							zonesIt.value().insert(opt, 0);
							lengthIt.value().insert(opt, 0);
							this->mmslZoneAreas.insert(zonefield, zonesIt.value());
							this->mmslZoneLength.insert(zonefield, lengthIt.value());
						}
					}
				}
			}
		}
		break;
		//else if (cricons) //----------------------------------------------ATTR_CONS--------
		case cricons:
		{
			if (sVarName.indexOf(tr("CRI_CONS_"), Qt::CaseInsensitive) != -1)
			{
				NMDebugInd(nmlog::nmindent+1, << "gonna split raw list: " << sValueStr.toStdString() << endl);
				QStringList outerList = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (outerList.size() != 0)
				{
					QString criLabel = outerList.takeAt(0);
					NMDebugInd(nmlog::nmindent+1, << "criLabel is '" << criLabel.toStdString() << "'" << endl);
					QString luLabel = outerList.takeAt(0);
					NMDebugInd(nmlog::nmindent+1, << "land use is '" << luLabel.toStdString() << "'" << endl);

					QMap<QString, QStringList> innerMap;
					innerMap.insert(luLabel, outerList);

					this->mmslCriCons.insert(criLabel, innerMap);

					NMDebugAI( << "cri cons: " << criLabel.toStdString() << ": "
							<< luLabel.toStdString() << ": "
							<< outerList.join(tr(" ")).toStdString() << endl);
				}
			}
		}
		break;
		//else if (objcons)	//------------------------------------------------OBJ_CONS-------
		case objcons:
		{
			if (sVarName.indexOf(tr("OBJ_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList objCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (objCons.size() > 0)
				{
					QString objkey = sVarName + QString(tr("_%1")).arg(objCons.value(0));
					this->mmslObjCons.insert(objkey, objCons);
					NMDebugAI( << objkey.toStdString() << ": "
							<< this->mmslObjCons.find(objkey).value().join(tr(" ")).toStdString() << endl);
				}
			}
		}
		break;
		case batch:
		{
			if (sVarName.compare("DATAPATH", Qt::CaseInsensitive) == 0)
			{
				QFileInfo fi(sValueStr);
				if (fi.isReadable())
				{
					this->msDataPath = sValueStr;
					NMDebugAI(<< "batch data path: " << this->msDataPath.toStdString() << endl);
				}
			}
			else if (sVarName.compare("PERTURB", Qt::CaseInsensitive) == 0)
			{
				this->mslPerturbItems = sValueStr.split(" ");
				if (mslPerturbItems.size() > 0)
				{
					NMDebugAI(<< "Criteria/constraints to be perturbed: ");
					foreach(const QString& pc, this->mslPerturbItems)
					{
						NMDebug(<< pc.toStdString() << " ");
					}
					NMDebug(<< endl);
				}
				else
				{
					NMDebugAI(<< "No perturbation criteria provided!" << endl);
				}
			}
			else if (sVarName.compare("UNCERTAINTIES", Qt::CaseInsensitive) == 0)
			{
				QStringList lunsure = sValueStr.split(" ");
				bool bok;
				float val;
				foreach(const QString& vstr, lunsure)
				{
					val = vstr.toFloat(&bok);
					if (bok)
						this->mflUncertainties.push_back(val);
				}

				if (this->mflUncertainties.size() > 0)
				{
					NMDebugAI(<< "Perturbation levels: ");
					foreach(const float& f, this->mflUncertainties)
					{
						NMDebug(<< f << " ");
					}
					NMDebug(<< endl);
				}
				else
				{
					NMDebugAI(<< "No uncertainty levels for perturbation provided!" << endl);
				}
			}
			else if (sVarName.compare("REPETITIONS", Qt::CaseInsensitive) == 0)
			{
				if (!sValueStr.isEmpty())
				{
					bool bok;
					long reps = sValueStr.toLong(&bok);
					if (bok)
					{
						this->mlReps = reps;
						NMDebugAI(<< "Number of perturbations: " << reps << endl);
					}
				}
			}
			else if (sVarName.compare("TIMEOUT", Qt::CaseInsensitive) == 0)
			{
				if (!sValueStr.isEmpty())
				{
					bool bok;
					unsigned int timeout = sValueStr.toUInt(&bok);
					if (bok)
					{
						this->muiTimeOut = timeout;
						NMDebugAI(<< "Solver timeout: " << timeout << endl);
					}
				}
			}
		}
		break;
		default:
			break;
		}
	}


	NMDebug(<< endl << "Report..." << endl << sReport.readAll().toStdString() << endl);

	los.close();

	NMDebugCtx(ctxNMMosra, << "done!");

	return 1;
}

//void NMMosra::setLayer(NMLayer* layer)
//{
//	this->mLayer = layer;
//	this->msLayerName = layer->objectName();
//}

// this function is useful when the optimisation settings
// were read from a file and hence only the layer name
// was set; the owner of the object then can read the
// layer name and make sure, that the appropriate dataset
// is set afterwards
QString NMMosra::getLayerName(void)
{
	return this->msLayerName;
}

int NMMosra::solveLp(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// check, whether all settings are ok
	if (!this->checkSettings())
	{
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	this->makeLp();

	if (!this->addObjFn())
	{
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
			this->mmslObjCons.size() > 0)
		this->addObjCons();

	// doe we have any additional constraints?
	if (this->mmslAreaCons.size() > 0)
	{
		if (!this->addExplicitAreaCons())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			return 0;
		}
	}

	if (this->mmslCriCons.size() > 0)
	{
		if (!this->addCriCons())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			return 0;
		}
	}

	if (!this->addImplicitAreaCons())
	{
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	// you've got a minute ... if setTimeOut() hasn't been called
	NMDebugAI(<< "solver time out set to " << this->muiTimeOut
			<< " seconds!" << std::endl);

	this->mLp->SetTimeout(this->muiTimeOut);
	this->mbCanceled = false;
	this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);

	this->mLp->Solve();

	this->createReport();

	NMDebug(<< this->getReport().toStdString() << endl);

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

void NMMosra::createReport(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// let's put out some info about the solution, if any
	QString streamstring;
	QTextStream sRes(&streamstring);
	sRes.setRealNumberNotation(QTextStream::SmartNotation);
	sRes.setRealNumberPrecision(15);

	sRes << endl << endl;
	sRes << tr("\tProblem setting details") << endl;
	sRes << tr("\t-----------------------") << endl << endl;
	sRes << this->msSettingsReport << endl;

	sRes << tr("\n\n\tResults from lp_solve 5.5\n"
		           "\t-------------------------\n\n"
		  "Please read the lp_solve documentation for\n"
		  "further information about optimisation results!\n\n");

	//get return value from the solve function
	int ret = this->mLp->GetLastReturnFromSolve();

	sRes << tr("Solver returned: ");
	switch (ret)
	{
	case -2: sRes << tr("NOMEMORY\n"); break;
	case 0: sRes << tr("OPTIMAL\n"); break;
	case 1: sRes << tr("SUBOPTIMAL\n"); break;
	case 2: sRes << tr("INFEASIBLE\n"); break;
	case 3: sRes << tr("UNBOUNDED\n"); break;
	case 4: sRes << tr("DEGENERATE\n"); break;
	case 5: sRes << tr("NUMFAILURE\n"); break;
	case 6: sRes << tr("USERABORT\n"); break;
	case 7: sRes << tr("TIMEOUT\n"); break;
	case 10: sRes << tr("PROCFAIL\n"); break;
	case 11: sRes << tr("PROCBREAK\n"); break;
	case 12: sRes << tr("FEASFOUND\n"); break;
	case 13: sRes << tr("NOFEASFOUND\n"); break;
	}

	if (ret != 0 && ret != 1 && ret != 12)
	{
		sRes << "There was neither a sub-optimal\n"
				"nor an optimal solution found by\n"
				"lp_solve 5.5!\n"
				"RETURN CODE = " << ret << " - see lp_solve doc!\n" << endl;
	}

	//log the number of solutions
	sRes << "Number of solutions: " << this->mLp->GetSolutionCount() << endl << endl;

	//log the objective function
	sRes << "Objective function result = " << this->mLp->GetObjective() << endl << endl;

	//get the values of the constraints
	int iNumCons = this->mLp->GetNRows();
	double *pdCons;
	this->mLp->GetPtrConstraints(&pdCons);

	//log the objective constraints
	if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
			this->mmslObjCons.size() > 0)
	{
		sRes << tr("OBJECTIVE CONSTRAINTS\n");
		QMap<QString, QStringList>::const_iterator oit = this->mmslObjCons.constBegin();
		for (; oit != this->mmslObjCons.constEnd(); oit++)
		{
			int index = this->mLp->GetNameIndex(oit.key().toStdString(), true);
			if (index >= 0)
				sRes << oit.key() << " = " << pdCons[index-1] << " ( "
					<< oit.value().at(1) << " " << oit.value().at(2) << " )" << endl;
		}
		sRes << endl;
	}

	//log the explicit areal constraints
	if (this->mmslAreaCons.size() > 0)
	{
		sRes << tr("USER DEFINED AREAL CONSTRAINTS\n");
		QMap<QString, QStringList>::const_iterator ait =
				this->mmslAreaCons.constBegin();
		int totalcount = 0;
		for (int r=0; ait != this->mmslAreaCons.constEnd(); ++ait, ++r)
		{
			QString sACL;
			QStringList zonespec;
			int index;
			double dval;
			bool ok;
			int nit = 1;

			if (ait.value().at(0).contains(":", Qt::CaseInsensitive))
			{
				zonespec = ait.value().at(0).split(tr(":"), QString::SkipEmptyParts);
				//nit = 2;
			}

			for (int q=0; q < nit; ++q)
			{
				sACL = this->mmslAreaConsLabel.at(totalcount);
				index = this->mLp->GetNameIndex(sACL.toStdString() , true);
				if (index >= 0)
				{
					dval = ait.value().at(2).toDouble(&ok);
					dval = this->convertAreaUnits(dval, ait.value().at(3), zonespec);
					if (q == 1)
						dval = 0;

					sRes << sACL << " = " << pdCons[index-1] << " ( "
						<< ait.value().at(1) << " " << dval << " )" << endl;
				}
				++totalcount;
			}
		}
		sRes << endl;
	}

	//log the attributive constraints
	if (this->mmslCriCons.size() > 0)
	{
		sRes << tr("CRITERIA CONSTRAINTS\n");

		QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit =
				this->mmslCriCons.constBegin();
		QMap<QString, QStringList>::const_iterator criit;
		for (; crilabit != this->mmslCriCons.constEnd(); ++crilabit)
		{
			for (criit = crilabit.value().constBegin(); criit != crilabit.value().constEnd(); ++criit)
			{
				QString compOp = criit.value().at(criit.value().size()-2);
				QString compTypeLabel;
				if (compOp.compare(tr("<=")) == 0)
					compTypeLabel = "upper";
				else if (compOp.compare(tr(">=")) == 0)
					compTypeLabel = "lower";
				else if (compOp.compare(tr("=")) == 0)
					compTypeLabel = "equals";

				QString sCL = crilabit.key() + QString(tr("_%1_%2")).arg(criit.key()).arg(compTypeLabel);
				int index = this->mLp->GetNameIndex(sCL.toStdString().c_str(), true);

				if (index >= 0)
				{
					sRes << sCL << " " << compOp << " " << pdCons[index -1] << endl;
				}
				else
				{
					QString tmplab = crilabit.key() + QString(tr("_%1")).arg(criit.key());
					sRes << "error reading cirterion constraint for '"
							<< tmplab << "'!" << endl;
				}
			}
		}
		sRes << endl;
	}

	// DEBUG - we just dump all constraints values here
	NMDebugAI( << endl << "just dumping all constraint values .... " << endl);
	int nrows = this->mLp->GetNRows();
	for (int q=1; q < nrows; ++q)
	{
		QString name(this->mLp->GetRowName(q).c_str());
		if (name.contains("Feature"))
			continue;
		int op = this->mLp->GetConstraintType(q-1);
		double cv = pdCons[q-1];
		char fv[256];
		::sprintf(fv, "%g", cv);
		NMDebugAI(<< name.toStdString() << " " << op << " " << fv << endl);
	}
	NMDebug(<< endl);


	sRes << endl;

	QString platz = tr("\t\t");
	sRes << "SENSITIVITY - CONSTRAINTS\n\n";
	sRes << "ROW" << platz << tr("\t") << "DUAL VALUE" << platz << "DUAL FROM" << platz << "DUAL TILL\n";
	//get some info about sensitivity
	double *pDuals, *pDualsFrom, *pDualsTill;
	if (this->mLp->GetPtrSensitivityRHS(&pDuals, &pDualsFrom, &pDualsTill))
	{
		int nRows = this->mLp->GetNameIndex("Feature_0a", true);//this->mLp->GetNRows();
		for (int r=1; r < nRows; r++)
		{
			sRes << QString(this->mLp->GetRowName(r).c_str()) << platz << tr("\t") <<
					pDuals[r] << platz << pDualsFrom[r] << platz << pDualsTill[r] << endl;
		}
	}

	this->msReport.clear();
	this->msReport = sRes.readAll();

	NMDebugCtx(ctxNMMosra, << "done!");

}

QString NMMosra::getReport(void)
{
	return this->msReport;
}

void NMMosra::writeReport(QString fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		NMErr(ctxNMMosra, << "failed writing file '" << fileName.toStdString() << "'!");
		return;
	}

	QByteArray bar(this->msReport.toStdString().c_str());
	file.write(bar);
	file.close();
}

int NMMosra::checkSettings(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	QString settrep;
	QTextStream sstr(&settrep);

	sstr << "type of DV (0=REAL | 1=INT | 2=BINARY): " << this->meDVType << endl;

	// get the attributes of the layer
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);

	NMDebugAI(<< "checking area field ..." << endl);
	//  get the total area of the layer (summing the provided field's data)
	if (this->msAreaField.isEmpty())
	{
		NMErr(ctxNMMosra, << "no area field specified!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	if (!dsAttr->HasArray(this->msAreaField.toStdString().c_str()))
	{
		NMErr(ctxNMMosra, << "specified area field could not be found!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	// --------------------------------------------------------------------------------------------------------
	NMDebugAI(<< "calculating area and counting features ..." << endl);
	vtkDataArray* areaAr = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
	vtkDataArray* nm_hole = dsAttr->GetArray("nm_hole");
	int numTuples = areaAr->GetNumberOfTuples();
	int numFeat = 0;

	QMap<QString, QMap<QString, double> >::iterator zonesIt;
	QMap<QString, QMap<QString, int> >::iterator zonesLenIt;
	QMap<QString, double>::iterator optIt;
	QMap<QString, int>::iterator optLenIt;

	double tmpVal;
	int tmpLen;
	bool arealCriValid = true;
	this->mdAreaTotal = 0;

	for (int cs=0; cs < areaAr->GetNumberOfTuples(); cs++)
	{
		if (nm_hole->GetTuple1(cs) == 1)
			continue;

		this->mdAreaTotal += areaAr->GetTuple1(cs);
		numFeat++;

		// iterate over the initialised zones and calc areas
		zonesIt = this->mmslZoneAreas.begin();
		zonesLenIt = this->mmslZoneLength.begin();
		for (; zonesIt != this->mmslZoneAreas.end(); ++zonesIt, ++zonesLenIt)
		{
			vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(
					dsAttr->GetAbstractArray(zonesIt.key().toStdString().c_str()));
			if (zoneAr == 0)
			{
				arealCriValid = false;
				NMErr(ctxNMMosra, << "specified zone field '" << zonesIt.key().toStdString()
						<< "' does not exist in the data base!");
				continue;
			}

			optIt = zonesIt.value().begin();
			optLenIt = zonesLenIt.value().begin();
			for (; optIt != zonesIt.value().end(); ++optIt, ++optLenIt)
			{
				if (zoneAr->GetValue(cs).find(optIt.key().toStdString()) != std::string::npos)
				{
					tmpVal = optIt.value() + areaAr->GetTuple1(cs);
					tmpLen = optLenIt.value() + 1;
					zonesIt.value().insert(optIt.key(), tmpVal);
					zonesLenIt.value().insert(optLenIt.key(), tmpLen);
				}
			}
		}
	}

	// report total area to the user
	this->mlNumOptFeat = numFeat;
	sstr << "total area from " << this->msAreaField
			<< "(" << numTuples << " | " << numFeat << ")" << " is " << this->mdAreaTotal << endl;


	// iterate over the initialised zones and report areas
	zonesIt = this->mmslZoneAreas.begin();
	zonesLenIt = this->mmslZoneLength.begin();
	for (; zonesIt != this->mmslZoneAreas.end(); ++zonesIt, ++zonesLenIt)
	{
		vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(zonesIt.key().toStdString().c_str()));
		if (zoneAr == 0)
			continue;

		optIt = zonesIt.value().begin();
		optLenIt = zonesLenIt.value().begin();
		for (; optIt != zonesIt.value().end(); ++optIt, ++optLenIt)
		{
			sstr << "total area for option '" << optIt.key() << "' with respect to zone field '" << zonesIt.key() <<
					"' = " << optIt.value() << endl;
			sstr << "no of features for option '" << optLenIt.key() << "' with respect to zone field '"
					<< zonesLenIt.key() << "' = " << optLenIt.value() << endl;
		}
	}

	// -----------------------------------------------------------------------------------------------------------------
	// now actually check the areas
	bool bConv = true;
	QMap<QString, QStringList>::const_iterator acIt = this->mmslAreaCons.constBegin();
	double totalConsArea = 0;
	for (; acIt != this->mmslAreaCons.constEnd(); ++acIt)
	{
		// get the user specified area constraint value and add it to the total of all area constraints
		if (acIt.value().size() < 4)
		{
			NMErr(ctxNMMosra, << "areal constraint '" << acIt.key().toStdString()
					<< ": " << acIt.value().join(" ").toStdString() << "' is invalid! Check number of parameters!");
			arealCriValid = false;
			continue;
		}

		QString unittype = acIt.value().at(3);
		QStringList zonespec;
		double val = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, zonespec);

		// now check, whether we've got an issue with one of the zonal values
		QString OptZone = acIt.value().at(0);
		if (OptZone.contains(":", Qt::CaseInsensitive))
		{
			QStringList ozlist = OptZone.split(tr(":"), QString::SkipEmptyParts);
			QString opt = ozlist.at(0);
			QString zone = ozlist.at(1);

			val = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, ozlist);

			if (val > this->mmslZoneAreas.find(zone).value().find(opt).value())
			{
				NMErr(ctxNMMosra, << "area constraint for option '" << opt.toStdString() << "' with respect to zone field '"
						<< zone.toStdString() << "' exceeds the available area for that option in that zone!");
			//	arealCriValid = false;
			}
		}
		totalConsArea += val;
	}

	// now check on the total area
//	if (totalConsArea > this->mdAreaTotal)
//	{
//		NMErr(ctxNMMosra, << "the specified area constraints exceed the total available area!");
//		arealCriValid = false;
//	}

	// we just don't bother checking it as above, which allows for having constraints like
	// a >= x
	// a <= y with y>=x

	// well we don't bother with the above but if we're missing the unittype, we have to pull out!
	//arealCriValid = true;


	// selection
	// TODO: add some meaningful code for selections
//	if (this->mLayer->getLayerType() == NMLayer::NM_VECTOR_LAYER)
//	{
//		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this->mLayer);
//		this->mlNumOptFeat = vl->getNumberOfFeatures();
//
//		NMDebugAI( << "layer features: " << this->mlNumOptFeat << endl);
//	}
//	else // NM_IMAGE_LAYER
//	{
//		; //TODO: in case of an image layer return the number of
//		  //pixels
//	}

	// cell id field = vtkPolyData cellId

	// --------------------------------------------------------------------------------------------------------
	NMDebugAI(<< "checking performance indicator fields for optimisation criteria ..." << endl);
	// performance indicator fields
	QMap<QString, QStringList>::const_iterator criit =
			this->mmslCriteria.constBegin();

	bool criValid = true;
	for (; criit != this->mmslCriteria.constEnd(); ++criit)
	{
		// check each given performance indicator field
		QStringList fields = criit.value();
		for (int f=0; f < fields.size(); ++f)
		{
			if (!dsAttr->HasArray(fields.at(f).toStdString().c_str()))
			{
				criValid = false;
				NMErr(ctxNMMosra, << "couldn't find performance indicator '"
						<< fields.at(f).toStdString() << "'!");
			}
		}
	}

	NMDebugAI(<< "checking fields for evaluating criteria performance ..." << endl);
	// performance indicator fields
	criit = this->mmslEvalFields.constBegin();

	bool evalValid = true;
	for (; criit != this->mmslEvalFields.constEnd(); ++criit)
	{
		// check each given performance indicator field
		QStringList fields = criit.value();
		for (int f=0; f < fields.size(); ++f)
		{
			if (!dsAttr->HasArray(fields.at(f).toStdString().c_str()))
			{
				evalValid = false;
				NMErr(ctxNMMosra, << "couldn't find performance evaluation indicator '"
						<< fields.at(f).toStdString() << "'!");
			}
		}
	}

	// check the performance indicator fields specified with
	// criteria constraints
	bool criConsValid = true;
	if (this->mmslCriCons.size() > 0)
	{
		NMDebugAI(<< "checking performance indicator fields for attributive constraints ..." << endl);
		QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit =
				this->mmslCriCons.constBegin();

		for (; crilabit != this->mmslCriCons.end(); ++crilabit)
		{
			QMap<QString, QStringList> luFieldList = crilabit.value();
			criit = luFieldList.begin();
			for (; criit != luFieldList.end(); ++criit)
			{
				QStringList zonespec;
				QString landuse;
				QString zone = "";
				if (criit.key().contains(tr(":"), Qt::CaseInsensitive))
				{
					zonespec = criit.key().split(tr(":"), QString::SkipEmptyParts);
					landuse = zonespec.at(0);
					zone = zonespec.at(1);

					// check for zone field
					if (!dsAttr->HasArray(zone.toStdString().c_str()))
					{
						criConsValid = false;
						NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: couldn't find zone field '"
								<< zone.toStdString() << "'!");
					}
				}
				else
					landuse = criit.key();

				QStringList fieldList = criit.value();
				if (landuse.compare(tr("total"), Qt::CaseInsensitive) == 0)
				{
					// look through m_iNumOptions fields
					for (int f=0; f < this->miNumOptions; ++f)
					{
						if (!dsAttr->HasArray(fieldList.at(f).toStdString().c_str()))
						{
							criConsValid = false;
							NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
									<< fieldList.at(f).toStdString() << "'!");
						}
					}
				}
				else
				{
					// check whether the given land use matches one of the specified ones
					if (!this->mslOptions.contains(landuse))
					{
						criConsValid = false;
						NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: specified resource '"
								<< landuse.toStdString() << "' does not match any of "
								"the previously specified resources!");
						continue;
					}

					// look for the specified performance field of the given land use
					if (!dsAttr->HasArray(fieldList.at(0).toStdString().c_str()))
					{
						criConsValid = false;
						NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
								<< fieldList.at(0).toStdString() << "'!");
					}
				}
			}
		}
	}

	// --------------------------------------------------------------------------------------------------------
	NMDebugAI(<< "calculating size of the optimsation matrix ..." << endl);
	/*
	 * structure of the lp matrix
	 * here displayed for two options (land uses)
	 *
	 * i : feature index
	 * r : land use index
	 *
	 * X_i_r: area share of land-use r allocated to feature i
	 * b_i  : binary conditional variable we need to model that
	 *        a feature has to be either completely covered by
	 *        any combination of the land use options or not at all
	 *
	 * note: row-0 contains the objective function and cell 0,0 contains
	 *       the objective function value; row-1 and beyond contain the
	 *       constraints of the model
	 *
	 *colindex:     0     1       2      3      4       5
	 *row-0           | X_0_1 | X_0_2 | b_0 | X_1_1 | X_1_2 | b_1 | ...
	 *row-1
	 *
	 */
	this->mlNumArealDVar = this->miNumOptions * this->mlNumOptFeat;
	sstr << "mlNumArealDVar = miNumOptions * mlNumOptFeat = "
			<< this->mlNumArealDVar << " = " << this->miNumOptions << " * "
			<< this->mlNumOptFeat << endl;

	this->mlNumDVar =  this->mlNumArealDVar + this->mlNumOptFeat;
	sstr << "mlNumDvar =  mlNumArealDVar + mlNumOptFeat = "
			<< this->mlNumDVar << " = " << this-> mlNumArealDVar
			<< " + " << this->mlNumOptFeat << endl;

	// number of columns of the decision matrix
	this->mlLpCols = this->mlNumDVar + 1;
	sstr << "mlLpCols = mlNumDvar + 1  = " << this->mlLpCols << endl;

	// Scalarisation method
	QString sMeth;
	if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
		sMeth = tr("Weighted Sum");
	else
		sMeth = tr("Interactive");
	sstr << "Scalarisation Method: " << sMeth << endl << endl;

	this->msSettingsReport = sstr.readAll();

	NMDebug(<< this->msSettingsReport.toStdString() << endl);

	NMDebugCtx(ctxNMMosra, << "done!");
	if (!criValid || !criConsValid || !evalValid || !arealCriValid)
	{
		NMErr(ctxNMMosra, << "The configuration file contains invalid settings!");
		return 0;
	}
	else
		return 1;
}

int NMMosra::mapLp(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// check return value from lp_solve
	int ret = this->mLp->GetLastReturnFromSolve();

	if (ret != 0 && ret != 1 && ret != 12)
	{
		NMDebugAI( << "unfortunately no feasible solution!" << endl);
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	// get the attributes of the layer
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	long lNumCells = hole->GetNumberOfTuples();

	// create a new result field for the resource option
	vtkSmartPointer<vtkStringArray> opt_str;
	opt_str = vtkSmartPointer<vtkStringArray>::New();
	opt_str->SetName("OPT_STR");
	opt_str->Allocate(lNumCells, 100);
	NMDebugAI( << "created attribute 'OPT_STR'" << endl);


	// check for the scoring attributes
	std::vector< vtkSmartPointer< vtkDoubleArray > > vValAr;
	for (int option=0; option < this->miNumOptions; option++)
	{
		QString optx_val = QString(tr("OPT%1_VAL")).arg(option+1);
		vtkSmartPointer<vtkDoubleArray> var = vtkSmartPointer<vtkDoubleArray>::New();
		var->SetName(optx_val.toStdString().c_str());
		var->Allocate(lNumCells, 100);
		vValAr.push_back(var);

		NMDebugAI( << "created scoring attribute '" << optx_val.toStdString() << "'" << endl);
	}

	// get the decision vars
	double * pdVars;
	this->mLp->GetPtrVariables(&pdVars);

	// some more vars
	QString sColName, sOptStr;
	double dVal;
	int iValIndex;
	const int iOffset = this->miNumOptions;
	long lNonHoleCounter = 0;

	NMDebugAI( << "extracting results ..." << endl);
	// -------------------------------------------------------------------for each feature
	for (int f=0; f < lNumCells; f++)
	{
		// special treatment for polygon holes
		if (hole->GetTuple1(f))
		{
			// write 0 values into the OPTx_VAL arrarys for holes
			for (int option=0; option < this->miNumOptions; option++)
				vValAr.at(option)->InsertValue(f, 0.0);

			opt_str->InsertValue(f, "");

			// leap frog
			continue;
		}

		// -----------------------------------------------------------------for each option
		sOptStr.clear();
		for (int option=0; option < this->miNumOptions; option++)
		{
			// format the column name (i.e. name of the decision variable
			// we are dealing with int this iteration)
			sColName = QString(tr("X_%1_%2")).arg(lNonHoleCounter).arg(option+1);

			//determine index for the pdVars array holding the values of the decision
			//variables (i.e. the appropriate column index - 1)
			iValIndex = this->mLp->GetNameIndex(sColName.toStdString(), false)-1;

			// get the actual value and put it into the right OPTx_VAL array (attribute)
			dVal = pdVars[iValIndex];
			vValAr.at(option)->InsertValue(f, dVal);

			// format sOptStr
			if (dVal > 0)
				sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
		}

		// trim sOptStr and write into array
		sOptStr = sOptStr.simplified();
		opt_str->InsertValue(f, sOptStr.toStdString().c_str());

		if (f % 100 == 0)
			NMDebug(<< ".");

		// increment the valid feature counter
		lNonHoleCounter++;
	}

	NMDebug(<< " finished!" << endl);

	// don't forget to add the new attributes to the
	// data set! (remember: availalbe attributes with the same name
	// are overriden!)
	ds->GetCellData()->AddArray(opt_str);
	for (int option=0; option < this->miNumOptions; option++)
		ds->GetCellData()->AddArray(vValAr.at(option));

	// call data set changed signal on layer to trigger
	// update chain for reflecting the changes elsewhere
	//this->mLayer->emitDataSetChanged();

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

int NMMosra::makeLp(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	if (this->mLp == 0)
		return 0;

	/*
	 * structure of the lp matrix  assuming that
	 * we've got two options (land uses)
	 *
	 * i : feature index
	 * r : land use index
	 *
	 * X_i_r: area share of land-use r allocated to feature i
	 * b_i  : binary conditional variable we need to model that
	 *        a feature has to be either completely covered by
	 *        any combination of the land use options or not at all
	 *
	 * note: row-0 contains the objective function and cell 0,0 contains
	 *       the objective function value; row-1 and beyond contain the
	 *       constraints of the model
	 *
	 *colindex:     0     1       2      3      4       5
	 *row-0           | X_0_1 | X_0_2 | b_0 | X_1_1 | X_1_2 | b_1 | ...
	 *row-1 (constraint #1: i.e. coefficients for decision variables)
	 *row-2 (constraint #2)
	 *row-3  etc.
	 *etc.
	 */

	// create the LP structure (i.e. columns (i.e. decision vars),
	// column names and column types; note: the default type is REAL
	this->mLp->MakeLp(0,this->mlLpCols);
	long colPos = 1;
	for (int of=0; of < this->mlNumOptFeat; ++of, ++colPos)
	{
		QString colname;
		for (int opt=1; opt <= this->miNumOptions; ++opt, ++colPos)
		{
			colname = QString(tr("X_%1_%2")).arg(of).arg(opt);
			this->mLp->SetColName(colPos, colname.toStdString());

			// set variable type for areal decision variables
			switch(this->meDVType)
			{
			case NMMosra::NM_MOSO_BINARY:
				this->mLp->SetBinary(colPos, true);
				break;
			case NMMosra::NM_MOSO_INT:
				this->mLp->SetInt(colPos, true);
				break;
			}
		}

		colname = QString(tr("b_%1")).arg(of);
		this->mLp->SetColName(colPos, colname.toStdString());
		this->mLp->SetBinary(colPos, true);
	}

//	// initiate the objective function to zero; note that all unspecified
//	// obj functions values are initiated to zero by lp_solve, so we
//	// just set one value and then we're done
//	double row = 0;
//	int colno = 1;
//
//	this->mLp->SetAddRowmode(true);
//	this->mLp->SetObjFnEx(1, &row, &colno);
//	this->mLp->SetAddRowmode(false);

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

vtkSmartPointer<vtkTable>
NMMosra::getDataSetAsTable()
{
	if (this->mDataSet == 0)
		return 0;

	vtkSmartPointer<vtkTable> tabCalc = vtkSmartPointer<vtkTable>::New();
	tabCalc->SetRowData(this->mDataSet->GetAttributes(vtkDataSet::CELL));
	return tabCalc;
}


void
NMMosra::perturbCriterion(const QString& criterion,
		float percent)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// check, whether the dataset has been set yet
	if (this->mDataSet == 0)
	{
		NMErr(ctxNMMosra, << "Input dataset has not been set!");
		return;
	}

	vtkSmartPointer<vtkTable> tabCalc = this->getDataSetAsTable();

	// need to distinguish between criterion and constraint
	// so, if we've got just one repetition, we assume to
	// have to deal with a constraint rather than a criterion
	if (this->mlReps == 1)
	{
		// constraint
		// check, whether we've more than one
		QStringList metaList = criterion.split(",");
		foreach(const QString& cri, metaList)
		{
			this->varyConstraint(cri, percent);
		}
	}
	else
	{
		// identify the criterion
		QStringList fields = this->mmslCriteria.value(criterion);
		if (fields.size() == 0)
		{
			NMDebugAI(<< "nothing to perturb for criterion '"
					<< criterion.toStdString() << "'" << endl);
			return;
		}

		// perturb field values by +/- percent of field value
		srand(time(0));
		foreach(const QString field, fields)
		{
			vtkDataArray* srcAr = vtkDataArray::SafeDownCast(
					tabCalc->GetColumnByName(field.toStdString().c_str()));

			for (int r=0; r < tabCalc->GetNumberOfRows(); ++r)
			{
				double inval = srcAr->GetTuple1(r);
				double uncval = inval * ((rand() % ((int)percent+1))/100.0);
				double newval;
				if (rand() % 2)
					newval = inval + uncval;
				else
					newval = inval - uncval;

				srcAr->SetTuple1(r, newval);
			}
		}
	}

	NMDebugCtx(ctxNMMosra, << "done!");
}

void
NMMosra::varyConstraint(const QString& constraint, float percent)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// identifying the constraint
	QStringList identlist = constraint.split(":");
	if (identlist.size() < 3)
	{
		NMErr(ctxNMMosra, << "Invalid pertubation item specified: "
				<< constraint.toStdString() << std::endl);
		NMDebugCtx(ctxNMMosra, << "done!");

		//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
		//e.setMsg("Invalid pertubation item specified!");
		//throw e;

		return;
	}

	// read information
	QString type = identlist.at(0);
	QString label = identlist.at(1);
	QString use = identlist.at(2);
	QString zone = "";
	if (identlist.size() == 4)
		zone = identlist.at(3);

	if (type.compare("CRI", Qt::CaseInsensitive) == 0)
	{
		// since we could have more than one constraint with the same
		// label, we just go through and check the use
		QMap<QString, QStringList> lufm;
		QMultiMap<QString, QMap<QString, QStringList> >::iterator it =
				mmslCriCons.begin();
		while(it != mmslCriCons.end())
		{
			if (it.key().compare(label) == 0)
			{
				if (it.value().find(use) != it.value().end())
				{
					lufm = mmslCriCons.take(it.key());
					break;
				}
			}

			++it;
		}


		if (lufm.isEmpty())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setMsg("Land use field map is empty!");
			//throw e;
			return;
		}

		// get the field list + the operator + cap
		QStringList fieldvaluelist = lufm.value(use);
		if (fieldvaluelist.empty())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setMsg("field value list is empty!");
			//throw e;
			return;
		}

		fieldvaluelist = lufm.take(use);

		bool bok;
		double cap = fieldvaluelist.last().toDouble(&bok);
		if (!bok)
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setMsg("Land use field map is empty!");
			//throw e;
			return;
		}

		NMDebugAI(<< "old value for " << constraint.toStdString()
				<< " = " << cap << endl);


		// adjust the cap value by adding the percentage of change
		cap += (cap * percent/100.0);

		NMDebugAI(<< "new value for " << constraint.toStdString()
				<< " = " << cap << endl);

		// vary the constraint value (cap)
		QString strVal = QString("%1").arg(cap);
		fieldvaluelist.replace(fieldvaluelist.size()-1, strVal);

		// put the multi-map back together again
		lufm.insert(use, fieldvaluelist);
		mmslCriCons.insert(label, lufm);

	}
	else if (type.compare("AREA", Qt::CaseInsensitive) == 0)
	{
		//ToDo
	}
	else if (type.compare("OBJ", Qt::CaseInsensitive) == 0)
	{
		//ToDo
	}

	NMDebugCtx(ctxNMMosra, << "done!");
}

int NMMosra::addObjFn(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// get the hole array
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());


	// some vars we need

	// init some mem for the row and column objects of the
	// problem
	double* pdRow = new double[this->mlNumArealDVar];
	int* piColno = new int[this->mlNumArealDVar];
	int* plFieldIndices = new int[this->miNumOptions];


	QString s1Maxmin = this->mmslObjectives.begin().value().at(0).left(3);
	QString sxMaxmin;
	QString sFieldName;
	long lId, lNumCells = hole->GetNumberOfTuples();
	double dFeatID, dCoeff, dWeight = 1;
	bool convOK;

	// ------------------------------------------------------------------for each objective
	// set coefficients of areal decision variables
	QMap<QString, QStringList>::const_iterator it =
			this->mmslObjectives.constBegin();

	unsigned int objcount = 0;
	for (; it != this->mmslObjectives.constEnd(); ++it, ++objcount)
	{
		// get field indices for the performance indicators
		NMDebugAI( << "objective '" << it.key().toStdString() << "' ..." << endl);

		NMDebugAI( << "criterion / option: <performance indi field> <field index> ... " << endl);
		for (int option=0; option < this->miNumOptions; option++)
		{
			// field name of performance indicator for
			// current criterion and option
			QString sCriterion = it.key();
			QStringList criFieldList = this->mmslCriteria.value(sCriterion);
			QString sField = "";
			if (option <= criFieldList.size()-1)
				sField = criFieldList.at(option);

			if (dsAttr->GetArray(sField.toStdString().c_str(), *(plFieldIndices + option)) == NULL)
			{
				NMErr(ctxNMMosra, << "failed to get performance indicator '"
						<< sField.toStdString() << "' for option '"
						<< this->mslOptions.at(option).toStdString() << "'!");

				// free some memory
				delete[] pdRow;
				delete[] piColno;
				delete[] plFieldIndices;

				NMDebugCtx(ctxNMMosra, << "done!");
				return 0;
			}

			NMDebugAI( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
					<< ": " << sField.toStdString() << " " << plFieldIndices[option] << endl);
		}

		// maximise or minimise ?
		sxMaxmin = it.value().at(0).left(3);

		// align optimisation task with that of objective 1
		bool bMalMinusEins = false;
		if (s1Maxmin.compare(sxMaxmin, Qt::CaseInsensitive) != 0)
			bMalMinusEins = true;

		// get the weight for this objective
		dWeight = it.value().at(1).toDouble(&convOK);
		NMDebugAI( << "sxMaxmin: " << sxMaxmin.toStdString() << " | adjust task: " << bMalMinusEins << " | weight: " << dWeight << endl);

		NMDebugAI( << "processing individual features now" << endl);
		// --------------------------------------------------for each feature
		long colPos = 1;
		long arpos = 0;
		for (int f=0; f < lNumCells; ++f)
		{
			// leap over holes
			if (hole->GetTuple1(f))
				continue;

			// ----------------------------------------------for each option
			for (int option=0; option < this->miNumOptions; option++, arpos++, colPos++)
			{
				// get performance indicator for current feature and
				// criterion and option
				vtkDataArray* da = dsAttr->GetArray(plFieldIndices[option]);
				dCoeff = da->GetTuple1(f);

				// DEBUG
				//if (f < 100)
				//	NMDebugAI( << lCounter << " : " << this->mslOptions.at(option).toStdString()
				//			<< " : " << da->GetName() << " : " << dCoeff << endl);

				// adjust according to MinMax of objective 1
				dCoeff = bMalMinusEins ? dCoeff * -1 : dCoeff;

				// apply objective weight if applicable
				if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
					dCoeff *= dWeight;

				// do we have binary decision variables?
				double area = areas->GetTuple1(f);
				QString sArea = QString(tr("%1")).arg(area, 0, 'g');
				bool bok;
				switch(this->meDVType)
				{
				case NM_MOSO_BINARY:
					dCoeff = vtkMath::Floor(dCoeff);
					dCoeff = (int)dCoeff * (int)sArea.toDouble(&bok);
					break;
				case NM_MOSO_INT:
					dCoeff = vtkMath::Floor(dCoeff);
					break;
				}


				// write coefficient into row array
				if (objcount)
					pdRow[arpos] += dCoeff;
				else
					pdRow[arpos] = dCoeff;
				piColno[arpos] = colPos;

				//NMDebug(<< this->mLp->GetColName(colPos) << "=" << pdRow[arpos] << "  ");

			}

			// increment the column number counter to the next areal decision variable
			// (leaping one binary deciscion var associated with each areal decision variable)
			colPos++;

			if (f % 500 == 0)
				NMDebug(<< ".");
		}

		NMDebug(<< " finished!" << endl);
	}

	int ncols = this->mLp->GetNColumns();
	//NMDebugAI( << "num cols: " << ncols << " | NumArealDVar: " << this->mlNumArealDVar << endl);


	// set the objective function
	this->mLp->SetAddRowmode(true);
	this->mLp->SetObjFnEx(this->mlNumArealDVar, pdRow, piColno);
	this->mLp->SetAddRowmode(false);

	// set the optimisation task
	if (s1Maxmin.compare(tr("min"), Qt::CaseInsensitive) == 0)
		this->mLp->SetMinim();
	else
		this->mLp->SetMaxim();

	// free some memory
	delete[] pdRow;
	delete[] piColno;
	delete[] plFieldIndices;

	NMDebugCtx(ctxNMMosra, << "done!");

	return 1;
}

int NMMosra::addObjCons(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// get data set attributes
	// get the hole array
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
	long lNumCells = hole->GetNumberOfTuples();

	// create vectors holding the constraint types
	// and objective constraint labels
	std::vector<unsigned int> vnConsType;
	std::vector<QString> vsObjConsLabel;
	QMap<QString, QStringList>::const_iterator it = this->mmslObjCons.constBegin();

	NMDebugAI( << "reading props" << endl);
	for (; it != this->mmslObjCons.constEnd(); it++)
	{
		// add the operator
		if (it.value().at(1).compare(tr("<=")) == 0)
			vnConsType.push_back(1);
		else if (it.value().at(1).compare(tr(">=")) == 0)
			vnConsType.push_back(2);
		else // =
			vnConsType.push_back(3);

		// add the key i.e. objective as label
		vsObjConsLabel.push_back(it.key());

		NMDebugAI( << it.key().toStdString() << ": " << it.value().join(tr(" ")).toStdString() << endl);
	}

	// allocate the required memory
	double* pdRow = new double[this->mlNumArealDVar];
	int* piColno = new int[this->mlNumArealDVar];
	int* piFieldIndices = new int[this->miNumOptions];

	// some more vars
	QString sFieldName;
	QString sObjCri;
	double dConsVal;
	double dCoeff;

	// turn on row mode
	this->mLp->SetAddRowmode(true);
	long lRowCounter = this->mLp->GetNRows();

	it = this->mmslObjCons.constBegin();
	// ------------------------------------------------for each objective
	for (int obj=0; it != this->mmslObjCons.constEnd(); it++, obj++)
	{
		// get the constraint value
		bool bConvOK;
		double dVal =
		dVal = it.value().at(2).toDouble(&bConvOK);
		if (bConvOK)
			dConsVal = dVal;
		else
			NMErr(ctxNMMosra, << "couldn't convert " << it.value().at(2).toStdString() << " into a double!" << endl);

		NMDebugAI( << it.key().toStdString() << endl);

		NMDebugAI( << "criterion / option: <performance indi field> <field index> ... " << endl);
		for (int option=0; option < this->miNumOptions; option++)
		{
			// field name of performance indicator for
			// current criterion and option
			QString sCriterion = it.value().at(0);
			QString sField = this->mmslCriteria.value(sCriterion).at(option);
			if (dsAttr->GetArray(sField.toStdString().c_str(), *(piFieldIndices + option)) == NULL)
			{
				NMErr(ctxNMMosra, << "failed to get performance indicator '"
						<< sField.toStdString() << "'!");

				// free some memory
				delete[] pdRow;
				delete[] piColno;
				delete[] piFieldIndices;

				NMDebugCtx(ctxNMMosra, << "done!");
				return 0;
			}

			NMDebugAI( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
					<< ": " << sField.toStdString() << " " << piFieldIndices[option] << endl);
		}

		// ------------------------------------------------------------for each spatial alternative
		long lCounter = 0;
		long colpos = 1;
		for (int f=0; f < lNumCells; f++)
		{
			// leap frog holes in polygons
			if (hole->GetTuple1(f))
				continue;

			for (int option=0; option < this->miNumOptions; option++)
			{
				vtkDataArray* da = dsAttr->GetArray(piFieldIndices[option]);
				dCoeff = da->GetTuple1(f);

				// do we have binary decision variables?
				double area = areas->GetTuple1(f);
				QString sArea = QString(tr("%1")).arg(area, 0, 'g');
				bool bok;
				switch(this->meDVType)
				{
				case NM_MOSO_BINARY:
					dCoeff = vtkMath::Floor(dCoeff);
					dCoeff = (int)dCoeff * (int)sArea.toDouble(&bok);
					break;
				case NM_MOSO_INT:
					dCoeff = vtkMath::Floor(dCoeff);
					break;
				}

				pdRow[lCounter] = dCoeff;
				piColno[lCounter] = colpos;

				// increment the counter
				lCounter++;
				colpos++;
			}

			// leap frog the binary decision variable for this feature
			colpos++;

			if (f % 100 == 0)
				NMDebug(<< ".");
		}

		NMDebug(<< " finished!" << endl);

		// add the constraint to the matrix
		this->mLp->AddConstraintEx(this->mlNumArealDVar, pdRow, piColno, vnConsType.at(obj), dConsVal);

		// increment the row counter
		lRowCounter++;

		// add the label for this constraint
		this->mLp->SetRowName(lRowCounter, vsObjConsLabel.at(obj).toStdString());
	}


	this->mLp->SetAddRowmode(false);

	// free some memory
	delete[] pdRow;
	delete[] piColno;
	delete[] piFieldIndices;

	NMDebugCtx(ctxNMMosra, << "done!");

	return 1;
}

int NMMosra::addExplicitAreaCons(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// get the hole array
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());

	std::vector<QString> vsConsLabel;
	std::vector<int> vnZoneLength;
	std::vector<QString> vsZoneField;
	std::vector<unsigned int> vnOptionIndex;
	std::vector<unsigned int> vnConsType;
	std::vector<double> vdRHS;
	long lNumCells = hole->GetNumberOfTuples();
	double dUserVal;
	double dUserArea = 0.0;
	double dAreaTotal = this->mdAreaTotal;
	double dAreaSelected = this->mdAreaSelected <= 0 ? this->mdAreaTotal : this->mdAreaSelected;

	QMap<QString, QStringList>::const_iterator it =
			this->mmslAreaCons.constBegin();

	NMDebugAI( << this->mmslAreaCons.size() << " cons to process" << endl);

	// ----------------------------------------------------for each constraint
	for(int iConsCounter=1; it != this->mmslAreaCons.constEnd(); it++, iConsCounter++)
	{
		NMDebugAI( << it.key().toStdString() << " "
				<< it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

		// set the constraint label
		QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
		vsConsLabel.push_back(sConsLabel);
		//this->msvAreaConsLabel.push_back(sConsLabel);

		// look for a 'zoned' constraint
		QString opt;
		QString zone;
		QStringList optzones;
		if (it.value().at(0).contains(tr(":"), Qt::CaseInsensitive))
		{
			optzones = it.value().at(0).split(tr(":"), QString::SkipEmptyParts);
			opt = optzones.at(0);
			zone = optzones.at(1);
			vnZoneLength.push_back(this->mmslZoneLength.find(zone).value().find(opt).value());
		}
		else
		{
			opt = it.value().at(0);
			zone = "";
		}
		vsZoneField.push_back(zone);

		// set the option index
		unsigned int idx = this->mslOptions.indexOf(
				QRegExp(opt, Qt::CaseInsensitive, QRegExp::FixedString));
		vnOptionIndex.push_back(idx);
		NMDebugAI( << "option index for '" << opt.toStdString() << "' = " << idx + 1 << endl);

		// set the constraint type
		if (it.value().at(1).compare(tr("<=")) == 0)
			vnConsType.push_back(1);
		else if (it.value().at(1).compare(tr(">=")) == 0)
			vnConsType.push_back(2);
		else
			vnConsType.push_back(3);
		NMDebugAI( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);

		// set the user value
		bool bConvOK;
		dUserVal = it.value().at(2).toDouble(&bConvOK);
		double dtval = this->convertAreaUnits(dUserVal, it.value().at(3), optzones);
		NMDebugAI( << "dUserVal (" << dUserVal << ") as '" << it.value().at(3).toStdString() << "' = " << dtval << endl);

		vdRHS.push_back(dtval);
		dUserArea += dtval;
	}

	// todo: we need to account for selected areas at one stage

//	if (dUserArea > dAreaSelected)
//	{
//		NMErr(ctxNMMosra, << "The specified areal constraints are "
//				<< "inconsistent with the available area!");
//		return 0;
//	}

	double* pdRow = new double[this->mlNumOptFeat];
	int* piColno = new int[this->mlNumOptFeat];

	double* pdRow2;
	int* piColno2;

	long lId;
	double dCoeff;

	// set add row mode
	this->mLp->SetAddRowmode(true);
	long lRowCounter = this->mLp->GetNRows();

	// calc the offset required to jump from one feature-option-column to the next
	// one; since we've got one binary conditional decision variable for each feature
	// we've got to add 1 to the offset;
	int iOffset = this->miNumOptions + 1;

	it = this->mmslAreaCons.constBegin();
	// ------------------------------------------------ for each constraint
	for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r)
	{
		NMDebugAI( << vsConsLabel.at(r).toStdString() << " - adding constraint" << endl);

		// array defining zones for this constraints
		bool bZoneCons = false;
		vtkStringArray* zoneAr;
		QString inLabel = vsConsLabel.at(r);
		QString outLabel;

		if (!vsZoneField.at(r).isEmpty())
		{
			pdRow2 = new double[this->mlNumOptFeat];
			piColno2 = new int[this->mlNumOptFeat];

			zoneAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(vsZoneField.at(r).toStdString().c_str()));
			bZoneCons = true;

			inLabel = QString(tr("%1_in")).arg(vsConsLabel.at(r));
			outLabel = QString(tr("%1_out")).arg(vsConsLabel.at(r));
		}

		// determine the initial offset for the current option (i.e. the first column number
		// of the lp-matrix); since the option index is 0-based, but the columns are 1-based
		// we've got to add 1!
		long lCounter = vnOptionIndex.at(r) + 1;

		long nonHoleCounter = 0;
		// ---------------------------------------------for each feature
		for (int f=0; f < lNumCells; f++)
		{
			// skip holes
			if (hole->GetTuple1(f))
				continue;

			dCoeff = areas->GetTuple1(f);
			QString sConsVal = QString(tr("%1")).arg(dCoeff, 0, 'g');

			// set coefficients depending on whether we've got a zoned constraint or not
			if (bZoneCons)
			{
				// set coefficients for zone polygons
				if (zoneAr->GetValue(f).find(this->mslOptions.at(vnOptionIndex.at(r)).toStdString())
						!= std::string::npos)
				{
					// set the coefficient
					switch(this->meDVType)
					{
					case NMMosra::NM_MOSO_BINARY:
						bool bok;
						pdRow[nonHoleCounter] = sConsVal.toDouble(&bok);
						break;
					default: // i.e. for all other DV  types (real, integer)
						pdRow[nonHoleCounter] = 1;
						break;
					}
					pdRow2[nonHoleCounter] = 0;

					// set the column number
					piColno[nonHoleCounter] = lCounter;
					piColno2[nonHoleCounter] = lCounter;
				}
				else	// set the coefficients for non-zone polygons
				{
					// set the coefficient
					switch(this->meDVType)
					{
					case NMMosra::NM_MOSO_BINARY:
						pdRow2[nonHoleCounter] = ::atof(sConsVal.toStdString().c_str());
						break;
					default: // i.e. for all other DV  types (real, integer)
						pdRow2[nonHoleCounter] = 1;
						break;
					}
					pdRow[nonHoleCounter] = 0;

					// set the column number
					piColno[nonHoleCounter] = lCounter;
					piColno2[nonHoleCounter] = lCounter;
				}
			}
			else // set the coefficients for a non-zone constraint
			{
				// set the coefficient
				switch(this->meDVType)
				{
				case NMMosra::NM_MOSO_BINARY:
					pdRow[nonHoleCounter] = ::atof(sConsVal.toStdString().c_str());
					break;
				default: // i.e. for all other DV  types (real, integer)
					pdRow[nonHoleCounter] = 1;
					break;
				}

				// set the column number
				piColno[nonHoleCounter] = lCounter;
			}

			// set the counter to the next appropriate column number
			// for this option
			lCounter += iOffset;

			if (f % 100 == 0)
				NMDebug(<< ".");

			// increment the counter for valid cells (i.e. valid spatial opt features
			++nonHoleCounter;
		}

		NMDebug(<< " finished!" << endl);

		// add the constraint
		this->mLp->AddConstraintEx(this->mlNumOptFeat,
				pdRow, piColno, vnConsType.at(r),
				vdRHS.at(r));

		// increment the row counter
		++lRowCounter;

		// assign a label for this constraint and store label for
		// retrieval of post-optimisation constraint values
		this->mLp->SetRowName(lRowCounter, inLabel.toStdString());
		this->mmslAreaConsLabel.append(inLabel);

		// add the corresponding out-constraint, if we've got zoning
		if (bZoneCons)
		{
//			this->mLp->AddConstraintEx(this->mlNumOptFeat,
//					pdRow2, piColno2, 3, 0);
//
//			++lRowCounter;
//
//			this->mLp->SetRowName(lRowCounter, outLabel.toStdString());
//			this->mmslAreaConsLabel.append(outLabel);

			// free memory
			delete[] pdRow2;
			delete[] piColno2;
		}
	}

	// free memory
	delete[] pdRow;
	delete[] piColno;

	// turn off rowmode
	this->mLp->SetAddRowmode(false);

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

int NMMosra::addImplicitAreaCons(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// allocate memory for arrays
	// we cover each feature separately, so we need miNumOptions coefficients
	// plus the binary conditional to realise the either zero or all area
	// constraint for each feature
	double* pdRow = new double[this->miNumOptions + 1];
	int* piColno = new int[this->miNumOptions + 1];

	// get the hole array
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());

	long lNumCells = hole->GetNumberOfTuples();
	long lId;
	double dConsVal;
	double dFeatID;
	double dVal;

	// set add_row_mode for adding constraints
	this->mLp->SetAddRowmode(true);

	long lRowCounter = this->mLp->GetNRows();
	long colpos = 1;
	// --------------------------------------------for each feature
	for (int f=0; f < lNumCells; f++)
	{
		// leap over holes!
		if (hole->GetTuple1(f))
			continue;

		// get the area of the current feature
		dConsVal = areas->GetTuple1(f);

		// round value when dealing with integer decision variables
		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dConsVal = vtkMath::Floor(dConsVal);

		// can't really comprehend this dirty hack again
		// but it its meaning in the original LUMASS version ...
		QString sConsVal = QString(tr("%1")).arg(dConsVal, 0, 'g');

		//-----------------------------------------for each option
		for (int option=0; option < this->miNumOptions; option++, colpos++)
		{
			// set the coefficient
			switch(this->meDVType)
			{
			case NMMosra::NM_MOSO_BINARY:
				pdRow[option] = ::atof(sConsVal.toStdString().c_str());
				break;
			default: // i.e. for all other DV  types (real, integer)
				pdRow[option] = 1;
				break;
			}

			// set the column number
			piColno[option] = colpos;

			//NMDebugAI( << "feat cons: option | colps = " << option << " | " << lCounter+1 << endl);

		}

		// ......................................................................................
		// add the first constraint: SUM(x_i_r) - A_i * b_i >= 0

		// set the coefficient for the binary decision variable for the current feature
		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;
		piColno[this->miNumOptions] = colpos;

		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
				piColno, 2, 0);

		// increment row (constraint) counter
		lRowCounter++;

		// label the currently added constraint
		QString sRowName = QString(tr("Feature_%1a")).arg(f);
		this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

		// ......................................................................................
		// add the second constraint: SUM(x_i_r) - A_i * b_i <= 0
		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;

		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
				piColno, 1, 0);
		lRowCounter++;

		sRowName = QString(tr("Feature_%1b")).arg(f);
		this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

		// ........................................................................................
		// column position counter
		colpos++;

		if (f % 500 == 0)
			NMDebug(<< ".");
	}
	NMDebug(<< " finished!" << endl);

	// turn off row mode
	this->mLp->SetAddRowmode(false);

	// free memory
	delete[] pdRow;
	delete[] piColno;

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

int NMMosra::addCriCons(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// extract relevant information for building constraints
	// this could have been incorporated into the actual "value loop"
	// but it is much more readable like this, at least to me, and
	// it doesn't hurt the performance too much because there are
	// usually not too many constraints

	QVector<QString> vLabels;
	// one constraint per land use (or 'total'=all land uses)
	QVector<QString> vLandUses;
	// vector of zones (may be and empty string though, if no zone was specified for a particular
	// constraint
	QVector<QString> vZones;
	// vector of vector of field names per constraint (only when 'total' is given more than one)
	QVector<QVector<QString> > vvFieldNames;
	// vector of vector of land use indices (i.e. position in mslOptions and thus
	// relative position within the optimisation matrix per feature)
	QVector<QVector<unsigned int> > vvIdxLandUses;
	// vector of comparison operator-id (one per constraint)
	QVector<int> vOperators;
	// vector of values of the right hand side
	QVector<double> vRHS;

	QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit;
	QMap<QString, QStringList>::const_iterator criit;
	for (crilabit = this->mmslCriCons.begin(); crilabit != this->mmslCriCons.end(); ++crilabit)
	{
		QMap<QString, QStringList> luFieldList = crilabit.value();
		for (criit = luFieldList.begin(); criit != luFieldList.end(); ++criit)
		{
			QString compOp = criit.value().at(criit.value().size()-2);
			QString compTypeLabel;
			if (compOp.compare(tr("<=")) == 0)
			{
				vOperators.push_back(1);
				compTypeLabel = "upper";
			}
			else if (compOp.compare(tr(">=")) == 0)
			{
				vOperators.push_back(2);
				compTypeLabel = "lower";
			}
			else if (compOp.compare(tr("=")) == 0)
			{
				vOperators.push_back(3);
				compTypeLabel = "equals";
			}
			else
			{
				NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: " << crilabit.key().toStdString()
						<< "_" << criit.key().toStdString() <<	": invalid comparison operator '"
						<< compOp.toStdString() << "'!");
				return 0;
			}

			// construct the label for each constraint
			QString label = crilabit.key() + QString(tr("_%1_%2")).arg(criit.key()).arg(compTypeLabel);
			vLabels.push_back(label);
			vLandUses.push_back(criit.key());

			bool convertable;
			QString sRhs = criit.value().last();
			double rhs = sRhs.toDouble(&convertable);
			if (!convertable)
			{
				NMErr(ctxNMMosra, << "CRITERIA_CONSTRAINTS: " << label.toStdString() << ": invalid value '"
						<< sRhs.toStdString() << "'!");
				return 0;
			}
			vRHS.push_back(rhs);

			// ......................................................................
			// extract field names and land use indices

			// first of all, account for zoning
			QStringList zonespec;
			QString opt;
			QString zone;

			if (criit.key().contains(tr(":"), Qt::CaseInsensitive))
			{
				zonespec = criit.key().split(tr(":"), QString::SkipEmptyParts);
				opt = zonespec.at(0);
				zone = zonespec.at(1);
			}
			else
			{
				opt = criit.key();
				zone = "";
			}
			vZones.push_back(zone);

			// now look for resource indices and criterion field names
			QVector<QString> vCriterionFieldNames;
			QVector<unsigned int> vCriterionLandUseIdx;
			QStringList fieldList = criit.value();
			if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
			{
				// look through m_iNumOptions fields
				for (int f=0; f < this->miNumOptions; ++f)
				{
					vCriterionFieldNames.push_back(fieldList.at(f));
					vCriterionLandUseIdx.push_back(f);
				}
			}
			else
			{
				vCriterionLandUseIdx.push_back(
						this->mslOptions.indexOf(
								QRegExp(opt, Qt::CaseInsensitive, QRegExp::FixedString)));
				vCriterionFieldNames.push_back(fieldList.at(0));
			}
			vvFieldNames.push_back(vCriterionFieldNames);
			vvIdxLandUses.push_back(vCriterionLandUseIdx);
		}
	}

	// get the hole array
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
	long lNumCells = hole->GetNumberOfTuples();


	// iterate over constraints and process them one at a time
	// (or should we process them while looping over all features)?

	this->mLp->SetAddRowmode(true);
	long lRowCounter = this->mLp->GetNRows();

	for (int labelidx = 0; labelidx < vLabels.size(); ++labelidx)
	{
		NMDebugAI(<< "preparing constraint " << vLabels[labelidx].toStdString() << endl);

		// get the performance indicator fields, land use indices
		// and allocate the constraint buffers
		QVector<unsigned int> landUseIdxs = vvIdxLandUses[labelidx];
		QVector<QString> fieldNames = vvFieldNames[labelidx];
		int numCriOptions = fieldNames.size();

		double* pdRow = new double[numCriOptions * this->mlNumOptFeat];
		int* piColno = new int[numCriOptions * this->mlNumOptFeat];

		long arpos = 0;
		long colPos = 1;
		for (long f=0; f < lNumCells; ++f)
		{
			//leap over holes
			if (hole->GetTuple1(f))
				continue;

			for(int i=0; i < numCriOptions; ++i, ++arpos)
			{
				int optIdx = landUseIdxs[i];

				vtkDataArray* performanceIndicator = dsAttr->GetArray(
						fieldNames[i].toStdString().c_str());

				double coeff = 0.0;
				bool bAddCoeff = true;

				// check whether, we've got zoning and whether the criterion constraint
				// for the current land use option shall be restricted to this zone
				if (!vZones.at(labelidx).isEmpty())
				{
					vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(
							dsAttr->GetAbstractArray(vZones.at(labelidx).toStdString().c_str()));
					if (zoneAr->GetValue(f).find(this->mslOptions.at(optIdx).toStdString()) == std::string::npos)
						bAddCoeff = false;
				}

				if (bAddCoeff)
				{
					switch(this->meDVType)
					{
					case NMMosra::NM_MOSO_BINARY:
						coeff = areas->GetTuple1(f) * performanceIndicator->GetTuple1(f);
						break;
					default:
						coeff = performanceIndicator->GetTuple1(f);
						break;
					}
				}

				pdRow[arpos] = coeff;
				piColno[arpos] = colPos + optIdx;

				if (f % 200 == 0)
					NMDebug(<< ".");
			}

			// jump to the first option of the next feature (keep in mind that we've got
			// one binary decision var (i.e. column) per feature, hence the +1
			colPos += this->miNumOptions + 1;
		}
		NMDebug(<< " finished!" << std::endl);

		// add constraint
		NMDebugAI(<< "adding constraint to LP ..." << std::endl);
		this->mLp->AddConstraintEx(numCriOptions * this->mlNumOptFeat,
				pdRow, piColno, vOperators[labelidx], vRHS[labelidx]);

		// increment the row counter
		++lRowCounter;


		//  set the constraint label
		NMDebugAI(<< "labeling constraint ..." << std::endl);
		this->mLp->SetRowName(lRowCounter, vLabels[labelidx].toStdString().c_str());

		delete[] pdRow;
		delete[] piColno;

	}

	// turn off rowmode
	this->mLp->SetAddRowmode(false);

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

vtkSmartPointer<vtkTable> NMMosra::sumResults()
{
	NMDebugCtx(ctxNMMosra, << "...");

	NMDebugAI(<< "getting input arrays (attributes) ..." << std::endl);
	// get hold of the input attributes we need
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* holeAr = dsAttr->GetArray("nm_hole");
	vtkDataArray* areaAr = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
	vtkStringArray* luAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(
			this->msLandUseField.toStdString().c_str()));

	vtkStringArray* optStrAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray("OPT_STR"));
	if (optStrAr == 0)
	{
		NMErr(ctxNMMosra, << "failed to fetch result array 'OPT_STR'!");
		return 0;
	}

	QList<vtkDataArray*> optValArs;
	for (int option=0; option < this->miNumOptions; ++option)
	{
		QString arName = QString(tr("OPT%1_VAL")).arg(option+1);
		vtkDataArray* optvalar = dsAttr->GetArray(arName.toStdString().c_str());
		if (optvalar == 0)
		{
			NMErr(ctxNMMosra, << "failed to fetch result array '"
					<< arName.toStdString().c_str() << "'!");
			return 0;
		}
		optValArs.append(optvalar);
	}

	/* create the vtkTable, which will hold the summarised optimisation results
	 * - the column sequence is area, criteria, constraints and for each of those categories we've
	 *   got in turn four columns: current (CUR), optimised (OPT), difference (DIFF), relative
	 *   difference (REL) (i.e. percent);
	 * - the row sequence equals the sequence of land use options (resources) as specified by
	 *   the user by the OPTIONS specifier in the parameter file (*.los)
	 *
	 *  Resource   | Area_CUR | Area_OPT | Area_DIFF | Area_REL | CRI1_CUR | CRI1_OPT | ... | CONS1_CUR | CONS1_OPT ...
	 *  -----------+----------+--------------------------------------------------------------------------------------
	 *  resource_1 |          |
	 *  resource_2 |		  |
	 *  ...        |
	 *  resource_n |
//	 *  mixed      |
	 *  total      |
	 *
	 */

	NMDebugAI(<< "creating the results table..." << std::endl);

	// determine the number of rows and columns
	//int resNumCols = (this->mmslCriteria.size() + this->mmslCriCons.size()) * 3 +1;

	// we sum over each resource option (land use), all mixed landuses (mixed) and
	// the overall total
	//int resNumRows = this->miNumOptions + 2; // with mixed
	int resNumRows = this->miNumOptions + 1;   // without mixed
	long ncells = holeAr->GetNumberOfTuples();

	vtkSmartPointer<vtkTable> restab = vtkSmartPointer<vtkTable>::New();
	restab->SetNumberOfRows(resNumRows);

	// add the row header array (land use options) + total
	vtkSmartPointer<vtkStringArray> rowheads = vtkSmartPointer<vtkStringArray>::New();
	rowheads->SetName("Resource");
	rowheads->SetNumberOfComponents(1);
	rowheads->SetNumberOfTuples(resNumRows);
	for (int r=0; r < this->miNumOptions; ++r)
        rowheads->SetValue(r, this->mslOptions.at(r).toLatin1());
	//rowheads->SetValue(resNumRows-2, "Mixed");
	rowheads->SetValue(resNumRows-1, "Total");
	restab->AddColumn(rowheads);

	// the series to the table

	QStringList colsuffix;
	colsuffix << "CUR" << "OPT" << "DIFF" << "REL";

	// add the area results
	for (int s=0; s < colsuffix.size(); ++s)
	{
		QString colname = QString(tr("%1_%2")).arg(this->msAreaField).
				arg(colsuffix.at(s));
		vtkSmartPointer<vtkDoubleArray> aar = vtkSmartPointer<vtkDoubleArray>::New();
        aar->SetName(colname.toLatin1());
		aar->SetNumberOfComponents(1);
		aar->SetNumberOfTuples(resNumRows);
		aar->FillComponent(0, 0);

		restab->AddColumn(aar);
	}


	// add the criterion related columns
	QMap<QString, QStringList>::const_iterator criit = this->mmslCriteria.constBegin();
	for (; criit != this->mmslCriteria.constEnd(); ++criit)
	{
		for (int s=0; s < colsuffix.size(); ++s)
		{
			QString colname = QString(tr("e_%1_%2")).arg(criit.key()).arg(colsuffix[s]);
			vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
            da->SetName(colname.toLatin1());
			da->SetNumberOfComponents(1);
			da->SetNumberOfTuples(resNumRows);
			da->FillComponent(0, 0);

			restab->AddColumn(da);
		}
	}

	// add columns for attribute constraints
	QMultiMap< QString, QMap< QString, QStringList > >::const_iterator constrit =
			this->mmslCriCons.constBegin();
	for (; constrit != this->mmslCriCons.constEnd(); ++constrit)
	{
		criit = constrit.value().constBegin();
		for (; criit != constrit.value().constEnd(); ++criit)
		{
			for (int s=0; s < colsuffix.size(); ++s)
			{
				QString colname = QString(tr("c_%1%2_%3")).arg(constrit.key()).
						arg(criit.key()).arg(colsuffix.at(s));
				vtkSmartPointer<vtkDoubleArray> daa = vtkSmartPointer<vtkDoubleArray>::New();
                daa->SetName(colname.toLatin1());
				daa->SetNumberOfComponents(1);
				daa->SetNumberOfTuples(resNumRows);
				daa->FillComponent(0, 0);

				restab->AddColumn(daa);
			}
		}
	}

//	NMDebugAI(<< "dump table structure...." << std::endl);
//	restab->Dump(10, resNumRows);

	NMDebugAI(<< "summarising results ..." << std::endl << std::endl);


	int ind = nmlog::nmindent + 1;
	int ind2 = ind+1;

	// loop over the data set and sequentially process (update) all target columns
	// only after the final iteration, the table holds correct values;
	for (long cell=0; cell < ncells; ++cell)
	{
		// don't tumble into holes
		if (holeAr->GetTuple1(cell))
			continue;

		// read the current and optimised resource
		QString curResource = (const char*)luAr->GetValue(cell);
		// make sure, we're not fooled by any leading or trailing white spaces
		curResource = curResource.simplified();
		QString optResource = (const char*)optStrAr->GetValue(cell);
		QStringList optResList = optResource.split(tr(" "), QString::SkipEmptyParts);

		// DEBUG
//		NMDebugInd(ind, << "curResource: " << curResource.toStdString() << endl);
//		NMDebugInd(ind, << "optResource: " << optResource.toStdString() << endl);
//		NMDebugInd(ind, << "optResList: ");
//		for (int r=0; r < optResList.size(); ++r)
//		{
//			NMDebug(<< "-" << optResList.at(r).toStdString() << "-");
//		}
//		NMDebug(<< endl << endl);

		for (int option=0; option < this->miNumOptions; ++option)
		{
			// common vars
			vtkDataArray* evalAr;
			QString evalField;
			double performanceValue;

			// handle current land uses
			double curArea;
			double curaccumArea;
			if (curResource.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
			{
//				NMDebugInd(ind, << "evaluating current resource option '" <<
//						this->mslOptions.at(option).toStdString() << "' ..." << endl);

				// process area
				curArea = areaAr->GetTuple1(cell);
				curaccumArea = curArea + restab->GetValue(option, 1).ToDouble();
				restab->SetValue(option,1, vtkVariant(curaccumArea));

//				NMDebugInd(ind2, << "curArea: " << curArea << endl);
//				NMDebugInd(ind2, << "curaccumArea: " << curaccumArea << endl);

				// update criteria stats
				QMap<QString, QStringList>::const_iterator evalit =
						this->mmslEvalFields.constBegin();
				int coloffset = 1;
				for (; evalit != this->mmslEvalFields.constEnd(); ++evalit, ++coloffset)
				{
					evalField = evalit.value().at(option);

//					NMDebugInd(ind2, << "checking criterion: " << evalit.key().toStdString() << endl);

					evalAr = dsAttr->GetArray(evalField.toStdString().c_str());

					performanceValue = evalAr->GetTuple1(cell) * curArea +
							restab->GetValue(option, coloffset*4 + 1).ToDouble();
					restab->SetValue(option, coloffset*4 + 1, vtkVariant(performanceValue));

//					NMDebugInd(ind2, << "performance measure: " << performanceValue
//							<< " goes into row,col: " << option << ", " << (coloffset*4+1) << endl);

				}
//				NMDebug(<< endl);

				// update the constraints evaluations
				constrit =  this->mmslCriCons.constBegin();
				coloffset = this->mmslEvalFields.size() + 1;
				for (; constrit != this->mmslCriCons.constEnd(); ++constrit, ++coloffset)
				{
//					NMDebugInd(ind2, << "checking attr cons: " << constrit.key().toStdString() << endl);

					QMap<QString, QStringList>::const_iterator lufmap =
							constrit.value().constBegin();
					for (; lufmap != constrit.value().constEnd(); ++lufmap)
					{
//						NMDebugInd(2, << "land use field map: " << lufmap.key().toStdString() << " " << lufmap.value().join(tr(" ")).toStdString() << endl);

						evalField = tr("");

						// account for zoning
						QStringList zonespec;
						QString opt;
						QString zone;
						if (lufmap.key().contains(tr(":"), Qt::CaseInsensitive))
						{
							zonespec = lufmap.key().split(tr(":"), QString::SkipEmptyParts);
							opt = zonespec.at(0);
							zone = zonespec.at(1);
						}
						else
						{
							opt = lufmap.key();
							zone = "";
						}

						// identify the right field for looking up the performance value
						if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
							evalField = lufmap.value().at(option);
						else if (opt.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
							evalField = lufmap.value().at(0);


						evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
						if (evalAr == 0)
							continue;

						performanceValue = evalAr->GetTuple1(cell) * curArea +
								restab->GetValue(option, coloffset*4 + 1).ToDouble();
						restab->SetValue(option, coloffset*4 + 1, vtkVariant(performanceValue));

//						NMDebugInd(ind2, << "attr performance measure: " << performanceValue
//								<< " goes into row,col: " << option << ", " << (coloffset*4+1) << endl << endl);

					}
				}
			}
//			NMDebug(<< endl);

			// handle the optimised land uses
			double optArea = 0;
			double accumArea = 0;
			if (optResList.contains(this->mslOptions.at(option), Qt::CaseInsensitive))
			{
				// calc the target row of the resulting tab depending on whether we've got
				// mixed land use or not
				//int resTabRow = option;//optResList.size() > 1 ? resNumRows-2 : option;

//				NMDebugInd(ind, << "evaluating optimised resource option '" <<
//						this->mslOptions.at(option).toStdString() << "' ..." << endl);

				// get the allocated area for this resource category
				optArea = optValArs.at(option)->GetTuple1(cell);

//				// track mixed land-use separately
//				if (optResList.size() > 1)
//				{
//					restab->SetValue(resNumRows-2, 2,
//							vtkVariant(optArea + restab->GetValue(resNumRows-2, 2).ToDouble()));
//				}

				// define variable for row index in result table
				int resTabRow = option;//optResList.size() > 1 ? resNumRows-2 : option;

				// update area stats for actual resource category
				accumArea = optArea + restab->GetValue(resTabRow, 2).ToDouble();
				restab->SetValue(resTabRow, 2, vtkVariant(accumArea));

//				NMDebugInd(ind2, << "optArea: " << optArea << endl);
//				NMDebugInd(ind2, << "optaccumArea: " << accumArea << endl);

				// update criteria stats
				QMap<QString, QStringList>::const_iterator evalit =
						this->mmslEvalFields.constBegin();
				int coloffset = 1;
				for (; evalit != this->mmslEvalFields.constEnd(); ++evalit, ++coloffset)
				{
//					NMDebugInd(ind2, << "checking criterion: " << evalit.key().toStdString() << endl);

					evalField = evalit.value().at(option);
					evalAr = dsAttr->GetArray(evalField.toStdString().c_str());

					performanceValue = evalAr->GetTuple1(cell) * optArea +
							restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();
					restab->SetValue(resTabRow, coloffset*4 + 2, vtkVariant(performanceValue));

//					NMDebugInd(ind2, << "performance measure: " << performanceValue
//							<< "goes into row,col: " << resTabRow << ", " << (coloffset*4+2) << endl);
				}

				// update the constraints evaluations
				constrit =  this->mmslCriCons.constBegin();
				coloffset = this->mmslEvalFields.size() + 1;
				for (; constrit != this->mmslCriCons.constEnd(); ++constrit, ++coloffset)
				{
//					NMDebugInd(ind2, << "checking attr cons: " << constrit.key().toStdString() << endl);

					QMap<QString, QStringList>::const_iterator lufmap =
							constrit.value().constBegin();
					for (; lufmap != constrit.value().constEnd(); ++lufmap)
					{

//						NMDebugInd(2, << "land use field map: " << lufmap.key().toStdString() << " " << lufmap.value().join(tr(" ")).toStdString() << endl);

						evalField = tr("");

						// account for zoning
						QStringList zonespec;
						QString opt;
						QString zone;
						if (lufmap.key().contains(tr(":"), Qt::CaseInsensitive))
						{
							zonespec = lufmap.key().split(tr(":"), QString::SkipEmptyParts);
							opt = zonespec.at(0);
							zone = zonespec.at(1);
						}
						else
						{
							opt = lufmap.key();
							zone = "";
						}

						if (optResList.contains(opt, Qt::CaseInsensitive))
						{

							if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
								evalField = lufmap.value().at(option);
							else if (opt.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
								evalField = lufmap.value().at(0);

							evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
							if (evalAr == 0)
								continue;

							performanceValue = evalAr->GetTuple1(cell) * optArea +
									restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();
							restab->SetValue(resTabRow, coloffset*4 + 2, vtkVariant(performanceValue));

	//						NMDebugInd(ind2, << "attr performance measure: " << performanceValue
	//								<< "goes into row,col: " << resTabRow << ", " << (coloffset*4+2) << endl << endl);

						}
					}
				}
			}
		}

		if (cell % 200 == 0)
		{
			NMDebug(<< ".");
		}
	}
	NMDebug(<< " finished!" << std::endl);

	// sum totals
	int ncols = restab->GetNumberOfColumns();
	for (int sp=1; sp < ncols; ++sp)
	{
		double sum = 0.0;
		for (int recs=0; recs < resNumRows-1; ++recs)
			sum += restab->GetValue(recs, sp).ToDouble();
		restab->SetValue(resNumRows-1, sp, vtkVariant(sum));
	}

	// calc differences

	for (int recs=0; recs < resNumRows; ++recs)
	{
		for (int sp=0; sp < ncols-3; sp += 4)
		{
			double diff = restab->GetValue(recs, sp+2).ToDouble() -
					      restab->GetValue(recs, sp+1).ToDouble();
			restab->SetValue(recs, sp+3, vtkVariant(diff));
			double denom = restab->GetValue(recs, sp+1).ToDouble();
			if (denom)
			{
				double rel = diff / denom * 100;
				restab->SetValue(recs, sp+4, vtkVariant(rel));
			}
		}
	}


	NMDebugCtx(ctxNMMosra, << "done!");
	return restab;
}

//QStandardItemModel* NMMosra::prepareResChartModel(vtkTable* restab)
//{
//	if (restab == 0)
//		return 0;
//
//	NMDebugCtx(ctxNMMosra, << "...");
//
//
//	int nDestCols = restab->GetNumberOfRows();
//	int nSrcCols = restab->GetNumberOfColumns();
//	int nDestRows = (nSrcCols-1) / 4;
//
//	QStandardItemModel* model = new QStandardItemModel(nDestRows, nDestCols, this->parent());
//	model->setItemPrototype(new QStandardItem());
//
//	NMDebugAI( << "populating table ..." << endl);
//
//	QStringList slVHeaderLabels;
//	int srccol = 4;
//	for (int row=0; row < nDestRows; ++row, srccol+=4)
//	{
//		QString sVHeader = restab->GetColumnName(srccol);
//		slVHeaderLabels.append(sVHeader);
//		model->setVerticalHeaderItem(row, new QStandardItem());
//		model->verticalHeaderItem(row)->setData(QVariant((int)row*40), Qt::DisplayRole);
//
//		for (int col=0; col < nDestCols; ++col)
//		{
//			if (row == 0)
//			{
//				QString sHHeader = restab->GetValue(col, 0).ToString().c_str();
//				model->setHorizontalHeaderItem(col, new QStandardItem());
//				model->horizontalHeaderItem(col)->setData(QVariant(sHHeader), Qt::DisplayRole);
//			}
//
//			model->setItem(row, col, new QStandardItem());
//			model->item(row, col)->setData(QVariant(restab->GetValue(col, srccol).ToDouble()),
//					Qt::DisplayRole);
//		}
//	}
//	model->setVerticalHeaderLabels(slVHeaderLabels);
//
//	NMDebugCtx(ctxNMMosra, << "done!");
//
//	return model;
//
//}

HLpHelper* NMMosra::getLp()
{
	return this->mLp;
}

int NMMosra::callbackIsSolveCanceled(lprec *lp, void *userhandle)
{
	if (userhandle == 0)
		return 1;

	NMMosra *mosra = (NMMosra*) userhandle;
	return mosra->isSolveCanceled();
}

int NMMosra::isSolveCanceled()
{
	return this->mbCanceled;
}

double NMMosra::convertAreaUnits(double dUserVal, AreaUnitType otype)
{
	bool bConvOK;
	double dtval;
	switch (otype)
	{
	case NMMosra::NM_MOSO_PERCENT_TOTAL:
		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
		else
			dtval = this->mdAreaTotal * dUserVal/100.0;
		break;
	case NMMosra::NM_MOSO_PERCENT_SELECTED:

		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(this->mdAreaSelected * dUserVal/100.0);
		else
			dtval = this->mdAreaSelected * dUserVal/100.0;
		break;
	default:
		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(dUserVal);
		else
			dtval = dUserVal;
		break;
	}

	return dtval;
}

double NMMosra::convertAreaUnits(double dUserVal, const QString& otype, const QStringList& zoneSpec)
{
	bool bConvOK;
	double dtval;

	if (otype.compare(tr("percent_of_total"), Qt::CaseInsensitive) == 0)
	{
		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
		else
			dtval = this->mdAreaTotal * dUserVal/100.0;
	}
	else if (otype.compare(tr("percent_of_selected"), Qt::CaseInsensitive) == 0)
	{

		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(this->mdAreaSelected * dUserVal/100.0);
		else
			dtval = this->mdAreaSelected * dUserVal/100.0;
	}
	else if (otype.compare(tr("percent_of_zone"), Qt::CaseInsensitive) == 0)
	{
		// if there is no zone spec given, we interpret it as percent_of_total
		if (zoneSpec.size() == 0)
		{
			if (this->meDVType == NMMosra::NM_MOSO_INT)
				dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
			else
				dtval = this->mdAreaTotal * dUserVal/100.0;

			// todo: issue a warning message, that a percent_of_total is used instead
		}
		else
		{
			double zonearea = this->mmslZoneAreas.find(zoneSpec.at(1)).value().find(zoneSpec.at(0)).value();
			if (this->meDVType == NMMosra::NM_MOSO_INT)
				dtval = vtkMath::Floor(zonearea * dUserVal/100.0);
			else
				dtval = zonearea * dUserVal/100.0;
		}
	}
	else
	{ // -> "map_units"
		if (this->meDVType == NMMosra::NM_MOSO_INT)
			dtval = vtkMath::Floor(dUserVal);
		else
			dtval = dUserVal;
	}

	return dtval;
}
