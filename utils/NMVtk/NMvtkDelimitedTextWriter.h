#ifndef NMVTKDELIMITEDTEXTWRITER_H
#define NMVTKDELIMITEDTEXTWRITER_H

#include "vtkDelimitedTextWriter.h"


class NMvtkDelimitedTextWriter : public vtkDelimitedTextWriter
{
public:
    static NMvtkDelimitedTextWriter* New();
    vtkTypeMacro(NMvtkDelimitedTextWriter, vtkDelimitedTextWriter);

protected:
    NMvtkDelimitedTextWriter(){}
    ~NMvtkDelimitedTextWriter() override {}

    virtual void WriteTable(vtkTable* table);

private:
    NMvtkDelimitedTextWriter(const NMvtkDelimitedTextWriter&) = delete;
    void operator=(const NMvtkDelimitedTextWriter&) = delete;
};

#endif // NMVTKDELIMITEDTEXTWRITER_H
