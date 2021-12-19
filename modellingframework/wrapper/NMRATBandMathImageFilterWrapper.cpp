 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd
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
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMModelController.h"
#include "NMRATBandMathImageFilterWrapper.h"
#include "NMImageReader.h"
#include "NMModelComponent.h"
#include "NMIterableComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMMfwException.h"

#include <string>
#include <vector>
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImage.h"
#include "otbRATBandMathImageFilter.h"
#include "NMItkDataObjectWrapper.h"
#include "NMOtbAttributeTableWrapper.h"
#include "otbAttributeTable.h"

#include <QVariant>
#include <QVector>
#include <QString>
#include <QStringList>

/** Helper Classes */
template <class inputType, class outputType, unsigned int Dimension>
class NMRATBandMathImageFilterWrapper_Internal
{
public:
    typedef otb::Image<inputType, Dimension> ImgType;
    typedef otb::RATBandMathImageFilter<ImgType> FilterType;
    //typedef otb::AutoResampleMapAlgebraFilter<ImgType> FilterType;
    typedef typename FilterType::Pointer FilterTypePointer;

    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands)
        {
            FilterTypePointer f = FilterType::New();
            otbFilter = f;
        }

    static void setExpression(itk::ProcessObject::Pointer& otbFilter,
            QString expression)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetExpression(expression.toStdString());
        }

    static void setNbExpr(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numExpr)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetNbExpr(numExpr);
        }

    static void setUseTableColumnCache(itk::ProcessObject::Pointer& otbFilter,
            bool useCache)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetUseTableColumnCache(useCache);
        }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, const QString& name)
        {
            //NMDebugCtx(ctxNMRATBandMathWrapper, << "...");
            ImgType* img = dynamic_cast<ImgType*>(dataObj);
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());

            //NMDebugAI(<< "used input ..." << std::endl);
            //NMDebugAI(<< "  index: " << idx << std::endl);
            //NMDebugAI(<< "  varName: " << varName.toStdString() << std::endl);

            filter->SetNthInput(idx, img); //, varName.toStdString());
            //NMDebugCtx(ctxNMRATBandMathWrapper, << "done!");
        }

    static void setNthInputName(itk::ProcessObject::Pointer& otbFilter,
            unsigned int idx, const std::string& name)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetNthInputName(idx, name);
        }

    static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
            unsigned int idx, otb::AttributeTable::Pointer tab, std::vector<std::string> varNames)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetNthAttributeTable(idx, tab, varNames);
        }

    static void setOutputNames(itk::ProcessObject::Pointer& otbFilter, const QStringList& outputNames)
        {
            std::vector<std::string> onames;
            foreach (const QString& name, outputNames)
            {
                onames.push_back(name.toStdString());
            }
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetOutputNames(onames);
        }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            ImgType* output = nullptr;
            if (idx < filter->GetNumberOfOutputs())
            {
                output = dynamic_cast<ImgType*>(filter->GetOutput(idx));
            }
            return output;
        }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, const QString& name)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            return filter->GetOutputByName(name.toStdString());
        }
};

template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 1>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 2>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<char, char, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<short, short, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<int, int, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<long, long, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<float, float, 3>;
template class NMRATBandMathImageFilterWrapper_Internal<double, double, 3>;


WrapFlexiGetOutput(NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal, callInputDimGetOutput )
WrapFlexiGetOutputByName(NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal, callInputDimGetOutputByName)
//GetOutputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
WrapFlexiInstantiation(NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal, callInputDimCreator )
//InstantiateObjectWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )
WrapFlexiSetNthInput(NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal, callInputDimSetInput )
//SetNthInputWrap( NMRATBandMathImageFilterWrapper, NMRATBandMathImageFilterWrapper_Internal )


#define callSetNthAttributeTable( filterPixelType, wrapName )											\
if (this->mInputNumDimensions == 1)                                                         \
{                                                                                           \
    wrapName<filterPixelType, filterPixelType, 1>::setNthAttributeTable(     \
            this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \
else if (this->mInputNumDimensions == 2)                                                         \
{                                                                                           \
    wrapName<filterPixelType, filterPixelType, 2>::setNthAttributeTable(     \
            this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \
else if (this->mInputNumDimensions == 3)                                                    \
{                                                                                           \
    wrapName<filterPixelType, filterPixelType, 3>::setNthAttributeTable(     \
            this->mOtbProcess, idx, table, colnames);                                          \
}                                                                                           \



#define callSetExpression( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setExpression( \
                this->mOtbProcess, expression); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setExpression( \
                this->mOtbProcess, expression); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setExpression( \
                this->mOtbProcess, expression); \
    }\
}

#define callSetOutputNamesBandMath( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setOutputNames( \
                this->mOtbProcess, outputNames); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setOutputNames( \
                this->mOtbProcess, outputNames); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setOutputNames( \
                this->mOtbProcess, outputNames); \
    }\
}

#define callSetNbExpr( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setNbExpr( \
                this->mOtbProcess, numExpr); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setNbExpr( \
                this->mOtbProcess, numExpr); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setNbExpr( \
                this->mOtbProcess, numExpr); \
    }\
}


#define callSetUseTableCache( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setUseTableColumnCache( \
                this->mOtbProcess, useCache); \
    }\
}


#define callSetNthInputName( filterPixelType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 1 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    } \
    else if (this->mInputNumDimensions == 2) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 2 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        NMRATBandMathImageFilterWrapper_Internal< filterPixelType, filterPixelType, 3 >::setNthInputName( \
                this->mOtbProcess, idx, name); \
    }\
}

/** Constructors/Destructors
 */


NMRATBandMathImageFilterWrapper::NMRATBandMathImageFilterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->ctx = "NMRATBandMathImageFilterWrapper";
    this->mbIsInitialised = false;
    this->setObjectName(tr("NMRATBandMathImageFilterWrapper"));
    this->mInputComponentType = otb::ImageIOBase::FLOAT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 2;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mParamPos = 0;
    this->mUseTableColumnCache = false;
    this->mParameterHandling = NMProcess::NM_USE_UP;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("InputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("InputTables"), QStringLiteral("InputTables"));
    mUserProperties.insert(QStringLiteral("UserOutputNames"), QStringLiteral("OutputNames"));
    mUserProperties.insert(QStringLiteral("MapExpressions"), QStringLiteral("MapExpressions"));
    mUserProperties.insert(QStringLiteral("UseTableColumnCache"), QStringLiteral("UseTableColumnCache"));
}

NMRATBandMathImageFilterWrapper::~NMRATBandMathImageFilterWrapper(void)
{
}

void
NMRATBandMathImageFilterWrapper
::setNthAttributeTable(unsigned int idx,
        otb::AttributeTable::Pointer table,
        std::vector<std::string> tableColumns)
{
    if (!this->mbIsInitialised)
        return;

    std::vector<std::string>& colnames = tableColumns;
    switch(this->mInputComponentType)
    {
    MacroPerType( callSetNthAttributeTable, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void
NMRATBandMathImageFilterWrapper
::setInternalNumExpression(unsigned int numExpr)
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mInputComponentType)
    {
    MacroPerType( callSetNbExpr, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void
NMRATBandMathImageFilterWrapper
::setInternalUseTableCache(bool useCache)
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mInputComponentType)
    {
    MacroPerType( callSetUseTableCache, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void
NMRATBandMathImageFilterWrapper
::setInternalNthInputName(unsigned int idx, const QString& varName)
{
    if (!this->mbIsInitialised)
        return;

    std::string name = varName.toStdString();

    switch(this->mInputComponentType)
    {
    MacroPerType( callSetNthInputName, NMRATBandMathImageFilterWrapper_Internal )
    default:
        break;
    }
}

void NMRATBandMathImageFilterWrapper::setInternalExpression(QString expression)
{
    if (!this->mbIsInitialised)
        return;

    switch (this->mInputComponentType)
    {
    MacroPerType( callSetExpression, NMRATBandMathImageFilterWrapper_Internal)
    default:
        break;
    }
}

void NMRATBandMathImageFilterWrapper::setInternalOutputNames(const QStringList& outputNames)
{
    if (!this->mbIsInitialised)
        return;

    switch (this->mInputComponentType)
    {
    MacroPerType( callSetOutputNamesBandMath, NMRATBandMathImageFilterWrapper_Internal)
    default:
        break;
    }
}

void
NMRATBandMathImageFilterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");

    int givenStep = step;
    // now let's set this process' special properties
    // set the calculation expression
//	step = this->mapHostIndexToPolicyIndex(givenStep, this->mMapExpressions.size());
//	QString currentExpression;
//	if (step < this->mMapExpressions.size())
//	{
//		currentExpression = this->mMapExpressions.at(step);//.toLower();
//		this->setInternalExpression(currentExpression);
//	}

    // this now takes care of pre-processing the current map expression
    // to fetch any constant epxression values (i.e. model parameters)
    // from other model components
    QVariant outNamesVar = this->getParameter("UserOutputNames");
    QStringList curOutNames;
    if (outNamesVar.isValid())
    {
        curOutNames = outNamesVar.toStringList();
        this->setInternalOutputNames(curOutNames);

        QString curOutNamesProvN = QString("nm:UserOutputNames=\"%1\"")
                                   .arg(curOutNames.join(" "));
        this->addRunTimeParaProvN(curOutNamesProvN);
    }
    else
    {
        //NMMfwException e(NMMfwException::NMProcess_MissingParameter);
        //e.setSource(this->parent()->objectName().toStdString());
        //e.setDescription("NMRATBandMathImageWrapper: No valid list of output names specified!");
        //throw e;

        NMLogWarn(<< ctx << ": No list of output names specified! Ensure outputs are added "
                  << " to downstream processing objects in the right order using 0-based "
                  << " output indices with the component name, e.g. ThisComponentName:1 for "
                  << " the second output!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    int numExpr = 1;
    QVariant exprParam = this->getParameter("MapExpressions");
    QString currentExpression;
    if (exprParam.isValid())
    {
        currentExpression = exprParam.toString();
        this->setInternalExpression(currentExpression);

        QString curExrProvN = QString("nm:MapExpression=\"%1\"")
                              .arg(currentExpression);
        this->addRunTimeParaProvN(curExrProvN);

        // work out number of expressions from supplied expresssion string
        numExpr = currentExpression.split(",", QString::SkipEmptyParts).size();
        this->setInternalNumExpression(numExpr);

        QString numExprProvN = QString("nm:NumExpressions=\"%1\"").arg(numExpr);
        this->addRunTimeParaProvN(numExprProvN);
    }
    else
    {
        NMMfwException e(NMMfwException::NMProcess_MissingParameter);
        e.setSource(this->parent()->objectName().toStdString());
        e.setDescription("NMRATBandMathImageWrapper: No map expression specified!");
        throw e;

        NMLogError(<< ctx << ": no map expression available!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

//    step = this->mapHostIndexToPolicyIndex(givenStep, this->mNumExpressions.size());
//    if (step < this->mNumExpressions.size())
//    {
//        bool bOK;
//        int numExpr = this->mNumExpressions.at(step).toInt(&bOK);
//        if (bOK)
//        {
//            this->setInternalNumExpression(numExpr);
//            QString numExprProvN = QString("nm:NumExpressions=\"%1\"").arg(numExpr);
//            this->addRunTimeParaProvN(numExprProvN);
//        }
//    }
//    else
//    {
//        NMDebugAI(<< "no number of expressions given, so we assume,"
//                << " we've got one expression!" << std::endl);
//    }

    this->setInternalUseTableCache(mUseTableColumnCache);
    QString useCache = this->mUseTableColumnCache ? QStringLiteral("yes") : QStringLiteral("no");
    QString useCacheProvN = QString("nm:UseTableColumnCache=\"%1\"").arg(useCache);
    this->addRunTimeParaProvN(useCacheProvN);


    NMModelController* ctrl = this->getModelController();
    if (ctrl == 0)
    {
        NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
        e.setSource(this->parent()->objectName().toStdString());
        std::stringstream msg;
        msg << "no ModelController available!" << std::endl;

        NMLogError(<< ctx << msg .str() << std::endl);

        NMDebugCtx(ctx, << "done!");
        e.setDescription(msg.str());
        throw e;
    }

    // we go through every input image, check, whether a table is
    // available for this step, and then we identify the
    // columns uses in the currentExpression and pass their
    // names to the internal process object
    step = this->mapHostIndexToPolicyIndex(givenStep,
            this->mInputComponents.size());
    QStringList currentInputs;
    if (step < this->mInputComponents.size())
    {
        currentInputs = this->mInputComponents.at(step);
        int cnt = 0;
        foreach (const QString& input, currentInputs)
        {
            QString inputCompName = ctrl->getComponentNameFromInputSpec(input);
            NMModelComponent* comp = ctrl->getComponent(inputCompName);
            if (comp == 0)
            {
                NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
                e.setSource(this->parent()->objectName().toStdString());
                std::stringstream msg;
                msg << "couldn't find " << inputCompName.toStdString() << "'!" << std::endl;

                NMLogError(<< ctx << msg .str() << std::endl);


                NMDebugCtx(ctx, << "done!");
                e.setDescription(msg.str());
                throw e;
            }

            // is this component taking part in this 'round'?
            NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(comp);
            if (    sic != nullptr
                 && sic->evalNumIterationsExpression(step+1) == 0
               )
            {
                NMLogDebug(<< ctx << inputCompName.toStdString() << "::NumIterationExpression = 0 : we're skipping it!" );
                continue;
            }


            NMDebugAI(<< "img-name #" << cnt << ": " << inputCompName.toStdString() << std::endl;)

            // make sure the input is linked into the pipeline properly, otherwise, we might get
            // an exception here, especially if a sink process component is being executed individually
            // for testing purposes
            comp->linkComponents(step, repo);

            QSharedPointer<NMItkDataObjectWrapper> dw = ctrl->getOutputFromSource(input);
            if (dw.isNull())
            {
                ++cnt;
                continue;
            }

            otb::AttributeTable::Pointer tab = dw->getOTBTab();
            std::vector<std::string> vcolnames;
            if (tab.IsNotNull())
            {
                QStringList colNamesProvN;
                for (int c=0; c < tab->GetNumCols(); ++c)
                {
                    std::string colname = tab->GetColumnName(c);
                    QString cn(colname.c_str());
                    //cn = cn.toLower();
                    if (currentExpression.contains(cn, Qt::CaseInsensitive))
                    {
                        vcolnames.push_back(cn.toStdString());
                        colNamesProvN << cn;
                    }
                }
                if (vcolnames.size() > 0)
                {
                    this->setNthAttributeTable(cnt, tab, vcolnames);

                    QString colNamesProvNAttr = QString("nm:TableColumns-%1=\"%2\"")
                                                .arg(cnt)
                                                .arg(colNamesProvN.join(" "));
                    this->addRunTimeParaProvN(colNamesProvNAttr);
                }
            }


            // replace any LookupParameters with their actual formula value
            // internal func wich looks after that!


            // we check, whether the input has a UserID defined, and if so, we set it as the
            // nth input variable name
            if (!comp->getUserID().isEmpty())
            {
                this->setInternalNthInputName(cnt, comp->getUserID());
                QString inputNameProvNAttr = QString("nm:InputUserID-%1=\"%2\"")
                                             .arg(cnt)
                                             .arg(comp->getUserID());
                this->addRunTimeParaProvN(inputNameProvNAttr);
            }
            else
            {
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(this->parent()->objectName().toStdString());
                std::stringstream msg;
                msg << "'" << inputCompName.toStdString() << "'"
                    << " Missing UserID!";
                e.setDescription(msg.str());
                NMLogError(<< ctx << msg .str());
                NMDebugCtx(ctx, << "done!");
                throw e;
            }
            ++cnt;
        }
    }


    NMDebugCtx(ctx, << "done!");
}


