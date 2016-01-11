#include "NMVtkMapScale.h"

#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"
#include "vtkRenderingAnnotationModule.h"

const std::string NMVtkMapScale::ctx = "NMVtkMapScale";
//#define ctxNMVtkMapScale = "NMVtkMapScale";


//vtkSandardNewMacro(NMVtkMapScale);

NMVtkMapScale*
NMVtkMapScale::New()
{
    return new NMVtkMapScale();
}


NMVtkMapScale::NMVtkMapScale()
    : vtkLegendScaleActor()
{
}

NMVtkMapScale::~NMVtkMapScale()
{
}

void
NMVtkMapScale::BuildRepresentation(vtkViewport *viewport)
{
    Superclass::BuildRepresentation(viewport);


    int* size = viewport->GetSize();

    //NMDebugAI(<< "size: " << size[0] << ", " << size[1] << std::endl);



}
