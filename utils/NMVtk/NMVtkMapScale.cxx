#include "NMVtkMapScale.h"

#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkActor2D.h"
#include "vtkTextMapper.h"

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



    if ( this->LegendVisibility )
    {

        double xr1 = 0.33333*size[0];
        double xr2 = 0.66666*size[0];
        this->Coordinate->SetValue(0.33333*size[0],15,0.0);
        double *xr = this->Coordinate->GetComputedWorldValue(viewport);
        double xrL[3]; xrL[0]=xr[0];xrL[1]=xr[1];xrL[2]=xr[2];
        this->Coordinate->SetValue(0.66667*size[0],15,0.0);
        xr = this->Coordinate->GetComputedWorldValue(viewport);
        double xrR[3]; xrR[0]=xr[0];xrR[1]=xr[1];xrR[2]=xr[2];
        double rlen = sqrt(vtkMath::Distance2BetweenPoints(xrL,xrR));




        // Update the position
        double x1 = 0.33333*size[0];
        double delX = x1/4.0;

        this->LegendPoints->SetPoint(0, x1,10,0);
        this->LegendPoints->SetPoint(1, x1+delX,10,0);
        this->LegendPoints->SetPoint(2, x1+2*delX,10,0);
        this->LegendPoints->SetPoint(3, x1+3*delX,10,0);
        this->LegendPoints->SetPoint(4, x1+4*delX,10,0);
        this->LegendPoints->SetPoint(5, x1,20,0);
        this->LegendPoints->SetPoint(6, x1+delX,20,0);
        this->LegendPoints->SetPoint(7, x1+2*delX,20,0);
        this->LegendPoints->SetPoint(8, x1+3*delX,20,0);
        this->LegendPoints->SetPoint(9, x1+4*delX,20,0);

        // Specify the position of the legend title
        this->LabelActors[5]->SetPosition(0.5*size[0],22);
        this->Coordinate->SetValue(0.33333*size[0],15,0.0);
        double *x = this->Coordinate->GetComputedWorldValue(viewport);
        double xL[3]; xL[0]=x[0];xL[1]=x[1];xL[2]=x[2];
        this->Coordinate->SetValue(0.66667*size[0],15,0.0);
        x = this->Coordinate->GetComputedWorldValue(viewport);
        double xR[3]; xR[0]=x[0];xR[1]=x[1];xR[2]=x[2];
        double len = sqrt(vtkMath::Distance2BetweenPoints(xL,xR));
        char buf[256];
        sprintf(buf,"1 : %d",vtkMath::Round(len));
        this->LabelMappers[5]->SetInput(buf);


        // Now specify the position of the legend labels
        x = this->LegendPoints->GetPoint(0);
        this->LabelActors[0]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(1);
        this->LabelActors[1]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(2);
        this->LabelActors[2]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(3);
        this->LabelActors[3]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(4);
        this->LabelActors[4]->SetPosition(x[0],x[1]-1);

        double lenIncr = len / 4.0;

        this->LabelMappers[0]->SetInput("0");
        sprintf(buf, "%g", (lenIncr));
        this->LabelMappers[1]->SetInput(buf);
        sprintf(buf, "%g", (2*lenIncr));
        this->LabelMappers[2]->SetInput(buf);
        sprintf(buf, "%g", (3*lenIncr));
        this->LabelMappers[3]->SetInput(buf);
        sprintf(buf, "%g", (4*lenIncr));
        this->LabelMappers[4]->SetInput(buf);


    }

}
