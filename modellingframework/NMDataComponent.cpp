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
#include "nmlog.h"
#include <string>
#include <sstream>

#include "NMModelController.h"
#include "NMDataComponent.h"
#include "NMIterableComponent.h"
#include "NMMfwException.h"

const std::string NMDataComponent::ctx = "NMDataComponent";

NMDataComponent::NMDataComponent(QObject* parent)
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

}

void
NMDataComponent::setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg)
{
    if (!mDataWrapper.isNull())
        mDataWrapper.clear();

	mDataWrapper = inputImg;
    // note QSharedPointer takes care of management, so no
    // parent required, actually if we had a parent
    // we'd get crash with double free error
    //mDataWrapper->setParent(this);
	emit NMDataComponentChanged();
}

void
NMDataComponent::linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctx, << "...");
	NMMfwException e(NMMfwException::NMDataComponent_InvalidParameter);
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
            e.setMsg("List of input specs is empty!");
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
		e.setMsg("No input component defined!");
		NMDebugCtx(ctx, << "done!");
		throw e;
	}

	// since the data component only accepts one input
	// we just grab the first
	QString inputSpec;
	inputSpec = listOfSpecs.at(0);

	if (inputSpec.isEmpty())
	{
		e.setMsg("No input component defined!");
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
				e.setMsg(msg.str());
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
	QMap<QString, NMModelComponent*>::const_iterator it =
			repo.find(mInputCompName);
	if (it == repo.end())
	{
		msg << "The specified input component '"
		    << mInputCompName.toStdString() << "' couldn't be found!";
		e.setMsg(msg.str());
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

    if (    paramSpec.isEmpty()
        ||  this->mDataWrapper.isNull()
        ||  this->mDataWrapper->getOTBTab().IsNull()
       )
    {
        return param;
    }

    //  <columnName>:<rowNumber>
    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
    long long row = 0;
    if (specList.size() == 2)
    {
        bool bok;
        row = specList.at(1).toLongLong(&bok);
        if (!bok)
        {
            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
            QString msg = QString("Specified row number '%1' is invalid!")
                    .arg(this->objectName()).arg(specList.at(1));
            me.setMsg(msg.toStdString());
            throw me;
            return param;
        }
    }

    otb::AttributeTable::Pointer tab = this->mDataWrapper->getOTBTab();

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

    return param;

    //    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
    //    if (specList.size() < 2)
    //    {
    //        specList << "0";
    //    }

    //    QVariant param;
    //    if (mDataWrapper.isNull() || mDataWrapper->getOTBTab().IsNull())
    //    {
    //        NMMfwException me(NMMfwException::NMModelComponent_UninitialisedDataObject);
    //        QString msg = QString("'%1' has not been initialised!").arg(this->objectName());
    //        me.setMsg(msg.toStdString());
    //        throw me;
    //        return param;
    //    }

    //    otb::AttributeTable::Pointer tab = mDataWrapper->getOTBTab();
    //    int idx = tab->ColumnExists(specList.at(0).toStdString());
    //    if (idx < 0)
    //    {
    //        NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
    //        QString msg = QString("'%1' has no column '%2'")
    //                .arg(this->objectName())
    //                .arg(specList.at(0));
    //        me.setMsg(msg.toStdString());
    //        throw me;
    //        return param;
    //    }

    //    bool bok = false;
    //    long long row = specList.at(1).toLongLong(&bok);
    //    if (!bok)
    //    {
    //        NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
    //        me.setMsg("Invalid parameter index!");
    //        throw me;
    //        return param;
    //    }

    //    row = row > tab->GetNumRows() ? tab->GetNumRows() : row;
    //    switch(tab->GetColumnType(idx))
    //    {
    //    case otb::AttributeTable::ATTYPE_DOUBLE:
    //        param = QVariant::fromValue(tab->GetDblValue(idx, row));
    //        break;
    //    case otb::AttributeTable::ATTYPE_INT:
    //        param = QVariant::fromValue(tab->GetIntValue(idx, row));
    //        break;
    //    case otb::AttributeTable::ATTYPE_STRING:
    //        param = QVariant::fromValue(QString(tab->GetStrValue(idx, row).c_str()));
    //        break;
    //    }

    //    return param;
}

void
NMDataComponent::fetchData(NMModelComponent* comp)
{
	NMDebugCtx(ctx, << "...");
	NMMfwException e(NMMfwException::NMDataComponent_InvalidParameter);
	std::stringstream msg;

	if (comp == 0)
	{
		msg << "Retrieved input component '"
			<< mInputCompName.toStdString() << "' is NULL!";
		e.setMsg(msg.str());
		NMDebugCtx(ctx, << "done!");
		throw e;
	}

    // the below doesn't really make sense, e.g. when we want to update a data buffer which
    // holds a timestamp of a time series, and the buffer itself sits on a different time
    // level (i.e. an outer loop), which is very plausible ...
//	bool sharesHost = false;
//	NMIterableComponent* host = this->getHostComponent();
//	if (host->isSubComponent(comp))
//	{
//		sharesHost = true;
//	}

//	// if we don't share the host, and we're in the 2nd+ iteration
//	// we assume, we've got the data already
//	if (!sharesHost
//			&& comp->getTimeLevel() != this->getTimeLevel()
//			&& mParamPos > 0)
//	{
//		NMDebugAI(<< "We believe the data is still up-to-date!" << std::endl);
//		NMDebugCtx(ctx, << "done!");
//		return;
//	}


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
		NMErr(ctx, << "Got NULL output so far, but could be all right.");
		NMMfwException de(NMMfwException::NMProcess_UninitialisedDataObject);
		de.setMsg("Input NMItkDataObjectWrapper is NULL!");
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
	//	e.setMsg(msg.str());
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

	QMap<QString, NMModelComponent*>::const_iterator it =
			repo.find(mInputCompName);

	if (it != repo.end())
	{
		this->fetchData(it.value());
	}

	NMDebugCtx(ctx, << "done!");
}

void
NMDataComponent::reset(void)
{
    if (!mDataWrapper.isNull())
	{
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
