/*****************************t*************************************************
 * Created by Alexander Herzige
 * 2013 Landcare Research New Zealand Ltd
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
 * NMDataComponent.cpp
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */
#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include <string>
#include <sstream>

#include "NMModelController.h"
#include "NMDataComponent.h"
#include "NMIterableComponent.h"
#include "NMMfwException.h"
#include "otbSQLiteTable.h"
#include "itkNMLogEvent.h"

const std::string NMDataComponent::ctx = "NMDataComponent";

NMDataComponent::NMDataComponent(QObject* parent)
    : NMModelComponent(parent)
{
    this->setParent(parent);
    this->initAttributes();
}

NMDataComponent::~NMDataComponent()
{
    reset();
}

void
NMDataComponent::initAttributes(void)
{
    NMModelComponent::initAttributes();
    mInputCompName.clear();
    mLastInputCompName.clear();
    mSourceMTime.setMSecsSinceEpoch(0);
    mDataWrapper.clear();
    mInputOutputIdx = 0;
    mLastInputOutputIdx = 0;
    mParamPos = 0;
    mbLinked = false;
    mTabMinPK = itk::NumericTraits<long long>::max();
    mTabMaxPK = itk::NumericTraits<long long>::NonpositiveMin();
    mIsStreamable = false;
}

void
NMDataComponent::setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg, const QString& name)
{
    // note QSharedPointer takes care of management, so no
    // parent required, actually if we had a parent
    // we'd get crash with double free error

    if (!mDataWrapper.isNull())
    {
        this->reset();
    }
    mDataWrapper = inputImg;

    if (mIsStreamable)
    {
        mDataWrapper->setIsStreaming(true);
    }

    if (!mDataWrapper.isNull())
    {
        if (mDataWrapper->getDataObject())
        {
            // add an observer to keep track of any internal log worthy events
            mObserver = ObserverType::New();
            mObserver->SetCallbackFunction(this, &NMModelComponent::ProcessLogEvent);
            mDataWrapper->getDataObject()->AddObserver(itk::NMLogEvent(), mObserver);
        }


        // in case we've got a table and getModelParameter is called
        // fetch the minimum value of its primary key so we're getting
        // the right parameter
        if (mDataWrapper->getOTBTab().IsNotNull())
        {
            mTabMinPK = mDataWrapper->getOTBTab()->GetMinPKValue();
            mTabMaxPK = mDataWrapper->getOTBTab()->GetMaxPKValue();
        }
    }

    emit NMDataComponentChanged();
}

void
NMDataComponent::linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");
    NMMfwException e(NMMfwException::NMDataComponent_InvalidParameter);
    e.setSource(this->objectName().toStdString());
    std::stringstream msg;

    this->processUserID();
    this->mParamPos = step;
    if (mbLinked)
    {
        NMDebugAI(<< "seems as if we've been linked in already!" << endl);
        NMDebugCtx(ctx, << "done!");
        return;
    }


    if (mInputs.size() == 0)
    {
        // if we haven't got an input component set-up,
        // we're also happy with data which was set from
        // outside the pipeline
        if (!mDataWrapper.isNull())
        {
            mbLinked = true;
            NMDebugCtx(ctx, << "...");
            return;
        }
        else
        {
            e.setDescription("List of input specs is empty!");
            NMDebugCtx(ctx, << "done!");
            throw e;
        }
    }

    // we apply the NM_USE_UP parameter handling
    // policy and store the input step as mParamPos
    // for use in the fetch data method
    if (step > mInputs.size()-1)
        step = mInputs.size()-1;

    QStringList listOfSpecs = mInputs.at(step);
    if (listOfSpecs.size() == 0)
    {
        e.setDescription("No input component defined!");
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    // since the data component only accepts one input
    // we just grab the first
    QString inputSpec;
    inputSpec = listOfSpecs.at(0);

    if (inputSpec.isEmpty())
    {
        e.setDescription("No input component defined!");
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    this->mLastInputCompName = this->mInputCompName;
    this->mLastInputOutputIdx = this->mInputOutputIdx;
    if (inputSpec.contains(":"))
    {
        QStringList inputSrcParams = inputSpec.split(":", Qt::SkipEmptyParts);
        mInputCompName = inputSrcParams.at(0);

        bool bOK = false;
        if (inputSrcParams.size() == 2)
        {
            mInputOutputIdx = inputSrcParams.at(1).toInt(&bOK);
            if (!bOK)
            {
                msg << "Failed to interpret input source parameter '"
                    << inputSpec.toStdString() << "'";
                e.setDescription(msg.str());
                NMDebugCtx(ctx, << "done!");
                throw e;
            }
        }
    }
    else
    {
        mInputCompName = inputSpec;
        mInputOutputIdx = 0;
    }

    // fetch the data from the source object
    NMModelComponent* inComp = this->getModelController()->getComponent(mInputCompName);

    if (inComp == 0)
    {
        msg << "The specified input component '"
            << mInputCompName.toStdString() << "' couldn't be found!";
        e.setDescription(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    this->mbLinked = true;

    NMDebugCtx(ctx, << "done!");
}

QVariant
NMDataComponent::getModelParameter(const QString &paramSpec)
{
    QVariant param;
    QString demsg;

    if (paramSpec.isEmpty())
    {
        demsg = QString("ERROR - %1::getModelParameter(%2) - %3!")
                .arg(this->objectName())
                .arg(paramSpec)
                .arg("invalid parameter specification!");
        return param = QVariant::fromValue(demsg);
    }


    // break up paramSpec, which can be either
    //   <columnName>:<rowNumber> or
    //   <objectProperty>:<index>
    QStringList specList = paramSpec.split(":", Qt::SkipEmptyParts);

    // fetch table, if applicable
    otb::AttributeTable::Pointer tab;
    if (    this->mDataWrapper.isNull()
        ||  this->mDataWrapper->getOTBTab().IsNull()
       )
    {
        this->update(this->getModelController()->getRepository());

        if (    this->mDataWrapper.isNull()
            ||  this->mDataWrapper->getOTBTab().IsNull()
           )
        {
            demsg = "ERROR - no parameter table found!";
            //return QVariant::fromValue(dmesg);
        }
        else
        {
            tab = this->mDataWrapper->getOTBTab();
        }
    }
    else
    {
        tab = this->mDataWrapper->getOTBTab();
    }

    // go and fetch the table value
    long long row = -1;
    long long recnum = tab.IsNotNull() ? tab->GetNumRows() : 0;

    // enable table attribute queries:
    // <obj>:rowcount       --> number of rows
    // <obj>:columncount    --> number of columns

    if (specList.at(0).compare("rowcount", Qt::CaseInsensitive) == 0)
    {
        param = QVariant::fromValue(recnum);
        return param;
    }
    else if (specList.at(0).compare("columncount", Qt::CaseInsensitive) == 0)
    {
        param = QVariant::fromValue(tab->GetNumCols());
        return param;
    }

    // see whether we've got a valid table column;
    int colidx = -1;
    if (tab.IsNotNull())
    {
        colidx = tab->ColumnExists(specList.at(0).toStdString().c_str());
        demsg = QString("couldn't find column '%1'!").arg(specList.at(0));
    }

    // if either the table is valid or the 'column name',
    // we try and fetch an object property
    if (colidx < 0)
    {
        param = NMModelComponent::getModelParameter(paramSpec);
        if (param.isValid() && !param.toString().startsWith("ERROR"))
        {
            return param;
        }

        QString msg = QString("ERROR - %1::getModelParameter(%2) - %3!")
                .arg(this->objectName())
                .arg(paramSpec)
                .arg(demsg);
        return param = QVariant::fromValue(msg);
    }

    //if (specList.size() == 2)
    {
        bool bthrow = false;
        bool bok = true;

        // receiving 1-based row number
        row = specList.size() == 2 ? specList.at(1).toLongLong(&bok) : 1;

        // if haven't got a numeric index, the user wants the numeric index for the given
        // value in column 'colidx'
        if (!bok)
        {
            QString pkcol = tab->GetPrimaryKey().c_str();
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
            {
                otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(tab.GetPointer());
                std::stringstream wclause;

                if (tab->GetColumnType(colidx) == otb::AttributeTable::ATTYPE_STRING)
                {
                    wclause << specList.at(0).toStdString().c_str() << " = '" << specList.at(1).toStdString().c_str() << "'";
                }
                else
                {
                    wclause << specList.at(0).toStdString().c_str() << " = " << specList.at(1).toStdString().c_str();
                }

                row = sqltab->GetIntValue(pkcol.toStdString(), wclause.str());
            }
            else
            {   int rec;
                bool bfound = false;
                for (rec = mTabMinPK; rec <= mTabMaxPK && !bfound; ++rec)
                {
                    if (tab->GetStrValue(colidx, rec).compare(specList.at(1).toStdString()) == 0)
                    {
                        bfound = true;
                    }
                }
                if (!bfound)
                {
                    bthrow = true;
                }
                row = rec;
            }

            if (!bthrow)
            {
                // remember, in parameter expressions, we're dealing in 1-based indices!
                if (mTabMinPK == 0)
                {
                    ++row;
                }
                return QVariant::fromValue(row);
            }
        }

        // if table PK is 0-based, we adjust the
        // specified row number
        if (mTabMinPK == 0)
        {
            --row;
        }

        // if row out of bounds throw an error
        if (row < mTabMinPK || row > mTabMaxPK)
        {
            bthrow = true;
        }

        if (bthrow)
        {
            QString msg = QString("ERROR - %1::getModelParameter(%2): invalid row number: %3")
                    .arg(this->objectName())
                    .arg(paramSpec)
                    .arg(specList.at(1));
            return param = QVariant::fromValue(msg);
        }
    }

    const otb::AttributeTable::TableColumnType type = tab->GetColumnType(colidx);
    switch (type)
    {
    case otb::AttributeTable::ATTYPE_STRING:
        param = QVariant::fromValue(QString(tab->GetStrValue(colidx, row).c_str()));
        break;
    case otb::AttributeTable::ATTYPE_INT:
        param = QVariant::fromValue(tab->GetIntValue(colidx, row));
        break;
    case otb::AttributeTable::ATTYPE_DOUBLE:
        param = QVariant::fromValue(tab->GetDblValue(colidx, row));
        break;
    }

    return param;
}

void
NMDataComponent::fetchData(NMModelComponent* comp)
{
    NMDebugCtx(ctx, << "...");
    NMMfwException e(NMMfwException::NMDataComponent_InvalidParameter);
    e.setSource(this->objectName().toStdString());
    std::stringstream msg;

    if (comp == 0)
    {
        msg << "Retrieved input component '"
            << mInputCompName.toStdString() << "' is NULL!";
        e.setDescription(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }


    NMDebugAI(<< "previous modified source time: "
            << mSourceMTime.toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString()
            << std::endl);

    // check, whether we've got to fetch the data again
    // or whether it is still up-to-date
    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
    if (ic != 0 && ic->getProcess() != 0 && mInputOutputIdx != ic->getProcess()->getAuxDataIdx())
    {
        ic->update(this->getModelController()->getRepository());
        NMDebugAI(<< "current modified source time: "
                  << ic->getProcess()->getModifiedTime().toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString()
                  << std::endl);
        this->mSourceMTime = ic->getProcess()->getModifiedTime();
    }

    QSharedPointer<NMItkDataObjectWrapper> to = comp->getOutput(mInputOutputIdx);
    if (to.isNull())
    {
        //NMLogError(<< ctx << ": input object is NULL!");
        NMMfwException de(NMMfwException::NMProcess_UninitialisedDataObject);
        de.setSource(this->objectName().toStdString());
        de.setDescription("Input NMItkDataObjectWrapper is NULL!");
        throw de;
    }

    // we always disconnect the data from the pipeline
    // when we've got pipeline data object
    if (to->getDataObject() != 0)
    {
        to->getDataObject()->DisconnectPipeline();
    }

    this->setInput(to);

    NMDebugCtx(ctx, << "done!");
}

QSharedPointer<NMItkDataObjectWrapper>
NMDataComponent::getOutput(unsigned int idx)
{
    return mDataWrapper;
}

QSharedPointer<NMItkDataObjectWrapper>
NMDataComponent::getOutput(const QString& name)
{
    return mDataWrapper;
}

void
NMDataComponent::update(const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");

    // prevent chasing our own tail
    if (mIsUpdating)
    {
        NMMfwException ul(NMMfwException::NMModelComponent_RecursiveUpdate);
        ul.setSource(this->objectName().toStdString());
        std::stringstream msg;
        msg << this->objectName().toStdString()
            << " is already updating itself!";
        ul.setDescription(msg.str());
        //NMDebugCtx(ctx, << "done!");
        NMLogWarn(<< ul.getSource() << ": " << ul.getDescription());
        return;
    }
    mIsUpdating = true;

    if (!this->mbLinked)
    {
        NMIterableComponent* host = this->getHostComponent();
        unsigned int step = host != 0 ? host->getIterationStep()-1 : 0;

        this->linkComponents(step, repo);
    }

    NMModelComponent* inComp = this->getModelController()->getComponent(mInputCompName);
    if (inComp)
    {
        // provenance information
        NMIterableComponent* host = this->getHostComponent();
        unsigned int hStep = 1;
        QString hostId = "-";
        if (host)
        {
            hStep = host->getIterationStep();
            hostId = QString("nm:%1_Update-%2")
                    .arg(host->objectName())
                    .arg(hStep);
        }
        QStringList attrs = this->getModelController()->getProvNAttributes(this);
        QStringList args;
        QString actId = QString("nm:%1_Update-%2")
                        .arg(this->objectName())
                        .arg(hStep);
        QDateTime startTime = QDateTime::currentDateTime();
        args << actId << "-" << hostId;
        //args << startTime.toString(Qt::ISODate);
        args << startTime.toString(getModelController()->getSetting("TimeFormat").toString());

        this->getModelController()->getLogger()->logProvN(NMLogger::NM_PROV_START, args, attrs);

        // update the data component
        this->fetchData(inComp);

        // provenance again ...
        QDateTime endTime = QDateTime::currentDateTime();
        args.clear();
        args << actId << "-" << hostId;
        //args << endTime.toString(Qt::ISODate);
        args << endTime.toString(getModelController()->getSetting("TimeFormat").toString());

        this->getModelController()->getLogger()->logProvN(NMLogger::NM_PROV_END, args, attrs);
    }

    mIsUpdating = false;

    NMDebugCtx(ctx, << "done!");
}

void
NMDataComponent::reset(void)
{
    NMDebugCtx(this->objectName().toStdString(), << "...");

    if (!mDataWrapper.isNull())
    {
        if (    mDataWrapper->getOTBTab().IsNotNull()
            &&  mDataWrapper->getOTBTab()->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE
           )
        {
            otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(mDataWrapper->getOTBTab().GetPointer());
            sqltab->CloseTable();
        }
        mDataWrapper.clear();
    }
    this->mbLinked = false;
    this->mIsUpdating = false;
    this->mInputCompName.clear();
    this->mLastInputCompName.clear();
    this->mSourceMTime.setMSecsSinceEpoch(0);
    this->mInputOutputIdx = 0;
    this->mLastInputOutputIdx = 0;

    emit NMDataComponentChanged();

    NMDebugCtx(this->objectName().toStdString(), << "done!");
}
