/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderViewBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderViewBase.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkDataRepresentation.h"

vtkStandardNewMacro(vtkRenderViewBase);

vtkRenderViewBase::vtkRenderViewBase()
{
  this->RenderOnMouseMove = false;
  this->InteractionMode = -1;
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  this->RenderWindow->AddRenderer(this->Renderer);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  this->SetInteractor(iren);

  this->SetInteractionMode(INTERACTION_MODE_2D);
}

vtkRenderViewBase::~vtkRenderViewBase()
{
}

vtkRenderer* vtkRenderViewBase::GetRenderer()
{
  return this->Renderer;
}

vtkRenderWindow* vtkRenderViewBase::GetRenderWindow()
{
  return this->RenderWindow;
}

void vtkRenderViewBase::SetRenderWindow(vtkRenderWindow* win)
{
  if (!win)
    {
    vtkErrorMacro(<< "SetRenderWindow called with a null window pointer."
                  << " That can't be right.");
    return;
    }

  // get rid of the render observer on the current window
  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveObserver(this->GetObserver());
    }

  // move renderers to new window
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  while(rens->GetNumberOfItems())
    {
    vtkRenderer* ren = rens->GetFirstRenderer();
    ren->SetRenderWindow(NULL);
    win->AddRenderer(ren);
    this->RenderWindow->RemoveRenderer(ren);
    }

  this->RenderWindow = win;
  this->RenderWindow->AddObserver(vtkCommand::EndEvent, this->GetObserver());

  if (win->GetInteractor())
    {
    // this will ensure that the interactor gets setup correctly.
    this->SetInteractor(win->GetInteractor());
    }
  else
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    this->SetInteractor(iren);
    iren->Delete();
    }

}

vtkRenderWindowInteractor* vtkRenderViewBase::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

void vtkRenderViewBase::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (!interactor)
    {
    vtkErrorMacro(<< "SetInteractor called with a null interactor pointer."
      << " That can't be right.");
    return;
    }

  if (this->Interactor == interactor)
    {
    return;
    }

  // get rid of the render observer on any current interactor
  if (this->Interactor)
    {
    this->Interactor->RemoveObserver(this->GetObserver());
    this->Interactor->SetRenderWindow(NULL);
    this->Interactor = 0;
    }

  this->Interactor = interactor;

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  interactor->EnableRenderOff();
  interactor->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::StartInteractionEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::EndInteractionEvent, this->GetObserver());
  this->RenderWindow->SetInteractor(interactor);

  // Pass on the interactor style.
  interactor->SetInteractorStyle(this->GetInteractorStyle());
}

vtkInteractorObserver* vtkRenderViewBase::GetInteractorStyle()
{
  return this->InteractorStyle;
}

void vtkRenderViewBase::SetInteractorStyle(vtkInteractorObserver* style)
{
  if (!style)
    {
    vtkErrorMacro("Interactor style must not be null.");
    return;
    }
  vtkInteractorObserver* oldStyle = this->GetInteractorStyle();
  if (style != oldStyle)
    {
    if (oldStyle)
      {
      oldStyle->RemoveObserver(this->GetObserver());
      }
    this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
    this->InteractorStyle = style;
    style->AddObserver(
      vtkCommand::SelectionChangedEvent, this->GetObserver());
    vtkInteractorStyleRubberBand2D* style2D =
      vtkInteractorStyleRubberBand2D::SafeDownCast(style);
    vtkInteractorStyleRubberBand3D* style3D =
      vtkInteractorStyleRubberBand3D::SafeDownCast(style);
    if (style2D)
      {
      style2D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_2D;
      }
    else if (style3D)
      {
      style3D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_3D;
      }
    else
      {
      this->InteractionMode = INTERACTION_MODE_UNKNOWN;
      }
    }
}

void vtkRenderViewBase::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
    {
    this->InteractionMode = mode;
    if (mode == INTERACTION_MODE_2D)
      {
      vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
      this->SetInteractorStyle(style);
      style->Delete();

      this->Renderer->GetActiveCamera()->ParallelProjectionOn();
      }
    else if (mode == INTERACTION_MODE_3D)
      {
      vtkInteractorStyleRubberBand3D* style = vtkInteractorStyleRubberBand3D::New();
      this->SetInteractorStyle(style);
      style->Delete();

      this->Renderer->GetActiveCamera()->ParallelProjectionOff();
      }
    else
      {
      vtkErrorMacro("Unknown interaction mode.");
      }
    }
}

void vtkRenderViewBase::SetRenderOnMouseMove(bool b)
{
  if (b == this->RenderOnMouseMove)
    {
    return;
    }

  vtkInteractorObserver* style = this->GetInteractorStyle();
  vtkInteractorStyleRubberBand2D* style2D =
    vtkInteractorStyleRubberBand2D::SafeDownCast(style);
  if (style2D)
    {
    style2D->SetRenderOnMouseMove(b);
    }
  vtkInteractorStyleRubberBand3D* style3D =
    vtkInteractorStyleRubberBand3D::SafeDownCast(style);
  if (style3D)
    {
    style3D->SetRenderOnMouseMove(b);
    }
  this->RenderOnMouseMove = b;
}

void vtkRenderViewBase::Render()
{
  // Indirectly call this->RenderWindow->Start() without crashing.
  // to create context if it is not yet created and to make it current
  // this is required for HoverWidget to be active after the first
  // render.
  this->RenderWindow->GetInteractor()->Initialize();

  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();
}

void vtkRenderViewBase::ResetCamera()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}

void vtkRenderViewBase::ResetCameraClippingRange()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
}

void vtkRenderViewBase::PrepareForRendering()
{
  this->Update();
}

void vtkRenderViewBase::ProcessEvents(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->GetInteractor() && eventId == vtkCommand::RenderEvent)
    {
    vtkDebugMacro(<< "interactor causing a render event.");
    this->Render();
    }
  if (vtkDataRepresentation::SafeDownCast(caller) &&
      eventId == vtkCommand::SelectionChangedEvent)
    {
    vtkDebugMacro("selection changed causing a render event");
    this->Render();
    }
  else if (vtkDataRepresentation::SafeDownCast(caller) &&
           eventId == vtkCommand::UpdateEvent)
    {
    // UpdateEvent is called from push pipeline executions from
    // vtkExecutionScheduler. We want to automatically render the view
    // when one of our representations is updated.
    vtkDebugMacro("push pipeline causing a render event");
    this->Render();
    }
  this->Superclass::ProcessEvents(caller, eventId, callData);
}

void vtkRenderViewBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
    {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Renderer: ";
  if (this->Renderer)
    {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "InteractionMode: " << this->InteractionMode << endl;
  os << indent << "RenderOnMouseMove: " << this->RenderOnMouseMove << endl;
}
