/*****************************t*************************************************
 * Created by Alexander Herzige
 * Copyright 2017 Landcare Research New Zealand Ltd
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
 * NMDataRefComponent.cpp
 *
 *  Created on: 30/03/2017
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
#include "NMDataRefComponent.h"
#include "NMIterableComponent.h"
#include "NMMfwException.h"
#include "otbSQLiteTable.h"
#include "itkNMLogEvent.h"

const std::string NMDataRefComponent::ctx = "NMDataRefComponent";

NMDataRefComponent::NMDataRefComponent(QObject* parent)
    : NMDataComponent(parent)
{
	this->setParent(parent);
	this->initAttributes();
}

NMDataRefComponent::~NMDataRefComponent()
{
	reset();
}

void
NMDataRefComponent::initAttributes(void)
{
	NMModelComponent::initAttributes();
	mInputCompName.clear();
	mLastInputCompName.clear();
	mSourceMTime.setMSecsSinceEpoch(0);
	mInputOutputIdx = 0;
	mLastInputOutputIdx = 0;
	mParamPos = 0;
	mbLinked = false;
    mTabMinPK = 0;
    mDataComponent = 0;
}

void
NMDataRefComponent::setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg)
{
    if (mDataComponent)
    {
        mDataComponent->setNthInput(idx, inputImg);
    }
    // get the data component this reference component is actually
    // referencing
    else
    {
        if (mDataComponent == 0)
        {
            NMLogError(<< ctx << " - source data component is NULL!");
        }
    }
}

void
NMDataRefComponent::updateDataSource(void)
{
    reset();
    mDataComponent = 0;

    NMModelComponent* comp = NMModelController::getInstance()->getComponent(mDataComponentName);
    if (comp)
    {
        NMDataComponent* dc = qobject_cast<NMDataComponent*>(comp);
        if (dc)
        {
            mDataComponent = dc;
            NMLogWarn(<< ctx << ": Source component identified by component name rather than "
                             << "UserID, which is subject to change, if this model component "
                             << "is imported into a different model!");
        }
        else
        {
            mDataComponent = 0;
            NMLogError(<< ctx << ": Specified source component '"
                              << mDataComponentName.toStdString() << "' "
                              << "couldn't be found is not a DataBuffer (NMDataComponent)!");
        }
    }
    else
    {
        QList<NMModelComponent*> comps = NMModelController::getInstance()->getComponents(mDataComponentName);

        QList<int> dcIdx;
        for (int i=0; i < comps.count(); ++i)
        {
            NMModelComponent* mc = comps.at(i);
            if (QString(mc->metaObject()->className()).compare(QString::fromLatin1("NMDataComponent")) == 0)
            {
                dcIdx << i;
            }
        }

        if (dcIdx.count() == 0)
        {
            NMLogError(<< ctx << ": Specified source component '"
                              << mDataComponentName.toStdString() << "' "
                              << "couldn't be found is not a DataBuffer (NMDataComponent)!");
            return;
        }
        else if (dcIdx.count() > 1)
        {
            NMLogError(<< ctx << ": The UserID of the source NMDataComponent is not unique!");
            return;
        }

        mDataComponent = qobject_cast<NMDataComponent*>(comps.at(dcIdx.at(0)));
    }
}

void
NMDataRefComponent::linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    if (mDataComponent == 0)
    {
        updateDataSource();
        if (mDataComponent == 0)
        {
            NMLogError(<< ctx << " - source data component is NULL!");
            return;
        }
    }


	NMDebugCtx(ctx, << "...");
    NMMfwException e(NMMfwException::NMDataComponent_InvalidParameter);
    e.setSource(this->objectName().toStdString());
	std::stringstream msg;

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
        if (!mDataComponent->mDataWrapper.isNull())
        {
            mbLinked = true;
            NMDebugCtx(ctx, << "done!");
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
		QStringList inputSrcParams = inputSpec.split(":", QString::SkipEmptyParts);
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
    NMModelComponent* inComp = NMModelController::getInstance()->getComponent(mInputCompName);

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
NMDataRefComponent::getModelParameter(const QString &paramSpec)
{
    QVariant param;

    if (mDataComponent == 0)
    {
        NMLogError(<< ctx << " - source data component is NULL!");
        return param;
    }

    bool bThrowUp = false;
    QString demsg;

    if (paramSpec.isEmpty())
    {
        bThrowUp = true;
        demsg = "invalid parameter specification!";
    }

    if (    mDataComponent->mDataWrapper.isNull()
        ||  mDataComponent->mDataWrapper->getOTBTab().IsNull()
       )
    {
        this->update(NMModelController::getInstance()->getRepository());

        if (    mDataComponent->mDataWrapper.isNull()
            ||  mDataComponent->mDataWrapper->getOTBTab().IsNull()
           )
        {
            bThrowUp = true;
            demsg = "no parameter table found!";
        }
    }

    if (bThrowUp)
    {
        QString msg = QString("ERROR - %1::getModelParameter(%2) - %3!")
                .arg(this->objectName())
                .arg(paramSpec)
                .arg(demsg);
        return param = QVariant::fromValue(msg);
    }

    //  <columnName>:<rowNumber>
    otb::AttributeTable::Pointer tab = mDataComponent->mDataWrapper->getOTBTab();

    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
    long long row = 0;
    long long recnum = tab->GetNumRows();
    if (specList.size() == 2)
    {
        bool bthrow = false;
        bool bok;
        // receiving 1-based row number
        row = specList.at(1).toLongLong(&bok);
        if (!bok)
        {
            bthrow = true;
        }
        else
        {
            // if table PK is 0-based, we adjust the
            // specified row number
            if (mTabMinPK == 0)
            {
                --row;

                // if row out of bounds throw an error
                if (row < 0 || row > recnum-1)
                {
                    bthrow = true;
                }
            }
        }

        if (bthrow)
        {
            QString msg = QString("ERROR - %1::getModelParameter(%2) - row number '%3' is invalid!")
                    .arg(this->objectName())
                    .arg(paramSpec)
                    .arg(specList.at(1));
            return param = QVariant::fromValue(msg);
        }
    }

    int colidx = tab->ColumnExists(specList.at(0).toStdString().c_str());
    if (colidx >= 0)
    {
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
    }
    else
    {
        QString msg = QString("ERROR - %1::getModelParameter(%2) - column '%3' is invalid!")
                .arg(this->objectName())
                .arg(paramSpec)
                .arg(specList.at(0));
        return param = QVariant::fromValue(msg);
    }

    return param;
}

void
NMDataRefComponent::fetchData(NMModelComponent* comp)
{
    if (mDataComponent == 0)
    {
        NMLogError(<< ctx << " - source data component is NULL!");
        return;
    }


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
		ic->update(NMModelController::getInstance()->getRepository());
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
NMDataRefComponent::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> ret;

    if (mDataComponent)
    {
        return mDataComponent->mDataWrapper;
    }

    return ret;
}

void
NMDataRefComponent::update(const QMap<QString, NMModelComponent*>& repo)
{
    if (mDataComponent == 0)
    {
        updateDataSource();
        if (mDataComponent == 0)
        {
            NMLogError(<< ctx << " - source data component is NULL!");
            return;
        }
    }

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

    NMModelComponent* inComp = NMModelController::getInstance()->getComponent(mInputCompName);
    if (inComp)
	{
        this->fetchData(inComp);
	}

    mIsUpdating = false;

	NMDebugCtx(ctx, << "done!");
}

void
NMDataRefComponent::reset(void)
{
    if (mDataComponent && !mDataComponent->mDataWrapper.isNull())
	{
        if (    mDataComponent->mDataWrapper->getOTBTab().IsNotNull()
            &&  mDataComponent->mDataWrapper->getOTBTab()->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE
           )
        {
            otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(mDataComponent->mDataWrapper->getOTBTab().GetPointer());
            sqltab->CloseTable();
        }
        mDataComponent->mDataWrapper.clear();
	}
    this->mDataComponent = 0;
	this->mbLinked = false;
    this->mIsUpdating = false;
	this->mInputCompName.clear();
	this->mLastInputCompName.clear();
	this->mSourceMTime.setMSecsSinceEpoch(0);
	this->mInputOutputIdx = 0;
	this->mLastInputOutputIdx = 0;

    emit NMDataComponentChanged();
}
