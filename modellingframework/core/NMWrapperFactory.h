#ifndef NMWRAPPERFACTORY_H
#define NMWRAPPERFACTORY_H

#include <QObject>
#include "NMProcess.h"

#if defined _WIN32
    #define WINCALL __stdcall
#else
    #define WINCALL
#endif

#include "nmmodframecore_export.h"

class NMMODFRAMECORE_EXPORT NMWrapperFactory : public QObject
{
    Q_OBJECT
public:
    NMWrapperFactory(QObject *parent = nullptr);
    virtual NMProcess* createWrapper(void)=0;
    virtual bool isSinkProcess(void)=0;
    virtual QString getWrapperClassName(void)=0;
    virtual QString getComponentAlias(void)=0;
    virtual QString getComponentInfo(void) {return QStringLiteral("");}

};

#endif // NMWRAPPERFACTORY_H
