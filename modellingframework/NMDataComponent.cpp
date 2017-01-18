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
    mTabMinPK = 0;
}

void
NMDataComponent::setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg)
{
    // note QSharedPointer takes care of management, so no
    // parent required, actually if we had a parent
    // we'd get crash with double free error

    if (!mDataWrapper.isNull())
        mDataWrapper.clear();
	mDataWrapper = inputImg;

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

//    QMap<QString, NMModelComponent*>::const_iterator it =
//			repo.find(mInputCompName);
//	if (it == repo.end())
    if (inComp == 0)
    {
		msg << "The specified input component '"
		    << mInputCompName.toStdString() << "' couldn't be found!";
        e.setDescription(msg.str());
		NMDebugCtx(ctx, << "done!");
		throw e;
	}

	// link the inputs component
	//it.value()->linkComponents(step, repo);

	//// request largest possible region
	//NMModelComponent* comp = it.value();
    //QSharedPointer<NMItkDataObjectWrapper> dw = comp->getOutput(mInputOutputIdx);
	//if (dw != 0)
	//{
	//	itk::DataObject* dobj = dw->getDataObject();
	//	//if (dobj != 0)
	//	//	dobj->SetRequestedRegionToLargestPossibleRegion();
	//	if (dobj == 0)//else
	//	{
	//		comp->linkComponents(step, repo);
	//		dobj->SetRequestedRegionToLargestPossibleRegion();
	//	}
	//}

	this->mbLinked = true;

	//this->fetchData(comp);

	NMDebugCtx(ctx, << "done!");
}

QVariant
NMDataComponent::getModelParameter(const QString &paramSpec)
{
    QVariant param;

    bool bThrowUp = false;
    QString demsg;

    if (paramSpec.isEmpty())
    {
        bThrowUp = true;
        demsg = "invalid parameter specification!";
    }

    if (    this->mDataWrapper.isNull()
        ||  this->mDataWrapper->getOTBTab().IsNull()
       )
    {
        this->update(NMModelController::getInstance()->getRepository());

        if (    this->mDataWrapper.isNull()
            ||  this->mDataWrapper->getOTBTab().IsNull()
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
    otb::AttributeTable::Pointer tab = this->mDataWrapper->getOTBTab();

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
	if (ic != 0 && ic->getProcess() != 0)
	{
		ic->update(NMModelController::getInstance()->getRepository());
		NMDebugAI(<< "current modified source time: "
				  << ic->getProcess()->getModifiedTime().toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString()
				  << std::endl);
		this->mSourceMTime = ic->getProcess()->getModifiedTime();
	}

    //NMIterableComponent* itComp = qobject_cast<NMIterableComponent*>(comp);
	//if (itComp != 0 && itComp->getProcess() != 0)
	//{
	//	if (itComp->getProcess()->getInternalProc() != 0)
	//		itComp->getProcess()->getInternalProc()->ReleaseDataBeforeUpdateFlagOn();
	//}

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
	//else
	//{
	//	msg << "Internal data object of '"
	//		<< mInputCompName.toStdString() << "' is NULL!";
    //	e.setDescription(msg.str());
	//	NMDebugCtx(ctx, << "done!");
	//	throw e;
	//}

	///// DEBUG - do we actually have some data here?
	//NMDebug(<< endl);
	//to->getDataObject()->Print(std::cout, itk::Indent(2));
	//NMDebug(<< endl);

    this->setInput(to);

	NMDebugCtx(ctx, << "done!");
}

QSharedPointer<NMItkDataObjectWrapper>
NMDataComponent::getOutput(unsigned int idx)
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
        NMDebugCtx(ctx, << "done!");
        return;
    }
    mIsUpdating = true;

//    // don't do anything, if we're already 'loaded'
//    if (    mDataWrapper != 0
//        &&  (   mDataWrapper->getDataObject() != 0
//            ||  mDataWrapper->getOTBTab().IsNotNull()
//            )
//       )
//    {
//        NMDebugAI(<< "It appears as if we've got our data already!" << std::endl);
//        NMDebugCtx(ctx, << "done!");
//        return;
//    }

    if (!this->mbLinked)
    {
        NMIterableComponent* host = this->getHostComponent();
        unsigned int step = host != 0 ? host->getIterationStep()-1 : 0;

        this->linkComponents(step, repo);
    }

    NMModelComponent* inComp = NMModelController::getInstance()->getComponent(mInputCompName);
    if (inComp)
	{
        //this->fetchData(it.value());
        this->fetchData(inComp);
	}

    mIsUpdating = false;

	NMDebugCtx(ctx, << "done!");
}

void
NMDataComponent::reset(void)
{
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
	this->mInputCompName.clear();
	this->mLastInputCompName.clear();
	this->mSourceMTime.setMSecsSinceEpoch(0);
	this->mInputOutputIdx = 0;
	this->mLastInputOutputIdx = 0;

	emit NMDataComponentChanged();
}
