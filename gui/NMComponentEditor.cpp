/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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

#include "NMComponentEditor.h"

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QList>
#include <QStringList>
#include <QScopedPointer>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <limits>

#include "NMProcess.h"
#include "NMModelComponent.h"
#include "NMDataComponent.h"
#include "NMIterableComponent.h"
#include "NMConditionalIterComponent.h"
#include "NMSequentialIterComponent.h"


const std::string NMComponentEditor::ctx = "NMComponentEditor";

NMComponentEditor::NMComponentEditor(QWidget *parent,
            NMCompEditorMode mode)
    : QWidget(parent), mEditorMode(mode), mObj(0), comp(0), proc(0),
      mUpdating(false)
{

#ifdef BUILD_RASSUPPORT
    this->mRasConn = 0;
#endif

    // set up the property browser
    switch(mEditorMode)
    {
    case NM_COMPEDITOR_GRPBOX:
        mPropBrowser = new QtGroupBoxPropertyBrowser(this);
        mPropBrowser->setObjectName("ComponentGroupBoxEditor");
        break;
    case NM_COMPEDITOR_TREE:
        {
            QtTreePropertyBrowser* tpb = new QtTreePropertyBrowser(this);
            tpb->setResizeMode(QtTreePropertyBrowser::Interactive);
            mPropBrowser = tpb;
            mPropBrowser->setObjectName("ComponentTreeEditor");
        }
        break;
    }

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(mPropBrowser->sizePolicy().hasHeightForWidth());
    mPropBrowser->setSizePolicy(sizePolicy);

    QVBoxLayout* lo = new QVBoxLayout();
    lo->addWidget(mPropBrowser);
    this->setLayout(lo);

    debugCounter = 1;
}

//void
//NMComponentEditor::closeEvent(QCloseEvent* event)
//{
//    emit finishedEditing(mObj);
//    QWidget::closeEvent(event);
//}

void
NMComponentEditor::update()
{
    if (mUpdating)
    {
        return;
    }

    if (mObj)
        this->setObject(mObj);
        //this->readComponentProperties(mObj, comp, proc);
}

void
NMComponentEditor::setObject(QObject* obj)
{
    //NMDebugCtx(ctx, << "...");

    // don't do anyting as long we're not ready for change!
    if (mUpdating)
    {
        return;
    }

    // reset the editor upon receiving a NULL object
    if (obj == 0)
    {
        if (this->comp != 0)
        {
            this->disconnect(comp);
        }
        if (this->proc != 0)
        {
            this->disconnect(proc);
        }

        this->mObj = 0;
        this->comp = 0;
        this->proc = 0;
        this->clear();
        debugCounter = 1;
        return;
    }

    NMModelComponent* c = qobject_cast<NMModelComponent*>(obj);
    NMIterableComponent* i = qobject_cast<NMIterableComponent*>(obj);
    NMProcess* p = i != 0 ? i->getProcess() : 0;

    if (mObj == 0)
    {
        if (c != 0)
        {
            mObj = obj;
            comp = c;
            connect(comp, SIGNAL(nmChanged()), this, SLOT(update()));
        }
        else
            return;

        if (p != 0)
        {
            proc = p;
            connect(proc, SIGNAL(nmChanged()), this, SLOT(update()));
        }
        debugCounter = 1;
    }
    else if (mObj != 0 && obj->objectName().compare(mObj->objectName()) != 0)
    {
        if (this->comp != 0)
        {
            this->disconnect(comp);
            comp = 0;
        }
        if (this->proc != 0)
        {
            this->disconnect(proc);
            proc = 0;
        }

        if (c != 0)
        {
            mObj = obj;
            comp = c;
            connect(comp, SIGNAL(nmChanged()), this, SLOT(update()));
        }
        if (p != 0)
        {
            proc = p;
            connect(proc, SIGNAL(nmChanged()), this, SLOT(update()));
        }
        debugCounter = 1;
    }

    //this->setWindowTitle(c->objectName());
    //this->mPropBrowser->setWindowTitle(comp->objectName());
    this->readComponentProperties(mObj, comp, proc);

//      NMDebugCtx(ctx, << "done!");
}

void NMComponentEditor::readComponentProperties(QObject* obj, NMModelComponent* comp,
        NMProcess* proc)
{
    mPropBrowser->clear();

    NMDebugAI(<< ">>>> #" << debugCounter << " - START: " << mObj->objectName().toStdString() << " >>>>>>>>>>>>>>>>>>" << std::endl);

    // let's start with the component's properties,
    // in case we've got a component
    if (comp != 0)// && compq->getProcess() == 0)
    {
        //connect(comp, SIGNAL(nmChanged()), this, SLOT(update()));

        // ------------------------------------------------------
        // PROCESSING A NON-ITERABLE and NON-PROCESS COMPONENT
        // ------------------------------------------------------

        const QMetaObject* meta = obj->metaObject();
        unsigned int nprop = meta->propertyCount();
        for (unsigned int p=0; p < nprop; ++p)
        {
            QMetaProperty property = meta->property(p);
            this->createPropertyEdit(property, obj);
        }

        // ------------------------------------------------------
        // PROCESSING AN AGGREGATE COMPONENT
        // ------------------------------------------------------

        // do we have a process component?
        NMIterableComponent* procComp =
                qobject_cast<NMIterableComponent*>(comp);
        if (procComp != 0)
            proc = procComp->getProcess();
        else
            proc = 0;

        if (procComp != 0 && proc == 0)
        {
            // now we add the subcomponents list for reference
            QStringList strCompChain;
            NMModelComponent* sc = procComp->getInternalStartComponent();
            strCompChain << "{";
            while (sc != 0)
            {
                strCompChain << QString(tr("{%1}")).arg(sc->objectName());
                sc = procComp->getNextInternalComponent();
            }
            strCompChain << "}";

            QtVariantEditorFactory* ed = new QtVariantEditorFactory();
            QtVariantPropertyManager* man = new QtVariantPropertyManager();
            QtVariantProperty* vprop;

            //connect(man, SIGNAL(valueChanged(QtProperty*, const QVariant &)),
            //        this, SLOT(applySettings(QtProperty*, const QVariant &)));

            QVariant vval = QVariant::fromValue(strCompChain);
            vprop = man->addProperty(QVariant::StringList, "Subcomponents");
            vprop->setValue(vval);
            vprop->setEnabled(false);
            mPropBrowser->setFactoryForManager(man, ed);
            mPropBrowser->addProperty(vprop);
        }
    }

    // ------------------------------------------------------
    // PROCESSING A PROCESS COMPONENT
    // ------------------------------------------------------

    // if this is a process component, we add the processes
    // properties to the dialog
    if (proc != 0)
    {
        //connect(proc, SIGNAL(nmChanged()), this, SLOT(update()));

        const QMetaObject* procmeta = proc->metaObject();
        unsigned int nprop = procmeta->propertyCount();
        for (unsigned int p=0; p < nprop; ++p)
        {
            QMetaProperty property = procmeta->property(p);
            this->createPropertyEdit(property, proc);
        }
    }

    NMDebugAI(<< "<<<< #" << debugCounter << " - END: " << mObj->objectName().toStdString() << " <<<<<<<<<<<<<<<<<<" << std::endl);
    debugCounter++;
}

void NMComponentEditor::createPropertyEdit(const QMetaProperty& property,
        QObject* obj)
{
    NMModelComponent* comp = qobject_cast<NMModelComponent*>(obj);
    NMProcess* proc = qobject_cast<NMProcess*>(obj);

    QString propName = QString(property.name());
    if (QString("objectName").compare(property.name()) == 0)
    {
        if (proc != 0)
            propName = "ProcessName";
        else if (comp != 0)
            propName = "ComponentName";
    }
    int propType = property.userType();

    NMDebugAI(<< propName.toStdString() << " (" << property.typeName()
            << "): " << obj->property(property.name()).toString().toStdString()
            << " ...");

    QString propToolTip = "";
    QVariant value;
    bool bok;
    QStringList ctypes;
    ctypes << "uchar" << "char" << "ushort" << "short"
           << "uint" << "int" << "ulong" << "long"
           << "float" << "double" << "unknown";

    QStringList parammodes;
    parammodes << "NM_USE_UP" << "NM_CYCLE" << "NM_SYNC_WITH_HOST";

    QtVariantPropertyManager* manager = new QtVariantPropertyManager();
    QtVariantProperty* prop;

    // assign replacement types for the not supported variant
    // types
    if (QString("char").compare(property.typeName()) == 0 ||
        QString("int").compare(property.typeName()) == 0 ||
        QString("short").compare(property.typeName()) == 0 ||
        QString("long").compare(property.typeName()) == 0
       )
    {
        propType = QVariant::Int;
        value = obj->property(property.name()).toInt(&bok);
        prop = manager->addProperty(propType, propName);
    }
    else if (QString("uchar").compare(property.typeName()) == 0 ||
            QString("uint").compare(property.typeName()) == 0 ||
            QString("ushort").compare(property.typeName()) == 0 ||
            QString("ulong").compare(property.typeName()) == 0
           )
    {
        value = obj->property(property.name()).toInt(&bok);
        propType = value.type();
        prop = manager->addProperty(propType, propName);
        prop->setAttribute("minimum", 1);
        prop->setAttribute("maximum", QVariant::fromValue(INT_MAX));
    }
    else if (QString("float").compare(property.typeName()) == 0)
    {
        value = obj->property(property.name()).toDouble(&bok);
        propType = value.type();
        prop = manager->addProperty(propType, propName);
        prop->setAttribute("minimum", QVariant::fromValue(
                (std::numeric_limits<float>::max()-1) * -1));
        prop->setAttribute("maximum", QVariant::fromValue(
                std::numeric_limits<float>::max()));
    }
    else if (QString("NMItkDataObjectWrapper::NMComponentType")
            .compare(property.typeName()) == 0)
    {
        propType = QtVariantPropertyManager::enumTypeId();
        prop = manager->addProperty(propType, propName);
        prop->setAttribute("enumNames", ctypes);
        QString curPropValStr = NMItkDataObjectWrapper::getComponentTypeString(
                obj->property(property.name())
                .value<NMItkDataObjectWrapper::NMComponentType>());
        NMDebug(<< "current value = " << curPropValStr.toStdString());
        for (unsigned int p=0; p < ctypes.size(); ++p)
        {
            if (ctypes.at(p).compare(curPropValStr) == 0)
                value = QVariant(p);
        }
        propToolTip = tr("PixelType");
    }
    else if (QString("NMProcess::AdvanceParameter").compare(property.typeName()) == 0)
    {
        propType = QtVariantPropertyManager::enumTypeId();
        prop = manager->addProperty(propType, propName);
        prop->setAttribute("enumNames", parammodes);
        NMProcess::AdvanceParameter ap =
                obj->property(property.name()).value<NMProcess::AdvanceParameter>();
        value = QVariant(ap);
        propToolTip = tr("NMProcess::AdvanceParameter");
        NMDebug(<< "current value = " << ap);
    }
#ifdef BUILD_RASSUPPORT
    else if (QString("NMRasdamanConnectorWrapper*")
            .compare(property.typeName()) == 0)
    {
        NMRasdamanConnectorWrapper* wrap = obj->property(property.name())
                .value<NMRasdamanConnectorWrapper*>();

        propType = QVariant::Bool;
        value = QVariant(false);
        prop = manager->addProperty(propType, propName);

        if (this->mRasConn == 0)
        {
            prop->setEnabled(false);
        }
        else if (this->mRasConn->getConnector() == 0)
        {
            prop->setEnabled(false);
        }

        if (wrap != 0 && wrap->getConnector() != 0)
        {
            value = QVariant(true);
        }
    }
#endif
    else if (QString("QStringList")
            .compare(property.typeName()) == 0)
    {
        QStringList nakedList = obj->property(property.name()).toStringList();
        QStringList bracedList;
        bracedList << QString("{");
        foreach(QString s, nakedList)
        {
            bracedList << QString(tr("{%1}").arg(s));
        }
        bracedList << QString(tr("}"));

        value = QVariant::fromValue(bracedList);
        prop = manager->addProperty(QVariant::StringList, propName);
    }
    else if (QString("QList<QStringList>").compare(property.typeName()) == 0)
    {
        QList<QStringList> lst = obj->property(property.name()).value<QList<QStringList> >();

        QStringList bracedList;
        bracedList << QString("{");
        foreach(QStringList l, lst)
        {
            bracedList << QString("{");
            foreach(QString s, l)
            {
                bracedList << QString(tr("{%1}").arg(s));
            }
            bracedList << QString(tr("}"));
        }
        bracedList << QString("}");

        value = QVariant::fromValue(bracedList);
        prop = manager->addProperty(QVariant::StringList, propName);
        propToolTip = tr("QList<QStringList>");
    }
    else if (QString("QList<QList<QStringList> >").compare(property.typeName()) == 0)
    {
        QList<QList<QStringList> > llsl = obj->property(property.name()).value<QList<QList<QStringList> > >();
        QStringList bracedList;

        bracedList << QString("{");
        foreach(QList<QStringList> lsl, llsl)
        {
            bracedList << QString("{");
            foreach(QStringList sl, lsl)
            {
                bracedList << QString("{");
                foreach(QString s, sl)
                {
                    bracedList << QString(tr("{%1}").arg(s));
                }
                bracedList << QString(tr("}"));
            }
            bracedList << QString("}");
        }
        bracedList << QString("}");

        value = QVariant::fromValue(bracedList);
        prop = manager->addProperty(QVariant::StringList, propName);
        propToolTip = tr("QList<QList<QStringList> >");
    }
    else
    {
        NMDebug(<< "standard ");
        value = obj->property(property.name());
        prop = manager->addProperty(propType, propName);
    }

    // add property to browser and set value
    if (prop != 0)
    {
        QtVariantEditorFactory* editor = new QtVariantEditorFactory();

        if (!value.isNull())
            prop->setValue(value);

        if (prop->propertyName().compare("ProcessName") == 0 ||
            prop->propertyName().compare("ComponentName") == 0)
            prop->setEnabled(false);

        if (propToolTip.isEmpty())
            propToolTip = value.typeName();

        prop->setToolTip(QString(tr("type: %1")).arg(propToolTip));
        //prop->setToolTip(QString(tr("type: %1")).arg(value.typeName()));
        //prop->setToolTip(propToolTip);

        mPropBrowser->setFactoryForManager(manager, editor);
        mPropBrowser->addProperty(prop);

        connect(manager, SIGNAL(valueChanged(QtProperty*, const QVariant &)),
                this, SLOT(applySettings(QtProperty*, const QVariant &)));

        connect(manager, SIGNAL(signalCallAuxEditor(QtProperty*, const QStringList &)),
                 this, SLOT(callFeeder(QtProperty*, const QStringList &)));

        NMDebug(<< " - processed!" << std::endl);
    }
    else
    {
        NMDebug(<< " - failed!" << std::endl);
        delete manager;
    }
}

void
NMComponentEditor::callFeeder(QtProperty* prop, const QStringList& val)
{
    NMDebugCtx(ctx, << "...");

    NMDebugAI(<< "Feeder for " << mObj->objectName().toStdString()
              << "'s '" << prop->propertyName().toStdString()
              << "' requested ..." << std::endl);

    NMDebugAI(<< "current value: " << val.join(":").toStdString() << std::endl);

    QVariant nestedList; // = QVariant::fromValue(nestedListFromStringList(val));
    if (!val.isEmpty())
    {
        if (!val.at(0).isEmpty() && !val.at(0).startsWith("invalid"))
        {
            nestedList = this->nestedListFromStringList(val);
        }
    }

    if (!nestedList.isValid())
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }


    if (QString::fromLatin1("QList<QStringList>").compare(QString(nestedList.typeName())) == 0)
    {
        QList<QStringList> _nestedList = nestedList.value<QList<QStringList> >();
        foreach(const QStringList& lst, _nestedList)
        {
            NMDebugAI(<< "  >> " << std::endl);
            foreach(const QString& qstr, lst)
            {
                NMDebugAI(<< "    -- " << qstr.toStdString() << std::endl);
            }
            NMDebugAI(<< "  << " << std::endl);
        }
    }
    else if (QString::fromLatin1("QList<QList<QStringList> >").compare(QString(nestedList.typeName())) == 0)
    {
        QList<QList<QStringList> > _nestedList = nestedList.value<QList<QList<QStringList> > >();
        foreach(const QList<QStringList>& llst, _nestedList)
        {
            NMDebugAI( << "  >> " << std::endl);
            foreach(const QStringList& lst, llst)
            {
                NMDebugAI(<< "    >> " << std::endl);
                foreach(const QString& qstr, lst)
                {
                    NMDebugAI(<< "      -- " << qstr.toStdString() << std::endl);
                }
                NMDebugAI(<< "    << " << std::endl);
            }
            NMDebugAI( << "  << " << std::endl);
        }
    }

    NMDebugCtx(ctx, << "done!");
}

void NMComponentEditor::applySettings(QtProperty* prop,
        const QVariant& val)
{
    NMDebugCtx(ctx, << "...");

    if (comp == 0 && proc == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    NMModelComponent* comp = qobject_cast<NMModelComponent*>(mObj);
    NMIterableComponent* itComp = qobject_cast<NMIterableComponent*>(mObj);
    NMProcess* proc = qobject_cast<NMProcess*>(mObj);

    mUpdating = true;
    if (mObj->property(prop->propertyName().toStdString().c_str()).isValid() ||
        (prop->propertyName().compare("ComponentName")== 0))
    {
        this->setComponentProperty(prop, mObj);
    }
    else if (itComp != 0 && itComp->getProcess() != 0)
    {
        proc = itComp->getProcess();
        if (proc->property(prop->propertyName().toStdString().c_str()).isValid() ||
            (prop->propertyName().compare("ProcessName") == 0))
        {
            this->setComponentProperty(prop, proc);
        }
    }
    else
    {
        this->setComponentProperty(prop, mObj);
    }
    mUpdating = false;
    NMDebugCtx(ctx, << "done!");
}

void NMComponentEditor::setComponentProperty(const QtProperty* prop,
        QObject* obj)
{
    bool bok;
    QStringList ctypes;
    ctypes << "uchar" << "char" << "ushort" << "short"
           << "uint" << "int" << "ulong" << "long"
           << "float" << "double" << "unknown";

    QStringList parammodes;
    parammodes << "NM_USE_UP" << "NM_CYCLE" << "NM_SYNC_WITH_HOST";

    QtVariantPropertyManager* manager =
            qobject_cast<QtVariantPropertyManager*>(prop->propertyManager());
    if (manager == 0)
    {
        NMErr(ctx, << "couldn't get the property manager for "
                << prop->propertyName().toStdString() << "!");
        return;
    }
    QVariant value = manager->value(prop);

    NMDebugAI(<< "setting " << prop->propertyName().toStdString()
            << " (" << value.typeName() << " = " << value.toString().toStdString()
            << ") ... ");

    QVariant updatedValue;
    QString propName = prop->propertyName();

    if (QString("ComponentName").compare(prop->propertyName()) == 0 ||
        QString("ProcessName").compare(prop->propertyName()) == 0)
    {
        propName = "objectName";
        updatedValue = value.toString();
    }
#ifdef BUILD_RASSUPPORT
    else if (QString("RasConnector").compare(prop->propertyName()) == 0)
    {
        if (value.toBool())
        {
            if (this->mRasConn == 0)
            {
                NMErr(ctx, << "rasdaman connector requested, but non available!");
                updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
            }
            else if (this->mRasConn->getConnector() == 0)
            {
                NMErr(ctx, << "rasdaman connector requested, but non available!");
                updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
            }
            else
                updatedValue.setValue<NMRasdamanConnectorWrapper*>(this->mRasConn);
        }
        else
            updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
    }
#endif
    else if (QString("int").compare(value.typeName()) == 0 &&
            prop->propertyName().contains("ComponentType"))
    {
        NMItkDataObjectWrapper::NMComponentType type =
                NMItkDataObjectWrapper::getComponentTypeFromString(ctypes.at(value.toInt(&bok)));
        updatedValue = QVariant::fromValue(type);
    }
    else if (QString("ParameterHandling").compare(prop->propertyName()) == 0)
    {
        int v = value.toInt(&bok);
        NMProcess::AdvanceParameter ap;
        switch(v)
        {
            case 0: ap = NMProcess::NM_USE_UP; break;
            case 1: ap = NMProcess::NM_CYCLE; break;
            case 2: ap = NMProcess::NM_SYNC_WITH_HOST; break;
        }
        updatedValue.setValue<NMProcess::AdvanceParameter>(ap);// = QVariant::fromValue(ap);
    }
    else if (QString("QStringList").compare(value.typeName()) == 0)
    {
        NMDebugAI(<< "incoming stringlist: " << value.toStringList().join(" | ").toStdString() << std::endl);
        QStringList bracedList = value.toStringList();
        bool bvalid = false;
        if (!bracedList.isEmpty())
        {
            if (!bracedList.at(0).isEmpty() && !bracedList.at(0).startsWith("invalid"))
            {
                updatedValue = this->nestedListFromStringList(bracedList);
                bvalid = true;
            }
        }

        // we set an empty value (i.e. override current property set for this object),
        // if the incoming value is not valid; otherwise, no opportunity for the user
        // to remove/clear a once set value, e.g. as it would be the case when a model
        // is loaded from disk.
        if (!bvalid)
        {
           QVariant emptyValue;
           QString longtypename = obj->property(propName.toStdString().c_str()).typeName();
           if (QString("QStringList").compare(longtypename) == 0)
           {
                emptyValue = QVariant::fromValue(QStringList());
           }
           else if (QString("QList<QStringList>").compare(longtypename) == 0)
           {
               emptyValue = QVariant::fromValue(QList<QStringList>());
           }
           else if (QString("QList<QList<QStringList> >").compare(longtypename) == 0)
           {
               emptyValue = QVariant::fromValue(QList< QList<QStringList> >());
           }

           updatedValue = emptyValue;
        }
    }
    else
    {
        updatedValue = value;
    }

    // do some value checking and set the new value
    if (!updatedValue.isNull() && updatedValue.isValid())
    {
        if (QString("Subcomponents").compare(prop->propertyName()) == 0 &&
            QString("QStringList").compare(updatedValue.typeName()) == 0)
        {
            this->updateSubComponents(updatedValue.value<QStringList>());
        }
        else
        {
            obj->setProperty(propName.toStdString().c_str(), updatedValue);
            NMDebugAI(<< "object property updated - type '"
                    << updatedValue.typeName() << "'" << endl);
        }
    }
    else
    {
        NMDebugAI(<< "object property update failed! Invalid value!" << std::endl);
    }
}

void NMComponentEditor::updateSubComponents(const QStringList& compList)
{
    NMIterableComponent* comp = qobject_cast<NMIterableComponent*>(mObj);
    if (comp == 0)
    {
        NMDebugCtx(ctx, << "comp is NULL!");
        return;
    }

    QStringList oldCompNames;
    QList<NMModelComponent*> oldComps;

    NMModelComponent* sub = comp->getInternalStartComponent();
    while(sub != 0)
    {
        oldCompNames.push_back(sub->objectName());
        sub = comp->getNextInternalComponent();
    }

    // remove all components
    // and add store them in a list
    foreach(QString ds, oldCompNames)
    {
        oldComps.append(comp->removeModelComponent(ds));
    }

    // add components in new order, make sure we don't add any new
    // components
    foreach(QString us, compList)
    {
        int idx = oldCompNames.indexOf(us);
        if(idx >=0)
        {
            comp->addModelComponent(oldComps.value(idx));
        }
    }
}

QVariant
NMComponentEditor::nestedListFromStringList(const QStringList& strList)
{
    //NMDebugCtx(ctx, << "...");
    QVariant val;

    // determine the max depth (level) of the stringlist
    int allowedlevel = 3;
    int maxlevel = 0;
    int levelcounter = 0;
    foreach(QString ts, strList)
    {
        if (ts.compare("{") == 0)// || ts.startsWith("{"))
        {
            ++levelcounter;
            maxlevel = levelcounter > maxlevel && levelcounter <= allowedlevel ? levelcounter : maxlevel;
        }
        else if (ts.compare("}") == 0)// || ts.endsWith("}"))
        {
            --levelcounter;
        }
    }
    //NMDebugAI( << "detected maxlevel: " << maxlevel << std::endl);
    //NMDebugAI( << "parsing list ..." << std::endl << std::endl);

    QList<QList<QStringList> >* llsl;
    QList<QStringList>* lsl;
    QStringList* sl;

    levelcounter = maxlevel;
    foreach(QString ts, strList)
    {
//		NMDebugAI(<< "'" << ts.toStdString() << "' on level " << levelcounter);
        if (ts.compare("{") == 0)
        {
            switch (levelcounter)
            {
            case 3:
//				NMDebug(<< " -> new llsl" << std::endl);
                llsl = new QList<QList<QStringList> >();
                break;
            case 2:
//				NMDebug(<< " -> new lsl" << std::endl);
                lsl = new QList<QStringList>();
                break;
            case 1:
//				NMDebug(<< " -> new sl" << std::endl);
                sl = new QStringList();
                break;
            default:
//				NMDebug(<< " - > can't handle this one!" << std::endl);
                continue;
                break;
            }
            --levelcounter;
        }
        else if (ts.compare("}") == 0)
        {
            switch (levelcounter)
            {
            case 0:
                if (maxlevel > 1)
                {
//					NMDebug(<< " -> close ")
                    lsl->push_back(*sl);
                    sl->clear();
                    delete sl;
                }
                break;
            case 1:
                if (maxlevel > 2)
                {
                    llsl->push_back(*lsl);
                    lsl->clear();
                    delete lsl;
                }
                break;
            default:
                break;
            }
            ++levelcounter;
        }
        else if (ts.startsWith("{")
                 && ts.endsWith("}")
                   && levelcounter == 0)
        {
            QString ns = ts.mid(1,ts.size()-2); //ts.remove(0,1);
            sl->push_back(ns);
//			NMDebugAI( << ">>> pushed '" << ns.toStdString() << "'" << std::endl);
        }
    }

    switch(maxlevel)
    {
    case 3:
        val = QVariant::fromValue(*llsl);
        llsl->clear();
        delete llsl;
        break;
    case 2:
        val = QVariant::fromValue(*lsl);
        lsl->clear();
        delete lsl;
        break;
    case 1:
        val = QVariant::fromValue(*sl);
        sl->clear();
        delete sl;
        break;
    default:
        break;
    }

    //NMDebugCtx(ctx, << "done!");
    return val;
}


