#ifndef NMWRAPPERFACTORY_H
#define NMWRAPPERFACTORY_H

#include <QObject>
#include "NMProcess.h"

#include "nmmodframecore_export.h"

class NMMODFRAMECORE_EXPORT NMWrapperFactory : public QObject
{
    Q_OBJECT
public:
    NMWrapperFactory(QObject *parent = nullptr);
    virtual NMProcess* createWrapper(void)=0;
    virtual bool isSinkProcess(void)=0;
    virtual QString getWrapperClassName(void)=0;

protected:


};

#endif // NMWRAPPERFACTORY_H
