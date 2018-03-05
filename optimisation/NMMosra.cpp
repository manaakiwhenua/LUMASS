/******************************************************************************
* Created by Alexander Herzig
* Copyright 2011-2016 Landcare Research New Zealand Ltd
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
 * NMMosra., cpp
 *
 *  Created on: 23/03/2011
 *      Author: alex
 */

#include "NMMosra.h"
#include "nmlog.h"
#include "itkNMLogEvent.h"
#include "NMLogger.h"

/// need to define NMMosra specific debug macros since this class
/// is used as part of the GUI as well as the modlling framework,
/// which sport different logging mechanisms; the macros below
/// cater for this unique situation

#define MosraLogError(arg) \
        { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_ERROR)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_ERROR, \
                        sstr.str().c_str()); \
        }

#define MosraLogWarn(arg)  \
        { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_WARN)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_WARN, \
                        sstr.str().c_str()); \
        }

#define MosraLogInfo(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_INFO)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_INFO, \
                        sstr.str().c_str()); \
       }

#define MosraLogDebug(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_DEBUG)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_DEBUG, \
                        sstr.str().c_str()); \
       }


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
#include <QDateTime>

#include "itkProcessObject.h"

#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkDelimitedTextWriter.h"

#include "lp_lib.h"


////////////////////////////////
/// NMMosraDataSet implementation
////////////////////////////////

NMMosraDataSet::NMMosraDataSet(QObject* parent)
    : mVtkDS(0), mOtbTab(0), mType(NM_MOSRA_DS_NONE)
{
    this->setParent(parent);
}

void
NMMosraDataSet::setDataSet(otb::AttributeTable::Pointer otbtab)
{
    mVtkDS = 0;
    if (otbtab.IsNotNull())
    {
        mOtbTab = otbtab;
        mType = NM_MOSRA_DS_OTBTAB;
    }
    else
    {
        mOtbTab = 0;
        mType = NM_MOSRA_DS_NONE;
    }
}

void
NMMosraDataSet::setDataSet(vtkDataSet* vtkds)
{
    mOtbTab = 0;
    if (vtkds)
    {
        mVtkDS = vtkds;
        mType = NM_MOSRA_DS_VTKDS;
    }
    else
    {
        mVtkDS = 0;
        mType = NM_MOSRA_DS_NONE;
    }
}

bool
NMMosraDataSet::hasColumn(const QString &columnName)
{
    bool ret = false;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        if (mOtbTab->ColumnExists(columnName.toStdString()) >= 0)
        {
            ret = true;
        }
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr->HasArray(columnName.toStdString().c_str()))
            {
                ret = true;
            }
            break;
        }
    default:
        ret = false;
    }

    return ret;
}

int
NMMosraDataSet::getNumRecs()
{
    int recs;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        recs = mOtbTab->GetNumRows();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                recs = dsAttr->GetArray(0)->GetNumberOfTuples();
            }
            break;
        }
    default:
        recs = 0;
    }

    return recs;
}

double
NMMosraDataSet::getDblValue(const QString &columnName, int row)
{
    double val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetDblValue(columnName.toStdString(), row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDataArray* da = dsAttr->GetArray(columnName.toStdString().c_str());
                val = da->GetTuple1(row);
            }
            break;
        }
    default:
        val = 0;
    }

    return val;
}

int
NMMosraDataSet::getIntValue(const QString &columnName, int row)
{
    int val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetIntValue(columnName.toStdString(), row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDataArray* da = dsAttr->GetArray(columnName.toStdString().c_str());
                val = da->GetTuple1(row);
            }
            break;
        }
    default:
        val = 0;
    }

    return val;
}

QString
NMMosraDataSet::getStrValue(const QString &columnName, int row)
{
    QString val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetStrValue(columnName.toStdString(), row).c_str();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(columnName.toStdString().c_str()));
                val = sa->GetValue(row);
            }
            break;
        }
    default:
        val = "";
    }

    return val;
}

/// int col impl
double
NMMosraDataSet::getDblValue(int col, int row)
{
    double val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetDblValue(col, row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDataArray* da = dsAttr->GetArray(col);
                val = da->GetTuple1(row);
            }
            break;
        }
    default:
        val = 0;
    }

    return val;
}

int
NMMosraDataSet::getIntValue(int col, int row)
{
    int val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetIntValue(col, row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDataArray* da = dsAttr->GetArray(col);
                val = da->GetTuple1(row);
            }
            break;
        }
    default:
        val = 0;
    }

    return val;
}

QString
NMMosraDataSet::getStrValue(int col, int row)
{
    QString val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetStrValue(col, row).c_str();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(col));
                val = sa->GetValue(row);
            }
            break;
        }
    default:
        val = "";
    }

    return val;
}
/// in col impl - end

void
NMMosraDataSet::addColumn(const QString &colName, NMMosraDataSetDataType type)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        switch(type)
        {
        case NM_MOSRA_DATATYPE_INT:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_INT);
            break;
        case NM_MOSRA_DATATYPE_DOUBLE:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_DOUBLE);
            break;
        case NM_MOSRA_DATATYPE_STRING:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_STRING);
            break;
        default:
            break;
        }

        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr && dsAttr->GetArray(0))
            {
                vtkAbstractArray* newar = 0;
                switch(type)
                {
                case NM_MOSRA_DATATYPE_INT:
                    newar = vtkAbstractArray::CreateArray(VTK_INT);
                    break;
                case NM_MOSRA_DATATYPE_DOUBLE:
                    newar = vtkAbstractArray::CreateArray(VTK_DOUBLE);
                    break;
                case NM_MOSRA_DATATYPE_STRING:
                    newar = vtkAbstractArray::CreateArray(VTK_STRING);
                    break;
                default:
                    break;
                }
                if (newar)
                {
                    newar->SetName(colName.toStdString().c_str());
                    newar->SetNumberOfComponents(1);
                    newar->SetNumberOfTuples(dsAttr->GetArray(0)->GetNumberOfTuples());
                    mVtkDS->GetCellData()->AddArray(newar);
                }
            }
        }
        break;
    default:
        break;
    }
}

void
NMMosraDataSet::setIntValue(const QString &colname, int row, int value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(),
                          static_cast<long long>(row),
                          static_cast<long long>(value));
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkIntArray* ia = vtkIntArray::SafeDownCast(
                            dsAttr->GetArray(colname.toStdString().c_str()));
                ia->SetValue(row, value);
            }
            break;
        }
    default:
        break;
    }
}

void
NMMosraDataSet::setDblValue(const QString &colname, int row, double value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(), static_cast<long long>(row), value);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(
                            dsAttr->GetArray(colname.toStdString().c_str()));
                da->SetValue(row, value);
            }
            break;
        }
    default:
        break;
    }
}

void
NMMosraDataSet::setStrValue(const QString &colname, int row, const QString& value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(), row, value.toStdString());
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(colname.toStdString().c_str()));
                sa->SetValue(row, value.toStdString());
            }
            break;
        }
    default:
        break;
    }
}

vtkSmartPointer<vtkTable>
NMMosraDataSet::getDataSetAsVtkTable()
{
    vtkSmartPointer<vtkTable> tab = 0;
    if (mVtkDS)
    {
        tab = vtkSmartPointer<vtkTable>::New();
        tab->SetRowData(mVtkDS->GetAttributes(vtkDataSet::CELL));
    }
    return tab;
}

int
NMMosraDataSet::getColumnIndex(const QString &colName)
{
    int val;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->ColumnExists(colName.toStdString());
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                if (dsAttr->GetAbstractArray(colName.toStdString().c_str(), val) == NULL)
                {
                    val = -1;
                }
            }
            break;
        }
    default:
        val = -1;
    }

    return val;
}

////////////////////////////////
/// NMMosra implementation
////////////////////////////////

const std::string NMMosra::ctxNMMosra = "NMMosra";

NMMosra::NMMosra(QObject* parent) //: QObject(parent)
{
	NMDebugCtx(ctxNMMosra, << "...");

    this->mDataSet = new NMMosraDataSet(this);

    this->mLogger = 0;
    this->mProcObj = 0;
	this->setParent(parent);
	this->mLp = new HLpHelper();
	this->reset();

    // seed the random number generator
    srand(time(0));

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
        this->mDataSet->setDataSet(const_cast<vtkDataSet*>(dataset));
	else
	{
        MosraLogError( << "vtkDataSet data set is NULL!");
	}
}

void
NMMosra::setDataSet(otb::AttributeTable::Pointer otbtab)
{
    if (otbtab.IsNotNull())
        this->mDataSet->setDataSet(otbtab);
    else
    {
        MosraLogError( << "otb::AttributeTable is NULL!");
    }
}

void NMMosra::reset(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	this->mLp->DeleteLp();
	this->msLosFileName.clear();
    this->mDataSet->setDataSet(0);

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

    this->mbBreakAtFirst = false;
	this->mbCanceled = false;

	msDataPath = std::getenv("HOME");
	mslPerturbItems.clear();
    mlflPerturbUncertainties.clear();
	mflUncertainties.clear();
	mlReps=1;

    mslPerfSumZones.clear();

    NMDebugCtx(ctxNMMosra, << "done!")
}

bool
NMMosra::doBatch()
{
	bool ret = false;

	if (   !msDataPath.isEmpty()
		&& mslPerturbItems.size() > 0
        && mlflPerturbUncertainties.size() > 0
	   )
	{
		ret = true;
	}

	return ret;
}


int NMMosra::loadSettings(QString fileName)
{
    NMDebugCtx(ctxNMMosra, << "...")

    QFile los(fileName);
    if (!los.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        MosraLogError( << "failed reading settings file!")
        NMDebugCtx(ctxNMMosra, << "done!")
        return 0;
    }
    QTextStream str(&los);

    int ret = parseStringSettings(str.readAll());

    los.close();

    this->msLosFileName = fileName;
    NMDebugCtx(ctxNMMosra, << "done!")
    return ret;
}

int NMMosra::parseStringSettings(QString strSettings)
{
    NMDebugCtx(ctxNMMosra, << "...")

	// reset this objects vars
	this->reset();

    QTextStream str;
    str.setString(&strSettings);

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
        featcons,
		cricons,
		objcons,
		batch,
		nosection
	};

	ParSec section = nosection;

    sReport << "Import Report" << endl << endl;
    MosraLogDebug( << "parsing settings file ..." << endl)
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
        else if (!sLine.compare(tr("<FEATURE_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = featcons;
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
        //----------------------------------------------PROBLEM-------------
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

                MosraLogInfo( << "DEVTYPE: " << this->meDVType << endl)
			}
			else if (sVarName.compare(tr("CRITERION_LAYER"), Qt::CaseInsensitive) == 0)
			{
				if (this->msLayerName.compare(sValueStr, Qt::CaseInsensitive) != 0)
                    this->mDataSet->setDataSet(0);

				this->msLayerName = sValueStr;
                MosraLogInfo( << "LayerName: " << this->msLayerName.toStdString() << endl)
			}
			else if (sVarName.compare(tr("LAND_USE_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msLandUseField = sValueStr;
                MosraLogInfo( << "LandUseField: " << this->msLandUseField.toStdString() << endl)
			}
			else if (sVarName.compare(tr("AREA_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msAreaField = sValueStr;
                MosraLogInfo( << "AreaField: " << this->msAreaField.toStdString() << endl)
			}
            else if (sVarName.compare(tr("PERFORMANCE_SUM_ZONES"), Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->mslPerfSumZones = sValueStr.split(" ", QString::SkipEmptyParts);
                }
                MosraLogInfo( << "PerformanceSumZones: " << mslPerfSumZones.join(" ").toStdString() << endl)
            }
		}
		break;
        //---------------------------------------------CRITERIA-----------
		case criteria:
		{
			if (sVarName.compare(tr("NUM_OPTIONS"), Qt::CaseInsensitive) == 0)
			{
				bool ok;
				long lo = sValueStr.toLong(&ok);
				if (ok)
					this->miNumOptions = lo;
				else
                {
                    MosraLogError(<< "Line " << numline << " contains an invalid number" << endl;)
                    sReport << "Line " << numline << " contains an invalid number" << endl;
                }


                MosraLogInfo( << "number of resource options: " << this->miNumOptions << endl)
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
                    MosraLogError(<< "Line " << numline << " contains an invalid number of options" << endl)
					sReport << "Line " << numline << " contains an invalid number of options" << endl;
				}

                MosraLogInfo( << "options: " << this->mslOptions.join(tr(" ")).toStdString() << endl)

			}
			else if (sVarName.indexOf(tr("CRI_"), Qt::CaseInsensitive) != -1)
			{
				QStringList criFieldNames = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (criFieldNames.size() == this->miNumOptions + 1)
				{
					QString scri = criFieldNames.at(0);
					criFieldNames.removeAt(0);
					this->mmslCriteria.insert(scri, criFieldNames);

                    MosraLogDebug( << "criterion: " << scri.toStdString() << " " << this->mmslCriteria.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criteria" << endl;)
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

                    MosraLogDebug( << "criterion evaluation fields: " << scri.toStdString() << " " << this->mmslEvalFields.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criterion evaluation fields" << endl;)
					sReport << "Line " << numline << " contains an invalid number of criterion evaluation fields" << endl;
				}
			}

		}
		break;
        //----------------------------------------OBJECTIVES----------
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
                MosraLogInfo( << "Scalarisation method is '" << sAggrMeth.toStdString() << "'" << endl)
			}
			else if (sVarName.indexOf(tr("OBJ_"), Qt::CaseInsensitive) != -1)
			{
				 QStringList objs = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				 if (objs.size() != 0)
				 {
					 QString obj = objs.takeAt(1);
					 this->mmslObjectives.insert(obj, objs);
                     MosraLogInfo( << "obj: " << obj.toStdString() << ": "
                             << this->mmslObjectives.find(obj).value().join(tr(" ")).toStdString() << endl)
				 }
			}
		}
		break;
        //-----------------------------------------AREAL_CONS------------
		case arealcons:
		{
			if (sVarName.indexOf(tr("AREAL_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList arCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (arCons.size() != 0)
				{
					this->mmslAreaCons.insert(sVarName, arCons);
                    MosraLogDebug( << "areal cons: " << sVarName.toStdString() << ": "
							<< this->mmslAreaCons.find(sVarName).value().join(tr(" ")).toStdString() << endl);

					// check, whether we've got a zoning constraint here and if so, initialise the
					// zones area with 0
					QString luopt = arCons.at(0);
					if (luopt.contains(':', Qt::CaseInsensitive))
					{
						QStringList luoptlist = luopt.split(tr(":"), QString::SkipEmptyParts);
						QString zonefield = luoptlist.at(1);
                        // allow for multiple land use options being specified separated by comata
                        // !without whitespace!
                        QStringList options = luoptlist.at(0).split("+", QString::SkipEmptyParts);

						// check, whether we've got already a map created for this zonefield
						QMap<QString, QMap<QString, double> >::iterator zonesIt;
						QMap<QString, QMap<QString, int> >::iterator lengthIt;
						zonesIt = this->mmslZoneAreas.find(zonefield);
						lengthIt = this->mmslZoneLength.find(zonefield);
						if (zonesIt == this->mmslZoneAreas.end())
						{
							QMap<QString, double> zoneArea;
							QMap<QString, int> zoneLength;
                            foreach(const QString& opt, options)
                            {
                                zoneArea.insert(opt, 0);
                                zoneLength.insert(opt, 0);
                            }
							this->mmslZoneAreas.insert(zonefield, zoneArea);
							this->mmslZoneLength.insert(zonefield, zoneLength);
						}
						else
						{
                            foreach(const QString& opt, options)
                            {
                                zonesIt.value().insert(opt, 0);
                                lengthIt.value().insert(opt, 0);
                            }
							this->mmslZoneAreas.insert(zonefield, zonesIt.value());
							this->mmslZoneLength.insert(zonefield, lengthIt.value());
						}
					}
				}
			}
		}
		break;
        //----------------------------------------------FEAT_CONS--------
        case featcons:
        {
            if (sVarName.indexOf(tr("FEAT_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList featCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
                if (featCons.size() != 0)
                {
                    this->mmslFeatCons.insert(sVarName, featCons);
                    MosraLogDebug( << "feature cons: " << sVarName.toStdString() << ": "
                            << this->mmslFeatCons.find(sVarName).value().join(tr(" ")).toStdString() << endl);
                }
            }
        }
        break;
        //----------------------------------------------ATTR_CONS--------
		case cricons:
		{
			if (sVarName.indexOf(tr("CRI_CONS_"), Qt::CaseInsensitive) != -1)
			{
                MosraLogDebug(<< "\tgonna split raw list: " << sValueStr.toStdString() << endl);
				QStringList outerList = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (outerList.size() != 0)
				{
					QString criLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tcriLabel is '" << criLabel.toStdString() << "'" << endl);
					QString luLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tland use is '" << luLabel.toStdString() << "'" << endl);

					QMap<QString, QStringList> innerMap;
					innerMap.insert(luLabel, outerList);

					this->mmslCriCons.insert(criLabel, innerMap);

                    MosraLogDebug( << "cri cons: " << criLabel.toStdString() << ": "
							<< luLabel.toStdString() << ": "
							<< outerList.join(tr(" ")).toStdString() << endl);
				}
			}
		}
		break;
        //------------------------------------------------OBJ_CONS-------
		case objcons:
		{
			if (sVarName.indexOf(tr("OBJ_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList objCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (objCons.size() > 0)
				{
					QString objkey = sVarName + QString(tr("_%1")).arg(objCons.value(0));
					this->mmslObjCons.insert(objkey, objCons);
                    MosraLogInfo( << objkey.toStdString() << ": "
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
                    MosraLogInfo(<< "batch data path: " << this->msDataPath.toStdString() << endl);
				}
			}
			else if (sVarName.compare("PERTURB", Qt::CaseInsensitive) == 0)
			{
				this->mslPerturbItems = sValueStr.split(" ");
				if (mslPerturbItems.size() > 0)
				{
                    MosraLogInfo(<< "Criteria/constraints to be perturbed: ");
					foreach(const QString& pc, this->mslPerturbItems)
					{
						NMDebug(<< pc.toStdString() << " ");
					}
					NMDebug(<< endl);
				}
				else
				{
                    MosraLogInfo(<< "No perturbation criteria provided!" << endl);
				}
			}
			else if (sVarName.compare("UNCERTAINTIES", Qt::CaseInsensitive) == 0)
			{
				QStringList lunsure = sValueStr.split(" ");
                bool bok;
				float val;
				foreach(const QString& vstr, lunsure)
				{
                    QStringList levels = vstr.split(",");
                    QList<float> lstUncertainties;
                    foreach(const QString& l, levels)
                    {
                        val = l.toFloat(&bok);
                        if (bok)
                        {
                            lstUncertainties.push_back(val);
                        }
                    }
                    if (lstUncertainties.size() > 0)
                    {
                        this->mlflPerturbUncertainties.push_back(lstUncertainties);
                    }
				}

                if (this->mlflPerturbUncertainties.size() > 0)
				{
                    std::stringstream logstr;
                    logstr << "Perturbation levels: ";
                    foreach(const QList<float>& lf, mlflPerturbUncertainties)
                    {
                        foreach(const float& f, lf)
                        {
                            logstr << f << " ";
                        }
                        logstr << " | ";
                    }
                    logstr << endl;
                    MosraLogDebug(<< logstr.str())
				}
				else
				{
                    MosraLogInfo(<< "No uncertainty levels for perturbation provided!" << endl);
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
                        MosraLogInfo(<< "Number of perturbations: " << reps << endl);
					}
				}
			}
			else if (sVarName.compare("TIMEOUT", Qt::CaseInsensitive) == 0)
			{
				if (!sValueStr.isEmpty())
				{
                    if (sValueStr.compare("break_at_first", Qt::CaseInsensitive) == 0)
                    {
                        this->muiTimeOut = 0;
                        this->mbBreakAtFirst = true;
                        MosraLogInfo(<< "Solver timeout: break at first feasible solution!" << endl);
                    }
                    else
                    {
                        bool bok;
                        unsigned int timeout = sValueStr.toUInt(&bok);
                        if (bok)
                        {
                            this->mbBreakAtFirst = false;
                            this->muiTimeOut = timeout;
                            MosraLogInfo(<< "Solver timeout: " << timeout << endl);
                        }
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
    MosraLogInfo(<< "checking optimisation settings - OK")

	this->makeLp();

	if (!this->addObjFn())
	{
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}
    MosraLogInfo(<< "adding objective function - OK")

	if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
			this->mmslObjCons.size() > 0)
    {
		this->addObjCons();
        MosraLogInfo(<< "adding objective constraints - OK")
    }

	// doe we have any additional constraints?
	if (this->mmslAreaCons.size() > 0)
	{
		if (!this->addExplicitAreaCons())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			return 0;
		}
        MosraLogInfo(<< "adding allocation constraints - OK")
	}

	if (this->mmslCriCons.size() > 0)
	{
		if (!this->addCriCons())
		{
			NMDebugCtx(ctxNMMosra, << "done!");
			return 0;
		}
        MosraLogInfo(<< "adding performance constraints - OK")
	}


    if (this->mmslFeatCons.size() > 0)
    {
        if (!this->addFeatureCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding feature constraints - OK");
    }
    else
    {
        if (!this->addImplicitAreaCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding internal areal (consistency) constraints - OK")
    }

    if (this->mbBreakAtFirst)
    {
        this->mLp->SetBreakAtFirst(true);

        MosraLogInfo(<< "solver stops at first feasible solution!" << std::endl);
    }
    else
    {
        this->mLp->SetTimeout(this->muiTimeOut);

        MosraLogInfo(<< "solver times out after " << this->muiTimeOut
                << " seconds!" << std::endl);
    }
    this->mbCanceled = false;
    this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);
    this->mLp->SetLogFunc((void*)this, NMMosra::lpLogCallback);



    this->mLp->SetPresolve(PRESOLVE_COLS |
                           PRESOLVE_ROWS |
                           PRESOLVE_IMPLIEDFREE |
                           PRESOLVE_REDUCEGCD |
                           PRESOLVE_MERGEROWS |
                           PRESOLVE_ROWDOMINATE |
                           PRESOLVE_COLDOMINATE |
                           PRESOLVE_KNAPSACK |
                           PRESOLVE_PROBEFIX);


	this->mLp->Solve();

	this->createReport();

    MosraLogInfo(<< "Optimisation Report ... \n" << this->getReport().toStdString() << endl);

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
            QStringList options;

			if (ait.value().at(0).contains(":", Qt::CaseInsensitive))
			{
				zonespec = ait.value().at(0).split(tr(":"), QString::SkipEmptyParts);
                options = zonespec.at(0).split(tr("+"), QString::SkipEmptyParts);
				//nit = 2;
			}
            else
            {
                options << ait.value().at(0).split(tr("+"), QString::SkipEmptyParts);
            }

			for (int q=0; q < nit; ++q)
			{
				sACL = this->mmslAreaConsLabel.at(totalcount);
				index = this->mLp->GetNameIndex(sACL.toStdString() , true);
				if (index >= 0)
				{
                    double maxval = 0;
                    foreach(const QString& opt, options)
                    {
                        QStringList ozpec;
                        if (zonespec.size() > 1)
                        {
                            ozpec << opt << zonespec.at(1);
                        }
                        dval = ait.value().at(2).toDouble(&ok);
                        dval = this->convertAreaUnits(dval, ait.value().at(3), ozpec);
                        maxval = dval > maxval ? dval : maxval;
                    }
					if (q == 1)
                        maxval = 0;

					sRes << sACL << " = " << pdCons[index-1] << " ( "
                        << ait.value().at(1) << " " << maxval << " )" << endl;
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
    MosraLogDebug( << endl << "just dumping all constraint values .... " << endl);
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
        MosraLogDebug(<< name.toStdString() << " " << op << " " << fv << endl);
	}
    //NMDebug(<< endl);


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
        MosraLogError( << "failed writing file '" << fileName.toStdString() << "'!");
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
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);

    MosraLogInfo(<< "Optimisation - Checking settings ...")
	//  get the total area of the layer (summing the provided field's data)
	if (this->msAreaField.isEmpty())
	{
        MosraLogError( << "no area field specified!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

//	if (!dsAttr->HasArray(this->msAreaField.toStdString().c_str()))
    if (!mDataSet->hasColumn(this->msAreaField.toStdString().c_str()))
	{
        MosraLogError( << "specified area field could not be found!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}
    MosraLogInfo(<< "area field OK" << endl);

	// --------------------------------------------------------------------------------------------------------
    //MosraLogInfo(<< "calculating area and counting features ..." << endl);
//	vtkDataArray* areaAr = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
//	vtkDataArray* nm_hole = dsAttr->GetArray("nm_hole");
//	int numTuples = areaAr->GetNumberOfTuples();
    bool nm_hole = mDataSet->hasColumn("nm_hole");
    int numTuples = mDataSet->getNumRecs();
	int numFeat = 0;

	QMap<QString, QMap<QString, double> >::iterator zonesIt;
	QMap<QString, QMap<QString, int> >::iterator zonesLenIt;
	QMap<QString, double>::iterator optIt;
	QMap<QString, int>::iterator optLenIt;

	double tmpVal;
	int tmpLen;
	bool arealCriValid = true;
	this->mdAreaTotal = 0;

//	for (int cs=0; cs < areaAr->GetNumberOfTuples(); cs++)
    for (int cs=0; cs < numTuples; cs++)
	{
//		if (nm_hole->GetTuple1(cs) == 1)
        if (nm_hole && mDataSet->getIntValue("nm_hole", cs) == 1)
        {
            continue;
        }

        //this->mdAreaTotal += areaAr->GetTuple1(cs);
        this->mdAreaTotal += mDataSet->getDblValue(this->msAreaField, cs);
		numFeat++;

		// iterate over the initialised zones and calc areas
		zonesIt = this->mmslZoneAreas.begin();
		zonesLenIt = this->mmslZoneLength.begin();
		for (; zonesIt != this->mmslZoneAreas.end(); ++zonesIt, ++zonesLenIt)
		{
            bool zoneAr = mDataSet->hasColumn(zonesIt.key());

//            vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(
//					dsAttr->GetAbstractArray(zonesIt.key().toStdString().c_str()));
//			if (zoneAr == 0)
            if (zoneAr == false)
			{
				arealCriValid = false;
                MosraLogError( << "specified zone field '" << zonesIt.key().toStdString()
						<< "' does not exist in the data base!");
				continue;
			}

			optIt = zonesIt.value().begin();
			optLenIt = zonesLenIt.value().begin();
			for (; optIt != zonesIt.value().end(); ++optIt, ++optLenIt)
			{
                std::string zoneVal = mDataSet->getStrValue(zonesIt.key(), cs).toStdString();
                if (zoneVal.find(optIt.key().toStdString()) != std::string::npos)
				{
//					tmpVal = optIt.value() + areaAr->GetTuple1(cs);
                    tmpVal = optIt.value() + mDataSet->getDblValue(this->msAreaField, cs);
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
        bool zoneAr = mDataSet->hasColumn(zonesIt.key());

//        vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(zonesIt.key().toStdString().c_str()));
//		if (zoneAr == 0)
        if (zoneAr == false)
        {
            continue;
        }

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
            MosraLogError( << "areal constraint '" << acIt.key().toStdString()
					<< ": " << acIt.value().join(" ").toStdString() << "' is invalid! Check number of parameters!");
			arealCriValid = false;
			continue;
		}

		QString unittype = acIt.value().at(3);
		QStringList zonespec;
		double val = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, zonespec);
        totalConsArea += val;

		// now check, whether we've got an issue with one of the zonal values
		QString OptZone = acIt.value().at(0);
		if (OptZone.contains(":", Qt::CaseInsensitive))
		{
			QStringList ozlist = OptZone.split(tr(":"), QString::SkipEmptyParts);
            QStringList options = ozlist.at(0).split(tr("+"), QString::SkipEmptyParts);
			QString zone = ozlist.at(1);

            double zval = 0;
            foreach(const QString& opt, options)
            {
                if (!this->mslOptions.contains(opt))
                {
                    MosraLogError( << "specified option '" << opt.toStdString()
                          << "' is not a valid resource option!");
                    arealCriValid = false;
                    continue;
                }

                QStringList ozspec;
                ozspec << opt << zone;
                double oval = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, ozspec);
                zval = oval > zval ? oval : zval;
            }


            foreach(const QString& opt, options)
            {
                QStringList ozspec;
                ozspec << opt << zone;
                double oval = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, ozspec);

                if (oval > zval)
                {
                    MosraLogWarn( << "area constraint for option '" << opt.toStdString() << "' with respect to zone field '"
                            << zone.toStdString() << "' exceeds the available area for that option in that zone!");
                    //	arealCriValid = false;
                }
            }
		}

	}
    if (arealCriValid)
    {
        MosraLogInfo(<< "areal constraints OK!");
    }



	// now check on the total area
//	if (totalConsArea > this->mdAreaTotal)
//	{
//		MosraLogError( << "the specified area constraints exceed the total available area!");
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
//		MosraLogInfo( << "layer features: " << this->mlNumOptFeat << endl);
//	}
//	else // NM_IMAGE_LAYER
//	{
//		; //TODO: in case of an image layer return the number of
//		  //pixels
//	}

	// cell id field = vtkPolyData cellId

	// --------------------------------------------------------------------------------------------------------
    //sMosraLogInfo(<< "checking performance indicator fields for optimisation criteria ..." << endl);
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
//			if (!dsAttr->HasArray(fields.at(f).toStdString().c_str()))
            if (!mDataSet->hasColumn(fields.at(f)))
			{
				criValid = false;
                MosraLogError( << "couldn't find performance indicator '"
						<< fields.at(f).toStdString() << "'!");
			}
		}
	}
    if (criValid)
    {
        MosraLogInfo( << "optimisation criteria: performance indicator fields OK");
    }

//    MosraLogInfo(<< "checking fields for evaluating criteria performance ..." << endl);
//	// performance indicator fields
	criit = this->mmslEvalFields.constBegin();

	bool evalValid = true;
	for (; criit != this->mmslEvalFields.constEnd(); ++criit)
	{
		// check each given performance indicator field
		QStringList fields = criit.value();
		for (int f=0; f < fields.size(); ++f)
		{
//			if (!dsAttr->HasArray(fields.at(f).toStdString().c_str()))
            if (!mDataSet->hasColumn(fields.at(f)))
			{
				evalValid = false;
                MosraLogError( << "couldn't find performance evaluation indicator '"
						<< fields.at(f).toStdString() << "'!");
			}
		}
	}
    if (evalValid)
    {
        MosraLogInfo(<< "optimisation criteria: performance evaluation fields OK")
    }

	// check the performance indicator fields specified with
	// criteria constraints
	bool criConsValid = true;
	if (this->mmslCriCons.size() > 0)
	{
        //MosraLogInfo(<< "checking performance indicator fields for attributive constraints ..." << endl);
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
//					if (!dsAttr->HasArray(zone.toStdString().c_str()))
                    if (!mDataSet->hasColumn(zone))
					{
						criConsValid = false;
                        MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find zone field '"
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
//						if (!dsAttr->HasArray(fieldList.at(f).toStdString().c_str()))
                        if (!mDataSet->hasColumn(fieldList.at(f)))
						{
							criConsValid = false;
                            MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
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
                        MosraLogError( << "CRITERIA_CONSTRAINTS: specified resource '"
								<< landuse.toStdString() << "' does not match any of "
								"the previously specified resources!");
						continue;
					}

					// look for the specified performance field of the given land use
//					if (!dsAttr->HasArray(fieldList.at(0).toStdString().c_str()))
                    if (!mDataSet->hasColumn(fieldList.at(0)))
					{
						criConsValid = false;
                        MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
								<< fieldList.at(0).toStdString() << "'!");
					}
				}
			}
		}
	}
    if (criConsValid)
    {
        MosraLogInfo(<< "performance constraints OK")
    }

	// --------------------------------------------------------------------------------------------------------
    //MosraLogInfo(<< "calculating size of the optimsation matrix ..." << endl);
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

    this->mlNumDVar =  this->mlNumArealDVar;// + this->mlNumOptFeat;
	sstr << "mlNumDvar =  mlNumArealDVar + mlNumOptFeat = "
            << this->mlNumDVar << " = " << this-> mlNumArealDVar << endl;
            //<< " + " << this->mlNumOptFeat << endl;

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

    MosraLogDebug(<< "Optimisation Settings Report ...\n"
               << this->msSettingsReport.toStdString() << endl);

	NMDebugCtx(ctxNMMosra, << "done!");
	if (!criValid || !criConsValid || !evalValid || !arealCriValid)
	{
        MosraLogError( << "The configuration file contains invalid settings!");
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
        MosraLogDebug( << "unfortunately no feasible solution!" << endl);
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
	}

	// get the attributes of the layer
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    bool hole = mDataSet->hasColumn("nm_hole");
    //long lNumCells = hole->GetNumberOfTuples();
    long lNumCells = mDataSet->getNumRecs();

	// create a new result field for the resource option
//    vtkSmartPointer<vtkStringArray> opt_str;
//	opt_str = vtkSmartPointer<vtkStringArray>::New();
//	opt_str->SetName("OPT_STR");
//	opt_str->Allocate(lNumCells, 100);
    mDataSet->addColumn("OPT_STR", NMMosraDataSet::NM_MOSRA_DATATYPE_STRING);
    MosraLogDebug( << "created attribute 'OPT_STR'" << endl);


	// check for the scoring attributes
//	std::vector< vtkSmartPointer< vtkDoubleArray > > vValAr;
    QStringList vValAr;
	for (int option=0; option < this->miNumOptions; option++)
	{
		QString optx_val = QString(tr("OPT%1_VAL")).arg(option+1);
//		vtkSmartPointer<vtkDoubleArray> var = vtkSmartPointer<vtkDoubleArray>::New();
//		var->SetName(optx_val.toStdString().c_str());
//		var->Allocate(lNumCells, 100);
        mDataSet->addColumn(optx_val, NMMosraDataSet::NM_MOSRA_DATATYPE_DOUBLE);
//        vValAr.push_back(var);
        vValAr.push_back(optx_val);

        MosraLogDebug( << "created scoring attribute '" << optx_val.toStdString() << "'" << endl);
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

    //MosraLogDebug( << "extracting results ..." << endl);
	// -------------------------------------------------------------------for each feature
	for (int f=0; f < lNumCells; f++)
	{
		// special treatment for polygon holes
//		if (hole->GetTuple1(f))
        if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
		{
			// write 0 values into the OPTx_VAL arrarys for holes
			for (int option=0; option < this->miNumOptions; option++)
            {
                mDataSet->setDblValue(vValAr.at(option), f, 0.0);
//                vValAr.at(option)->InsertValue(f, 0.0);
            }
            mDataSet->setStrValue("OPT_STR", f, "");
//			opt_str->InsertValue(f, "");

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
//			vValAr.at(option)->InsertValue(f, dVal);
            mDataSet->setDblValue(vValAr.at(option), f, dVal);

			// format sOptStr
			if (dVal > 0)
				sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
		}

		// trim sOptStr and write into array
		sOptStr = sOptStr.simplified();
//		opt_str->InsertValue(f, sOptStr.toStdString().c_str());
        mDataSet->setStrValue("OPT_STR", f, sOptStr);

		if (f % 100 == 0)
            NMDebug(<< ".");

		// increment the valid feature counter
		lNonHoleCounter++;
	}

    MosraLogDebug(<< "extracted results!" << endl);

	// don't forget to add the new attributes to the
	// data set! (remember: availalbe attributes with the same name
	// are overriden!)
//	ds->GetCellData()->AddArray(opt_str);
//	for (int option=0; option < this->miNumOptions; option++)
//		ds->GetCellData()->AddArray(vValAr.at(option));

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
    for (int of=0; of < this->mlNumOptFeat; ++of)//, ++colPos)
	{
		QString colname;
		for (int opt=1; opt <= this->miNumOptions; ++opt, ++colPos)
		{
			colname = QString(tr("X_%1_%2")).arg(of).arg(opt);
			this->mLp->SetColName(colPos, colname.toStdString());

            //MosraLogInfo(<< "#" << colPos << ": " << colname.toStdString() << std::endl);

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

        //		colname = QString(tr("b_%1")).arg(of);
        //		this->mLp->SetColName(colPos, colname.toStdString());
        //		this->mLp->SetBinary(colPos, true);
        //  ++colPos;
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
    return mDataSet->getDataSetAsVtkTable();
}


bool
NMMosra::perturbCriterion(const QString& criterion,
        const QList<float>& percent)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// check, whether the dataset has been set yet
    if (   this->mDataSet == 0
        || mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_NONE
       )
	{
        MosraLogError( << "Input dataset has not been set!");
        return false;
	}

//	vtkSmartPointer<vtkTable> tabCalc = this->getDataSetAsTable();

    // extract individual constraint or criterion identifier
    // from the 'criterion' parameter list, e.g.
    // constraint identifier: "OBJ:Nleach,OBJ:Sediment"
    // criterion identifier: "Nleach,Sediment"
    // -> split by ','

    QStringList metaList = criterion.split(",", QString::SkipEmptyParts);

    // check for criterion or constraint
    if (metaList.size() == 0)
    {
        MosraLogError( << "Empty PERTURB parameter!");
        return false;
    }

    bool bCriterion = true;
    QString fstItem = metaList.at(0).trimmed();
    if (    fstItem.startsWith(QString("OBJ"))
         || fstItem.startsWith(QString("CRI"))
         //|| fstItem.startsWith(QString("AREA"))
       )
    {
        // nope, looks like we've got a constraint
        bCriterion = false;
    }

	// need to distinguish between criterion and constraint
	// so, if we've got just one repetition, we assume to
	// have to deal with a constraint rather than a criterion
    if (!bCriterion)
	{
		// constraint
        for (int cri=0; cri < metaList.size(); ++cri)
		{
            float perc = percent.last();
            if (cri < percent.size())
                perc = percent.at(cri);
            if (!this->varyConstraint(metaList.at(cri), perc))
            {
                return false;
            }
		}
	}
	else
	{
        for (int ptbItem=0; ptbItem < metaList.size(); ++ptbItem)
        {
            QString isolatedCriterion = metaList.at(ptbItem).trimmed();

            // extract the land use
            QStringList splitCriterion = isolatedCriterion.split(":", QString::SkipEmptyParts);
            if (splitCriterion.size() < 2)
            {
                MosraLogError( << "Invalid criterion identifier: '"
                                  << isolatedCriterion.toStdString() << "'!");
                return false;
            }

            QString criLabel = splitCriterion.at(0);
            QString optLabel = splitCriterion.at(1);

            int optIdx = this->mslOptions.indexOf(optLabel);

            // get the performance score field for the specified land use criterion
            // combination
            QStringList fields = this->mmslCriteria.value(criLabel);
            if (fields.size() == 0)
            {
                MosraLogError( << "Got an empty field list for '"
                                  << criLabel.toStdString() << "'!");
                return false;
            }

            QString field = fields.at(optIdx);

            // identify the criterion
            //            QStringList fields = this->mmslCriteria.value(isolatedCriterion);
            //            if (fields.size() == 0)
            //            {
            //                MosraLogInfo(<< "nothing to perturb for criterion '"
            //                        << criterion.toStdString() << "'" << endl);
            //                return false;
            //            }

            // perturb field values by +/- percent of field value
            //srand(time(0));
            //for (int f=0; f < fields.size(); ++f)
            {
                //const QString& field = fields.at(f);

                // grab corresponding uncertainty level, or re-use
                // the last one, if there aren't any more
                float perc = percent.last();
                //if (f < percent.size())
                //    perc = percent.at(f);
                if (ptbItem < percent.size())
                {
                    perc = percent.at(ptbItem);
                }

//                vtkDataArray* srcAr = vtkDataArray::SafeDownCast(
//                        tabCalc->GetColumnByName(field.toStdString().c_str()));

//                for (int r=0; r < tabCalc->GetNumberOfRows(); ++r)
                for (int r=0; r < mDataSet->getNumRecs(); ++r)
                {
//                    double inval = srcAr->GetTuple1(r);
                    double inval = mDataSet->getDblValue(field, r);
                    double uncval = inval * ((rand() % ((int)perc+1))/100.0);
                    double newval;
                    if (rand() % 2)
                        newval = inval + uncval;
                    else
                        newval = inval - uncval;

//                    srcAr->SetTuple1(r, newval);
                    mDataSet->setDblValue(field, r, newval);
                }
            }
        }
	}

	NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

bool
NMMosra::varyConstraint(const QString& constraint,
                        float percent)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// identifying the constraint
	QStringList identlist = constraint.split(":");
    if (identlist.size() < 2)
	{
        MosraLogError( << "Invalid pertubation item specified: "
				<< constraint.toStdString() << std::endl);
		NMDebugCtx(ctxNMMosra, << "done!");

		//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
		//e.setDescription("Invalid pertubation item specified!");
		//throw e;

        return false;
	}

	// read information
	QString type = identlist.at(0);
	QString label = identlist.at(1);
    QString use = "";
	QString zone = "";
    if (identlist.size() >= 3)
        use = identlist.at(2);
	if (identlist.size() == 4)
		zone = identlist.at(3);

    if (    type.compare("CRI", Qt::CaseInsensitive) == 0
        &&  !use.isEmpty()
       )
	{
        // construct the 'key' we're looking for
        QString tident = use;
        if (!zone.isEmpty())
        {
            tident = QString("%1:%2").arg(use).arg(zone);
        }

		// since we could have more than one constraint with the same
		// label, we just go through and check the use
		QMap<QString, QStringList> lufm;
		QMultiMap<QString, QMap<QString, QStringList> >::iterator it =
				mmslCriCons.begin();
		while(it != mmslCriCons.end())
		{
			if (it.key().compare(label) == 0)
			{
                if (it.value().find(tident) != it.value().end())
                {
                    lufm = mmslCriCons.take(it.key());
                    break;
                }
			}

			++it;
		}


		if (lufm.isEmpty())
		{
            MosraLogError( << "Land use field map is empty!");
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setDescription("Land use field map is empty!");
			//throw e;
            return false;
		}

		// get the field list + the operator + cap
        QStringList fieldvaluelist = lufm.value(tident);
		if (fieldvaluelist.empty())
		{
            MosraLogError( << "Field value list is empty!");
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setDescription("field value list is empty!");
			//throw e;
            return false;
		}

        fieldvaluelist = lufm.take(tident);

		bool bok;
		double cap = fieldvaluelist.last().toDouble(&bok);
		if (!bok)
		{
            MosraLogError( << "Constraint threshold is not a number!")
			NMDebugCtx(ctxNMMosra, << "done!");
			//NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
			//e.setDescription("Land use field map is empty!");
			//throw e;
            return false;
		}

        MosraLogDebug(<< "old value for " << constraint.toStdString()
				<< " = " << cap << endl);


		// adjust the cap value by adding the percentage of change
		cap += (cap * percent/100.0);

        MosraLogDebug(<< "new value for " << constraint.toStdString()
				<< " = " << cap << endl);

		// vary the constraint value (cap)
		QString strVal = QString("%1").arg(cap);
		fieldvaluelist.replace(fieldvaluelist.size()-1, strVal);

		// put the multi-map back together again
        lufm.insert(tident, fieldvaluelist);
		mmslCriCons.insert(label, lufm);

	}
	else if (type.compare("AREA", Qt::CaseInsensitive) == 0)
	{
		//ToDo
	}
	else if (type.compare("OBJ", Qt::CaseInsensitive) == 0)
	{
        // look for the objective contraint label (not key!) and
        // vary the contraint
        QMap<QString, QStringList>::iterator oconsIt =
                this->mmslObjCons.begin();

        while(oconsIt != this->mmslObjCons.end())
        {
            if (oconsIt.value().contains(label))
            {
                double val = oconsIt.value().takeLast().toDouble();
                MosraLogDebug(<< label.toStdString() << " old: " << val << std::endl);
                val += (val * percent/100.0);
                MosraLogDebug(<< label.toStdString() << " new: " << val << std::endl);
                oconsIt.value().push_back(QString("%1").arg(val));
                break;
            }

            ++oconsIt;
        }
	}

	NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

int NMMosra::addObjFn(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// get the hole array
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    bool hole = mDataSet->hasColumn("nm_hole");
//	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());


	// some vars we need

	// init some mem for the row and column objects of the
	// problem
	double* pdRow = new double[this->mlNumArealDVar];
	int* piColno = new int[this->mlNumArealDVar];
	int* plFieldIndices = new int[this->miNumOptions];


	QString s1Maxmin = this->mmslObjectives.begin().value().at(0).left(3);
	QString sxMaxmin;
	QString sFieldName;
    //long lId, lNumCells = hole->GetNumberOfTuples();
    long lNumCells = mDataSet->getNumRecs();
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
        MosraLogDebug( << "objective '" << it.key().toStdString() << "' ..." << endl);

        MosraLogDebug( << "criterion / option: <performance indi field> <field index> ... " << endl);
		for (int option=0; option < this->miNumOptions; option++)
		{
			// field name of performance indicator for
			// current criterion and option
			QString sCriterion = it.key();
			QStringList criFieldList = this->mmslCriteria.value(sCriterion);
			QString sField = "";
			if (option <= criFieldList.size()-1)
				sField = criFieldList.at(option);

            plFieldIndices[option] = mDataSet->getColumnIndex(sField);

//            if (dsAttr->GetArray(sField.toStdString().c_str(), *(plFieldIndices + option)) == NULL)
            if (plFieldIndices[option] == -1)
			{
                MosraLogError( << "failed to get performance indicator '"
						<< sField.toStdString() << "' for option '"
						<< this->mslOptions.at(option).toStdString() << "'!");

				// free some memory
				delete[] pdRow;
				delete[] piColno;
				delete[] plFieldIndices;

				NMDebugCtx(ctxNMMosra, << "done!");
				return 0;
			}

            MosraLogDebug( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
					<< ": " << sField.toStdString() << " " << plFieldIndices[option] << endl);
		}

		// maximise or minimise ?
		sxMaxmin = it.value().at(0).left(3);

		// align optimisation task with that of objective 1
		bool bMalMinusEins = false;
		if (s1Maxmin.compare(sxMaxmin, Qt::CaseInsensitive) != 0)
			bMalMinusEins = true;

		// get the weight for this objective
        dWeight = 1.0;
        if (it.value().size() == 2)
        {
            dWeight = it.value().at(1).toDouble(&convOK);
        }
        MosraLogDebug( << "sxMaxmin: " << sxMaxmin.toStdString() << " | adjust task: " << bMalMinusEins << " | weight: " << dWeight << endl);

        MosraLogDebug( << "processing individual features now" << endl);
		// --------------------------------------------------for each feature
		long colPos = 1;
		long arpos = 0;
		for (int f=0; f < lNumCells; ++f)
		{
			// leap over holes
//            if (hole->GetTuple1(f))
            if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
            {
				continue;
            }

			// ----------------------------------------------for each option
			for (int option=0; option < this->miNumOptions; option++, arpos++, colPos++)
			{
				// get performance indicator for current feature and
				// criterion and option
//				vtkDataArray* da = dsAttr->GetArray(plFieldIndices[option]);
//				dCoeff = da->GetTuple1(f);
                dCoeff = mDataSet->getDblValue(plFieldIndices[option], f);

				// DEBUG
				//if (f < 100)
                //	MosraLogDebug( << lCounter << " : " << this->mslOptions.at(option).toStdString()
				//			<< " : " << da->GetName() << " : " << dCoeff << endl);

				// adjust according to MinMax of objective 1
				dCoeff = bMalMinusEins ? dCoeff * -1 : dCoeff;

				// apply objective weight if applicable
				if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
					dCoeff *= dWeight;

				// do we have binary decision variables?
//				double area = areas->GetTuple1(f);
                double area = mDataSet->getDblValue(this->msAreaField, f);
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
            //colPos++;

			if (f % 500 == 0)
				NMDebug(<< ".");
		}

		NMDebug(<< " finished!" << endl);
	}

	int ncols = this->mLp->GetNColumns();
    //MosraLogDebug( << "num cols: " << ncols << " | NumArealDVar: " << this->mlNumArealDVar << endl);


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
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    bool hole = mDataSet->hasColumn("nm_hole");
//	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
//	long lNumCells = hole->GetNumberOfTuples();
    long lNumCells = mDataSet->getNumRecs();

	// create vectors holding the constraint types
	// and objective constraint labels
	std::vector<unsigned int> vnConsType;
	std::vector<QString> vsObjConsLabel;
	QMap<QString, QStringList>::const_iterator it = this->mmslObjCons.constBegin();

    MosraLogDebug( << "reading props" << endl);
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

        MosraLogDebug( << it.key().toStdString() << ": " << it.value().join(tr(" ")).toStdString() << endl);
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
            MosraLogError( << "couldn't convert " << it.value().at(2).toStdString() << " into a double!" << endl);

        MosraLogDebug( << it.key().toStdString() << endl);

        MosraLogDebug( << "criterion / option: <performance indi field> <field index> ... " << endl);
		for (int option=0; option < this->miNumOptions; option++)
		{
			// field name of performance indicator for
			// current criterion and option
			QString sCriterion = it.value().at(0);
			QString sField = this->mmslCriteria.value(sCriterion).at(option);

            piFieldIndices[option] = mDataSet->getColumnIndex(sField);

//			if (dsAttr->GetArray(sField.toStdString().c_str(), *(piFieldIndices + option)) == NULL)
            if (piFieldIndices[option] == -1)
			{
                MosraLogError( << "failed to get performance indicator '"
						<< sField.toStdString() << "'!");

				// free some memory
				delete[] pdRow;
				delete[] piColno;
				delete[] piFieldIndices;

				NMDebugCtx(ctxNMMosra, << "done!");
				return 0;
			}

            MosraLogDebug( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
					<< ": " << sField.toStdString() << " " << piFieldIndices[option] << endl);
		}

		// ------------------------------------------------------------for each spatial alternative
		long lCounter = 0;
		long colpos = 1;
		for (int f=0; f < lNumCells; f++)
		{
			// leap frog holes in polygons
//			if (hole->GetTuple1(f))
            if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
            {
				continue;
            }

			for (int option=0; option < this->miNumOptions; option++)
			{
//				vtkDataArray* da = dsAttr->GetArray(piFieldIndices[option]);
//				dCoeff = da->GetTuple1(f);
                dCoeff = mDataSet->getDblValue(piFieldIndices[option], f);

				// do we have binary decision variables?
//				double area = areas->GetTuple1(f);
                double area = mDataSet->getDblValue(this->msAreaField, f);
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
            //colpos++;

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

int NMMosra::addFeatureCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    bool hole = mDataSet->hasColumn("nm_hole");

    std::vector<QString> vsConsLabel;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    long lNumCells = mDataSet->getNumRecs();
    double dUserVal;
    double dUserArea = 0.0;
    double dAreaTotal = this->mdAreaTotal;

    QMap<QString, QStringList>::const_iterator it =
            this->mmslFeatCons.constBegin();

    MosraLogDebug(<< this->mmslFeatCons.size() << " cons to process" << endl);
    // ------------------------------------------------------ for each constraint
    for(int iConsCounter=1; it != this->mmslFeatCons.constEnd(); it++, iConsCounter++)
    {
        MosraLogDebug( << it.key().toStdString() << " "
                << it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

        // set the constraint label
        QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
        vsConsLabel.push_back(sConsLabel);

        QStringList options = it.value().at(0).split(tr("+"), QString::SkipEmptyParts);
        std::vector<unsigned int> noptidx;
        for(int no=0; no < options.size(); ++no)
        {
            unsigned int idx = this->mslOptions.indexOf(
                    QRegExp(options.at(no), Qt::CaseInsensitive, QRegExp::FixedString));
            noptidx.push_back(idx);
            MosraLogDebug( << "option index for '" << options.at(no).toStdString() << "' = " << idx + 1 << endl);
        }
        vvnOptionIndex.push_back(noptidx);

        bool bok;
        double drhs = it.value().at(2).toDouble(&bok);
        vdRHS.push_back(drhs);

        // set the constraint type
        if (it.value().at(1).compare(tr("<=")) == 0)
            vnConsType.push_back(1);
        else if (it.value().at(1).compare(tr(">=")) == 0)
            vnConsType.push_back(2);
        else
            vnConsType.push_back(3);
        MosraLogDebug( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);
    }


    //----------------------------------------------
    double* pdRow = 0;
    int* piColno = 0;

    long lId;
    double dCoeff;

    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    int iOffset = this->miNumOptions;

    it = this->mmslFeatCons.constBegin();
    //-------------------------------------------------------------------- for each constraint
    for (int r=0; it != this->mmslFeatCons.constEnd(); ++it, ++r)
    {
        MosraLogDebug( << vsConsLabel.at(r).toStdString() << " - adding constraint" << endl);

        const long numOptions = vvnOptionIndex.at(r).size();
        pdRow = new double[numOptions];
        piColno = new int[numOptions];

        // determine initial offset for current option
        std::vector<long> vlCounter;
        for (int no=0; no < numOptions; ++no)
        {
            vlCounter.push_back(vvnOptionIndex.at(r).at(no)+1);
        }

        long nonHoleCounter = 0;

        // --------------------------------- for each feature
        for (int f=0; f < lNumCells; ++f)
        {
            if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
            {
                continue;
            }

            dCoeff = mDataSet->getDblValue(this->msAreaField, f);
            QString sConsVal = QString(tr("%1")).arg(dCoeff, 0, 'g');

            long coeffCounter = 0;
            for (int opt=0; opt < numOptions; ++opt)
            {
                switch(this->meDVType)
                {
                case NMMosra::NM_MOSO_BINARY:
                    pdRow[coeffCounter] = ::atof(sConsVal.toStdString().c_str());
                    break;
                default: // i.e. for all other DV  types (real, integer)
                    pdRow[coeffCounter] = 1;
                    break;
                }

                piColno[coeffCounter] = vlCounter[opt];
                ++coeffCounter;
            }


            // add the constraint
            this->mLp->AddConstraintEx(numOptions, pdRow, piColno
                                       , vnConsType.at(r), vdRHS.at(r));

            ++lRowCounter;
            QString rowlabel = QString("feat%1_%2").arg(f).arg(vsConsLabel.at(r));
            this->mLp->SetRowName(lRowCounter, rowlabel.toStdString());


            for (int opt=0; opt < numOptions; ++opt)
            {
                vlCounter[opt] += iOffset;
            }

            if (f % 500 == 0)
            {
                NMDebug(<< ".");
            }

            ++nonHoleCounter;
        }
        NMDebug(<< "finished!" << endl);

        delete[] pdRow;
        delete[] piColno;
    }

    this->mLp->SetAddRowmode(false);


    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::addExplicitAreaCons(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// get the hole array
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    bool hole = mDataSet->hasColumn("nm_hole");
//	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());

	std::vector<QString> vsConsLabel;
	std::vector<int> vnZoneLength;
	std::vector<QString> vsZoneField;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
	std::vector<unsigned int> vnConsType;
	std::vector<double> vdRHS;
//	long lNumCells = hole->GetNumberOfTuples();
    long lNumCells = mDataSet->getNumRecs();
	double dUserVal;
	double dUserArea = 0.0;
    double dAreaTotal = this->mdAreaTotal;
    double dAreaSelected = this->mdAreaSelected <= 0 ? this->mdAreaTotal : this->mdAreaSelected;

	QMap<QString, QStringList>::const_iterator it =
			this->mmslAreaCons.constBegin();

    MosraLogDebug( << this->mmslAreaCons.size() << " cons to process" << endl);

	// ----------------------------------------------------for each constraint
	for(int iConsCounter=1; it != this->mmslAreaCons.constEnd(); it++, iConsCounter++)
	{
        MosraLogDebug( << it.key().toStdString() << " "
				<< it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

		// set the constraint label
		QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
		vsConsLabel.push_back(sConsLabel);

		// look for a 'zoned' constraint
		QString zone;
		QStringList optzones;
        QStringList options;
		if (it.value().at(0).contains(tr(":"), Qt::CaseInsensitive))
		{
			optzones = it.value().at(0).split(tr(":"), QString::SkipEmptyParts);
            options = optzones.at(0).split(tr("+"), QString::SkipEmptyParts);
			zone = optzones.at(1);
		}
		else
		{
            options = it.value().at(0).split(tr("+"), QString::SkipEmptyParts);
			zone = "";
		}
		vsZoneField.push_back(zone);


        // set the user value
        bool bConvOK;
        dUserVal = it.value().at(2).toDouble(&bConvOK);
        double dtval = 0;
        MosraLogDebug( << "dUserVal (" << dUserVal << ") as '" << it.value().at(3).toStdString() << "' = " << dtval << endl);

        int maxzonelen = 0;

		// set the option index
        // and the right hand side value
        std::vector<unsigned int> noptidx;
        for(int no=0; no < options.size(); ++no)
        {
            unsigned int idx = this->mslOptions.indexOf(
                    QRegExp(options.at(no), Qt::CaseInsensitive, QRegExp::FixedString));
            noptidx.push_back(idx);
            MosraLogDebug( << "option index for '" << options.at(no).toStdString() << "' = " << idx + 1 << endl);

            QStringList ozspec;
            ozspec << options.at(no) << zone;

            double oval = this->convertAreaUnits(dUserVal, it.value().at(3), ozspec);
            dtval = oval > dtval ? oval : dtval;

            int zonelen = 0;
            if (!zone.isEmpty())
            {
                zonelen = this->mmslZoneLength.find(zone).value().find(options.at(no)).value();
            }
            maxzonelen = zonelen > maxzonelen ? zonelen : maxzonelen;
        }
        vvnOptionIndex.push_back(noptidx);
        vdRHS.push_back(dtval);

        if (zone.isEmpty())
        {
            vnZoneLength.push_back(this->mlNumOptFeat);
        }
        else
        {
            vnZoneLength.push_back(maxzonelen);
        }

        dUserArea += dtval;

		// set the constraint type
		if (it.value().at(1).compare(tr("<=")) == 0)
			vnConsType.push_back(1);
		else if (it.value().at(1).compare(tr(">=")) == 0)
			vnConsType.push_back(2);
		else
			vnConsType.push_back(3);
        MosraLogDebug( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);

	}

	// todo: we need to account for selected areas at one stage

//	if (dUserArea > dAreaSelected)
//	{
//		MosraLogError( << "The specified areal constraints are "
//				<< "inconsistent with the available area!");
//		return 0;
//	}

    double* pdRow = 0;
    int* piColno = 0;

	long lId;
	double dCoeff;

	// set add row mode
	this->mLp->SetAddRowmode(true);
	long lRowCounter = this->mLp->GetNRows();

	// calc the offset required to jump from one feature-option-column to the next
	// one; since we've got one binary conditional decision variable for each feature
	// we've got to add 1 to the offset;
    //int iOffset = this->miNumOptions + 1;
    int iOffset = this->miNumOptions;

	it = this->mmslAreaCons.constBegin();
	// ------------------------------------------------ for each constraint
	for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r)
	{
        MosraLogDebug( << vsConsLabel.at(r).toStdString() << " - adding constraint" << endl);

		// array defining zones for this constraints
		bool bZoneCons = false;
//		vtkStringArray* zoneAr;
        int zoneArIdx = -1;
		QString inLabel = vsConsLabel.at(r);

        const long numOptions = vvnOptionIndex.at(r).size();
        pdRow = new double[vnZoneLength.at(r) * numOptions];
        piColno = new int[vnZoneLength.at(r) * numOptions];

		if (!vsZoneField.at(r).isEmpty())
		{
//			zoneAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(vsZoneField.at(r).toStdString().c_str()));
//            zoneArIdx = mDataSet->getColumnIndex(vsZoneField.at(r));
			bZoneCons = true;

            inLabel = QString(tr("%1")).arg(vsConsLabel.at(r));
		}

		// determine the initial offset for the current option (i.e. the first column number
		// of the lp-matrix); since the option index is 0-based, but the columns are 1-based
		// we've got to add 1!
        std::vector<long> vlCounter;
        for(int no=0; no < numOptions; ++no)
        {
            vlCounter.push_back(vvnOptionIndex.at(r).at(no) + 1);
        }

		long nonHoleCounter = 0;
        long coeffCounter = 0;
		// ---------------------------------------------for each feature
		for (int f=0; f < lNumCells; f++)
		{
			// skip holes
//			if (hole->GetTuple1(f))
            if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
            {
				continue;
            }

//			dCoeff = areas->GetTuple1(f);
            dCoeff = mDataSet->getDblValue(this->msAreaField, f);
			QString sConsVal = QString(tr("%1")).arg(dCoeff, 0, 'g');


            //const unsigned int& optidx = vvnOptionIndex.at(r).at(no);
            //const long& lCounter = vlCounter.at(no);
            // set coefficients depending on whether we've got a zoned constraint or not
            if (bZoneCons)
            {
                for (int no=0; no < numOptions; ++no)
                {
                    // set coefficients for zone polygons
                    std::string zoneArVal = mDataSet->getStrValue(vsZoneField.at(r), f).toStdString();

//                    if (zoneAr->GetValue(f).find(this->mslOptions.at(vvnOptionIndex.at(r).at(no)).toStdString())
//                            != std::string::npos)
                    if (zoneArVal.find(this->mslOptions.at(vvnOptionIndex.at(r).at(no)).toStdString())
                              != std::string::npos)
                    {
                        // set the coefficient
                        switch(this->meDVType)
                        {
                        case NMMosra::NM_MOSO_BINARY:
                            bool bok;
                            pdRow[coeffCounter] = sConsVal.toDouble(&bok);
                            break;
                        default: // i.e. for all other DV  types (real, integer)
                            pdRow[coeffCounter] = 1;
                            break;
                        }

                        // set the column number
                        piColno[coeffCounter] = vlCounter[no];
                        ++coeffCounter;
                    }
                }
            }
            else // set the coefficients for a non-zone constraint
            {
                // set the coefficient
                switch(this->meDVType)
                {
                case NMMosra::NM_MOSO_BINARY:
                    pdRow[coeffCounter] = ::atof(sConsVal.toStdString().c_str());
                    break;
                default: // i.e. for all other DV  types (real, integer)
                    pdRow[coeffCounter] = 1;
                    break;
                }

                // set the column number
                piColno[coeffCounter] = vlCounter[0];
                ++coeffCounter;
            }

            for(int no=0; no < numOptions; ++no)
            {
                vlCounter[no] += iOffset;
            }

            if (f % 500 == 0)
            {
                NMDebug(<< ".");
            }

			// increment the counter for valid cells (i.e. valid spatial opt features
			++nonHoleCounter;
		}

        NMDebug(<< " finished!" << endl);

		// add the constraint
        this->mLp->AddConstraintEx((vnZoneLength.at(r) * numOptions),
				pdRow, piColno, vnConsType.at(r),
				vdRHS.at(r));

		// increment the row counter
		++lRowCounter;

		// assign a label for this constraint and store label for
		// retrieval of post-optimisation constraint values
		this->mLp->SetRowName(lRowCounter, inLabel.toStdString());
		this->mmslAreaConsLabel.append(inLabel);

        // free memory
        delete[] pdRow;
        delete[] piColno;
	}


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
    //    double* pdRow = new double[this->miNumOptions + 1];
    //	int* piColno = new int[this->miNumOptions + 1];

    double* pdRow = new double[this->miNumOptions];
    int* piColno = new int[this->miNumOptions];

	// get the hole array
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    bool hole = mDataSet->hasColumn("nm_hole");
//	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());

//	long lNumCells = hole->GetNumberOfTuples();
    long lNumCells = mDataSet->getNumRecs();
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
//		if (hole->GetTuple1(f))
        if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
        {
			continue;
        }

		// get the area of the current feature
//		dConsVal = areas->GetTuple1(f);
        dConsVal = mDataSet->getDblValue(this->msAreaField, f);

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

            //MosraLogInfo( << "feat cons: option | colps = " << option << " | " << lCounter+1 << endl);

		}

		// ......................................................................................
		// add the first constraint: SUM(x_i_r) - A_i * b_i >= 0

		// set the coefficient for the binary decision variable for the current feature
//		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;
//		piColno[this->miNumOptions] = colpos;

//		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
//				piColno, 2, 0);

        this->mLp->AddConstraintEx(this->miNumOptions, pdRow,
                piColno, 1, ::atof(sConsVal.toStdString().c_str()));


		// increment row (constraint) counter
		lRowCounter++;

		// label the currently added constraint
		QString sRowName = QString(tr("Feature_%1a")).arg(f);
		this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

//		// ......................................................................................
//		// add the second constraint: SUM(x_i_r) - A_i * b_i <= 0
//		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;

//		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
//				piColno, 1, 0);
//		lRowCounter++;

//		sRowName = QString(tr("Feature_%1b")).arg(f);
//		this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

		// ........................................................................................
		// column position counter
        //colpos++;

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
                MosraLogError( << "CRITERIA_CONSTRAINTS: " << crilabit.key().toStdString()
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
                MosraLogError( << "CRITERIA_CONSTRAINTS: " << label.toStdString() << ": invalid value '"
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
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* areas = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
//	vtkDataArray* hole = dsAttr->GetArray("nm_hole");
//	long lNumCells = hole->GetNumberOfTuples();

    bool hole = mDataSet->hasColumn("nm_hole");
    long lNumCells = mDataSet->getNumRecs();

	// iterate over constraints and process them one at a time
	// (or should we process them while looping over all features)?

	this->mLp->SetAddRowmode(true);
	long lRowCounter = this->mLp->GetNRows();

	for (int labelidx = 0; labelidx < vLabels.size(); ++labelidx)
	{
        MosraLogDebug(<< "preparing constraint " << vLabels[labelidx].toStdString() << endl);

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
//			if (hole->GetTuple1(f))
            if (hole && mDataSet->getIntValue("nm_hole", f) == 1)
            {
				continue;
            }

			for(int i=0; i < numCriOptions; ++i, ++arpos)
			{
				int optIdx = landUseIdxs[i];

//				vtkDataArray* performanceIndicator = dsAttr->GetArray(
//						fieldNames[i].toStdString().c_str());
                int performanceIndicatorIdx = mDataSet->getColumnIndex(fieldNames[i]);

				double coeff = 0.0;
				bool bAddCoeff = true;

				// check whether, we've got zoning and whether the criterion constraint
				// for the current land use option shall be restricted to this zone
				if (!vZones.at(labelidx).isEmpty())
				{
//					vtkStringArray* zoneAr = vtkStringArray::SafeDownCast(
//							dsAttr->GetAbstractArray(vZones.at(labelidx).toStdString().c_str()));
                    std::string zoneArVal = mDataSet->getStrValue(vZones.at(labelidx), f).toStdString();
//					if (zoneAr->GetValue(f).find(this->mslOptions.at(optIdx).toStdString()) == std::string::npos)
//						bAddCoeff = false;
                    if (zoneArVal.find(this->mslOptions.at(optIdx).toStdString()) == std::string::npos)
                    {
                        bAddCoeff = false;
                    }
				}

				if (bAddCoeff)
				{
					switch(this->meDVType)
					{
					case NMMosra::NM_MOSO_BINARY:
//						coeff = areas->GetTuple1(f) * performanceIndicator->GetTuple1(f);
                        coeff = mDataSet->getDblValue(this->msAreaField, f) * mDataSet->getDblValue(performanceIndicatorIdx, f);
						break;
					default:
//						coeff = performanceIndicator->GetTuple1(f);
                        coeff = mDataSet->getDblValue(performanceIndicatorIdx, f);
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
            //colPos += this->miNumOptions + 1;
            colPos += this->miNumOptions;
		}
		NMDebug(<< " finished!" << std::endl);

		// add constraint
        MosraLogDebug(<< "adding constraint to LP ..." << std::endl);
		this->mLp->AddConstraintEx(numCriOptions * this->mlNumOptFeat,
				pdRow, piColno, vOperators[labelidx], vRHS[labelidx]);

		// increment the row counter
		++lRowCounter;


		//  set the constraint label
        MosraLogDebug(<< "labeling constraint ..." << std::endl);
		this->mLp->SetRowName(lRowCounter, vLabels[labelidx].toStdString().c_str());

		delete[] pdRow;
		delete[] piColno;

	}

	// turn off rowmode
	this->mLp->SetAddRowmode(false);

	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

vtkSmartPointer<vtkTable> NMMosra::sumResults(vtkSmartPointer<vtkTable>& changeMatrix)
{
	NMDebugCtx(ctxNMMosra, << "...");

    MosraLogDebug(<< "getting input arrays (attributes) ..." << std::endl);
	// get hold of the input attributes we need
//	vtkDataSet* ds = const_cast<vtkDataSet*>(this->mDataSet);
//	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
//	vtkDataArray* holeAr = dsAttr->GetArray("nm_hole");
//	vtkDataArray* areaAr = dsAttr->GetArray(this->msAreaField.toStdString().c_str());
//	vtkStringArray* luAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray(
//			this->msLandUseField.toStdString().c_str()));

    int luArIdx = mDataSet->getColumnIndex(this->msLandUseField);
    int areaArIdx = mDataSet->getColumnIndex(this->msAreaField);
    bool hole = mDataSet->hasColumn("nm_hole");

//	vtkStringArray* optStrAr = vtkStringArray::SafeDownCast(dsAttr->GetAbstractArray("OPT_STR"));
//    if (optStrAr == 0)
    int optStrArIdx = mDataSet->getColumnIndex("OPT_STR");
    if (optStrArIdx == -1)
	{
        MosraLogError( << "failed to fetch result array 'OPT_STR'!");
		return 0;
	}

//	QList<vtkDataArray*> optValArs;
    QList<int> optValArsIdx;
	for (int option=0; option < this->miNumOptions; ++option)
	{
		QString arName = QString(tr("OPT%1_VAL")).arg(option+1);
//		vtkDataArray* optvalar = dsAttr->GetArray(arName.toStdString().c_str());
//		if (optvalar == 0)
        int idx = mDataSet->getColumnIndex(arName);
        if (idx == -1)
		{
            MosraLogError( << "failed to fetch result array '"
					<< arName.toStdString().c_str() << "'!");
			return 0;
		}
//		optValArs.append(optvalar);
        optValArsIdx.append(idx);
	}

    /* ##################################################################################
     *                          CREATE PERFORMANCE SUMMARY TABLE
     * ################################################################################## */

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

    MosraLogDebug(<< "creating the results table..." << std::endl);

	// determine the number of rows and columns
	//int resNumCols = (this->mmslCriteria.size() + this->mmslCriCons.size()) * 3 +1;

	// we sum over each resource option (land use), all mixed landuses (mixed) and
	// the overall total
	//int resNumRows = this->miNumOptions + 2; // with mixed
	int resNumRows = this->miNumOptions + 1;   // without mixed

    // note: each zone defines an <zone>:IN and <zone>:OUT zone
    // (expecting boolean values {0,1})

    int numZones = 1; // we've got at leat the total area!
    for (int nz=0; nz < this->mslPerfSumZones.size(); ++nz)
    {
        resNumRows  += this->miNumOptions + 1;
        numZones += 1;
    }

//	long ncells = holeAr->GetNumberOfTuples();
    long ncells = mDataSet->getNumRecs();

	vtkSmartPointer<vtkTable> restab = vtkSmartPointer<vtkTable>::New();
	restab->SetNumberOfRows(resNumRows);

	// add the row header array (land use options) + total
	vtkSmartPointer<vtkStringArray> rowheads = vtkSmartPointer<vtkStringArray>::New();
	rowheads->SetName("Resource");
	rowheads->SetNumberOfComponents(1);
	rowheads->SetNumberOfTuples(resNumRows);

    int rowCount = 0;
    QString rowHead;
    for (int nz=0; nz < numZones; ++nz)
    {
        for (int r=0; r < this->miNumOptions; ++r)
        {
            if (nz == 0)
            {
                rowHead = this->mslOptions.at(r);
            }
            else
            {
                rowHead = QString("%1:%2")
                        .arg(this->mslPerfSumZones.at(nz-1))
                        .arg(this->mslOptions.at(r));
            }
            rowheads->SetValue(rowCount, (const char*)rowHead.toStdString().c_str());
            //MosraLogDebug(<< "RowHead #" << rowCount << ": " << rowHead.toStdString() << std::endl);
            ++rowCount;
        }

        if (nz == 0)
        {
            rowHead = QString("Total");
        }
        else
        {
            rowHead = QString("%1:Total")
                    .arg(this->mslPerfSumZones.at(nz-1));
        }
        rowheads->SetValue(rowCount, (const char*)rowHead.toStdString().c_str());
        //MosraLogDebug(<< "RowHead #" << rowCount << ": " << rowHead.toStdString() << std::endl);
        ++rowCount;

    }

    //rowheads->SetValue(resNumRows-2, "Mixed");
    //rowheads->SetValue(resNumRows-1, "Total");
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

    //	// add columns for attribute constraints
    //	QMultiMap< QString, QMap< QString, QStringList > >::const_iterator constrit =
    //			this->mmslCriCons.constBegin();
    //	for (; constrit != this->mmslCriCons.constEnd(); ++constrit)
    //	{
    //		criit = constrit.value().constBegin();
    //		for (; criit != constrit.value().constEnd(); ++criit)
    //		{
    //			for (int s=0; s < colsuffix.size(); ++s)
    //			{
    //				QString colname = QString(tr("c_%1%2_%3")).arg(constrit.key()).
    //						arg(criit.key()).arg(colsuffix.at(s));
    //				vtkSmartPointer<vtkDoubleArray> daa = vtkSmartPointer<vtkDoubleArray>::New();
    //                daa->SetName(colname.toLatin1());
    //				daa->SetNumberOfComponents(1);
    //				daa->SetNumberOfTuples(resNumRows);
    //				daa->FillComponent(0, 0);

    //				restab->AddColumn(daa);
    //			}
    //		}
    //	}

    /* ##################################################################################
     *                          CREATE RESOURCE CHANGE MATRIX
     * ################################################################################## */
    changeMatrix = vtkSmartPointer<vtkTable>::New();
    changeMatrix->SetNumberOfRows(this->miNumOptions+2);

    // add the row header array (land use options) + total
    vtkSmartPointer<vtkStringArray> chngheads = vtkSmartPointer<vtkStringArray>::New();
    chngheads->SetName("from/to");
    chngheads->SetNumberOfComponents(1);
    chngheads->SetNumberOfTuples(this->miNumOptions+2);

    for (int no=0; no < this->miNumOptions; ++no)
    {
        chngheads->SetValue(no, (const char*)this->mslOptions.at(no).toLatin1());
    }
    chngheads->SetValue(this->miNumOptions, "other");
    chngheads->SetValue(this->miNumOptions+1, "SUM");
    changeMatrix->AddColumn(chngheads);

    for (int no=0; no < this->miNumOptions+2; ++no)
    {
        vtkSmartPointer<vtkDoubleArray> car = vtkSmartPointer<vtkDoubleArray>::New();
        if (no < this->miNumOptions)
        {
            car->SetName((const char*)this->mslOptions.at(no).toLatin1());
        }
        else if (no == this->miNumOptions)
        {
            car->SetName("other");
        }
        else //if (no == this->miNumOptions+1)
        {
            car->SetName("SUM");
        }
        car->SetNumberOfComponents(1);
        car->SetNumberOfTuples(this->miNumOptions+2);
        car->FillComponent(0,0);
        changeMatrix->AddColumn(car);
    }


    /* ##################################################################################
     *                          SUMMARISE PERFORMANCES AND FILL CHANGE MATRIX
     * ################################################################################## */

    MosraLogDebug(<< "summarising results ..." << std::endl << std::endl);


// don't need these really, and it only breaks the WIN32 compilation
//	int ind = nmlog::nmindent + 1;
//	int ind2 = ind+1;

    int resRec = 0;
    int rec = 0;
    int zoneCounter = 0;
    // loop over the data set and sequentially process (update) all target columns
	// only after the final iteration, the table holds correct values;
	for (long cell=0; cell < ncells; ++cell)
	{
		// don't tumble into holes
//		if (holeAr->GetTuple1(cell))
        if (hole && mDataSet->getIntValue("nm_hole", cell) == 1)
        {
			continue;
        }

		// read the current and optimised resource
//		QString curResource = (const char*)luAr->GetValue(cell);
        QString curResource = mDataSet->getStrValue(luArIdx, cell);
		// make sure, we're not fooled by any leading or trailing white spaces
		curResource = curResource.simplified();
//		QString optResource = (const char*)optStrAr->GetValue(cell);
        QString optResource = mDataSet->getStrValue(optStrArIdx, cell);
		QStringList optResList = optResource.split(tr(" "), QString::SkipEmptyParts);

        // ===============================================================================
        //                      CHANGE ANALYSIS
        // ===============================================================================

        QVector<int> toIdx;

        // set from initially to 'other'
        int fromIdx = this->miNumOptions;
        for (int no=0; no < this->miNumOptions; ++no)
        {
            if (curResource.compare(this->mslOptions.at(no), Qt::CaseInsensitive) == 0)
            {
                fromIdx = no;
            }

            if (optResList.contains(this->mslOptions.at(no)))
            {
                toIdx.push_back(no);
            }
        }
        // add 'other' land use as recipient, if the optResList doesn't
        // contain curResource
        if (toIdx.size() == 0)
        {
            toIdx.push_back(this->miNumOptions);
        }

        int coloff = 1;
        for (int t=0; t < toIdx.size(); ++t)
        {
            double chngVal = changeMatrix->GetValue(fromIdx, toIdx.at(t)+coloff).ToDouble();
            double newValue = 0;
            if (toIdx.at(t) < this->miNumOptions)
            {
//                newValue = optValArs.at(toIdx.at(t))->GetTuple1(cell);
                newValue = mDataSet->getDblValue(optValArsIdx.at(toIdx.at(t)), cell);
            }
            else   // get the area value from the AreaHa field for 'other' land uses
            {
//                newValue = areaAr->GetTuple1(cell);
                newValue = mDataSet->getDblValue(areaArIdx, cell);
            }

            chngVal += newValue;
            changeMatrix->SetValue(fromIdx, toIdx.at(t)+coloff, vtkVariant(chngVal));
        }

        // DEBUG
        //		NMDebugInd(ind, << "curResource: " << curResource.toStdString() << endl);
        //		NMDebugInd(ind, << "optResource: " << optResource.toStdString() << endl);
        //		NMDebugInd(ind, << "optResList: ");
        //		for (int r=0; r < optResList.size(); ++r)
        //		{
        //			NMDebug(<< "-" << optResList.at(r).toStdString() << "-");
        //		}
        //		NMDebug(<< endl << endl);

        // ===============================================================================
        //                      PERFORMANCE ANALYSIS (TOTAL and per ZONE)
        // ===============================================================================
        for (int zone=0; zone < numZones; ++zone)
        {
            rec = zone * (this->miNumOptions+1);

//            vtkDataArray* zoneAr = 0;
            int zoneArIdx = -1;
            if (zone >= 1)
            {
                zoneArIdx = mDataSet->getColumnIndex(mslPerfSumZones.at(zone-1));
//                zoneAr = dsAttr->GetArray((const char*)mslPerfSumZones.at(zone-1).toStdString().c_str());
            }

//            if (zoneAr)
            if (zoneArIdx >= 0)
            {
//                if (zoneAr->GetTuple1(cell) == 0)
                if (mDataSet->getDblValue(zoneArIdx, cell) == 0)
                {
                    continue;
                }
            }

            resRec = rec;
            for (int option=0; option < this->miNumOptions; ++option)
            {
                // common vars
//                vtkDataArray* evalAr;
                int evalArIdx = -1;
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
//                    curArea = areaAr->GetTuple1(cell);
                    curArea = mDataSet->getDblValue(areaArIdx, cell);
                    curaccumArea = curArea + restab->GetValue(resRec, 1).ToDouble();
                    restab->SetValue(resRec,1, vtkVariant(curaccumArea));

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

//                        evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
                        evalArIdx = mDataSet->getColumnIndex(evalField);

//                        performanceValue = evalAr->GetTuple1(cell) * curArea +
//                                restab->GetValue(resRec, coloffset*4 + 1).ToDouble();
                        performanceValue = mDataSet->getDblValue(evalArIdx, cell) * curArea +
                                restab->GetValue(resRec, coloffset*4 + 1).ToDouble();

                        restab->SetValue(resRec, coloffset*4 + 1, vtkVariant(performanceValue));

                        //					NMDebugInd(ind2, << "performance measure: " << performanceValue
                        //							<< " goes into row,col: " << option << ", " << (coloffset*4+1) << endl);

                    }
                    //				NMDebug(<< endl);

                    //                        // update the constraints evaluations
                    //                        constrit =  this->mmslCriCons.constBegin();
                    //                        coloffset = this->mmslEvalFields.size() + 1;
                    //                        for (; constrit != this->mmslCriCons.constEnd(); ++constrit, ++coloffset)
                    //                        {
                    //                            //					NMDebugInd(ind2, << "checking attr cons: " << constrit.key().toStdString() << endl);

                    //                            QMap<QString, QStringList>::const_iterator lufmap =
                    //                                    constrit.value().constBegin();
                    //                            for (; lufmap != constrit.value().constEnd(); ++lufmap)
                    //                            {
                    //                                //						NMDebugInd(2, << "land use field map: " << lufmap.key().toStdString() << " " << lufmap.value().join(tr(" ")).toStdString() << endl);

                    //                                evalField = tr("");

                    //                                // account for zoning
                    //                                QStringList zonespec;
                    //                                QString opt;
                    //                                QString zone;
                    //                                if (lufmap.key().contains(tr(":"), Qt::CaseInsensitive))
                    //                                {
                    //                                    zonespec = lufmap.key().split(tr(":"), QString::SkipEmptyParts);
                    //                                    opt = zonespec.at(0);
                    //                                    zone = zonespec.at(1);
                    //                                }
                    //                                else
                    //                                {
                    //                                    opt = lufmap.key();
                    //                                    zone = "";
                    //                                }

                    //                                // identify the right field for looking up the performance value
                    //                                if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
                    //                                    evalField = lufmap.value().at(option);
                    //                                else if (opt.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
                    //                                    evalField = lufmap.value().at(0);


                    //                                evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
                    //                                if (evalAr == 0)
                    //                                    continue;

                    //                                performanceValue = evalAr->GetTuple1(cell) * curArea +
                    //                                        restab->GetValue(resRec, coloffset*4 + 1).ToDouble();
                    //                                restab->SetValue(resRec, coloffset*4 + 1, vtkVariant(performanceValue));

                    //                                //						NMDebugInd(ind2, << "attr performance measure: " << performanceValue
                    //                                //								<< " goes into row,col: " << option << ", " << (coloffset*4+1) << endl << endl);

                    //                            }
                    //                        }
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
//                    optArea = optValArs.at(option)->GetTuple1(cell);
                    optArea = mDataSet->getDblValue(optValArsIdx.at(option), cell);

                    //				// track mixed land-use separately
                    //				if (optResList.size() > 1)
                    //				{
                    //					restab->SetValue(resNumRows-2, 2,
                    //							vtkVariant(optArea + restab->GetValue(resNumRows-2, 2).ToDouble()));
                    //				}

                    // define variable for row index in result table
                    int resTabRow = resRec;//optResList.size() > 1 ? resNumRows-2 : option;

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
//                        evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
                        evalArIdx = mDataSet->getColumnIndex(evalField);

//                        performanceValue = evalAr->GetTuple1(cell) * optArea +
//                                restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();
                        performanceValue = mDataSet->getDblValue(evalArIdx, cell) * optArea +
                                restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();
                        restab->SetValue(resTabRow, coloffset*4 + 2, vtkVariant(performanceValue));

                        //					NMDebugInd(ind2, << "performance measure: " << performanceValue
                        //							<< "goes into row,col: " << resTabRow << ", " << (coloffset*4+2) << endl);
                    }

                    //                        // update the constraints evaluations
                    //                        constrit =  this->mmslCriCons.constBegin();
                    //                        coloffset = this->mmslEvalFields.size() + 1;
                    //                        for (; constrit != this->mmslCriCons.constEnd(); ++constrit, ++coloffset)
                    //                        {
                    //                            //					NMDebugInd(ind2, << "checking attr cons: " << constrit.key().toStdString() << endl);

                    //                            QMap<QString, QStringList>::const_iterator lufmap =
                    //                                    constrit.value().constBegin();
                    //                            for (; lufmap != constrit.value().constEnd(); ++lufmap)
                    //                            {

                    //                                //						NMDebugInd(2, << "land use field map: " << lufmap.key().toStdString() << " " << lufmap.value().join(tr(" ")).toStdString() << endl);

                    //                                evalField = tr("");

                    //                                // account for zoning
                    //                                QStringList zonespec;
                    //                                QString opt;
                    //                                QString zone;
                    //                                if (lufmap.key().contains(tr(":"), Qt::CaseInsensitive))
                    //                                {
                    //                                    zonespec = lufmap.key().split(tr(":"), QString::SkipEmptyParts);
                    //                                    opt = zonespec.at(0);
                    //                                    zone = zonespec.at(1);
                    //                                }
                    //                                else
                    //                                {
                    //                                    opt = lufmap.key();
                    //                                    zone = "";
                    //                                }

                    //                                if (optResList.contains(opt, Qt::CaseInsensitive))
                    //                                {

                    //                                    if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
                    //                                        evalField = lufmap.value().at(option);
                    //                                    else if (opt.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
                    //                                        evalField = lufmap.value().at(0);

                    //                                    evalAr = dsAttr->GetArray(evalField.toStdString().c_str());
                    //                                    if (evalAr == 0)
                    //                                        continue;

                    //                                    performanceValue = evalAr->GetTuple1(cell) * optArea +
                    //                                            restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();
                    //                                    restab->SetValue(resTabRow, coloffset*4 + 2, vtkVariant(performanceValue));

                    //                                    //						NMDebugInd(ind2, << "attr performance measure: " << performanceValue
                    //                                    //								<< "goes into row,col: " << resTabRow << ", " << (coloffset*4+2) << endl << endl);

                    //                                }
                    //                            }
                    //                        }
                }
                ++resRec;
            }
        }

		if (cell % 200 == 0)
		{
			NMDebug(<< ".");
		}
	}
	NMDebug(<< " finished!" << std::endl);

    // ===============================================================================
    //                  CALCULATE PERFORMANCE TOTALS
    // ===============================================================================


    // sum totals
    int ncols = restab->GetNumberOfColumns();
    for (int sp=1; sp < ncols; ++sp)
    {
        for (int zone=0; zone < numZones; ++zone)
        {
            double sum = 0.0;
            int rec = zone * (this->miNumOptions+1);
            for (int opt=0; opt < this->miNumOptions; ++opt)
            {
                sum += restab->GetValue(rec, sp).ToDouble();
                ++rec;
            }
            restab->SetValue(rec, sp, vtkVariant(sum));
        }
    }

    // calc differences
    for (int zone=0; zone < numZones; ++zone)
    {
        int rec = zone * (this->miNumOptions+1);
        for (int opt=0; opt < this->miNumOptions+1; ++opt)
        {
            for (int sp=0; sp < ncols-3; sp += 4)
            {
                double diff = restab->GetValue(rec, sp+2).ToDouble() -
                        restab->GetValue(rec, sp+1).ToDouble();
                restab->SetValue(rec, sp+3, vtkVariant(diff));
                double denom = restab->GetValue(rec, sp+1).ToDouble();
                if (denom)
                {
                    double rel = diff / denom * 100.0;
                    restab->SetValue(rec, sp+4, vtkVariant(rel));
                }
                else if (diff && denom == 0)
                {
                    restab->SetValue(rec, sp+4, vtkVariant(100.0));
                }
                else
                {
                    restab->SetValue(rec, sp+4, vtkVariant(0.0));
                }
            }
            ++rec;
        }
    }

    // ===============================================================================
    //                      CALCULATE CHANGE TOTALS
    // ===============================================================================

    // calc row totals
    int coloff = 1;
    for (int r=0; r < this->miNumOptions+1; ++r)
    {
        double val = 0;
        for (int c=0; c < this->miNumOptions+1+coloff; ++c)
        {
            val += changeMatrix->GetValue(r, c+coloff).ToDouble();
        }
        changeMatrix->SetValue(r, this->miNumOptions+1+coloff, vtkVariant(val));
    }

    for (int c=0; c < this->miNumOptions+1+coloff; ++c)
    {
        double val = 0;
        for (int r=0; r < this->miNumOptions+1; ++r)
        {
            val += changeMatrix->GetValue(r, c+coloff).ToDouble();
        }
        changeMatrix->SetValue(this->miNumOptions+1, c+coloff, vtkVariant(val));
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
//	MosraLogInfo( << "populating table ..." << endl);
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

    NMMosra *mosra = static_cast<NMMosra*>(userhandle);
	return mosra->isSolveCanceled();
}

void
NMMosra::lpLogCallback(lprec *lp, void *userhandle, char *buf)
{
    NMMosra* mosra = static_cast<NMMosra*>(userhandle);
    if (mosra)
    {
        mosra->forwardLpLog(buf);
    }
}

void NMMosra::forwardLpLog(const char* log)
{
    MosraLogInfo(<< "lp_solve: " << log);
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
