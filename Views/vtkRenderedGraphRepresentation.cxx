/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedGraphRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkRenderedGraphRepresentation.h"

#include "vtkAbstractTransform.h"
#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotationLink.h"
#include "vtkApplyColors.h"
#include "vtkArcParallelEdgeStrategy.h"
#include "vtkArrayMap.h"
#include "vtkAssignCoordinatesLayoutStrategy.h"
#include "vtkCellData.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkClustering2DLayoutStrategy.h"
#include "vtkCommunity2DLayoutStrategy.h"
#include "vtkConeLayoutStrategy.h"
#include "vtkConvertSelection.h"
#include "vtkCosmicTreeLayoutStrategy.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeLayout.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGeoEdgeStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToGlyphs.h"
#include "vtkGraphToPoints.h"
#include "vtkGraphToPolyData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPassThroughEdgeStrategy.h"
#include "vtkPassThroughLayoutStrategy.h"
#include "vtkPerturbCoincidentVertices.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomLayoutStrategy.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"

#include <vtkstd/algorithm>

vtkCxxRevisionMacro(vtkRenderedGraphRepresentation, "1.2");
vtkStandardNewMacro(vtkRenderedGraphRepresentation);

vtkRenderedGraphRepresentation::vtkRenderedGraphRepresentation()
{
  this->ApplyColors         = vtkSmartPointer<vtkApplyColors>::New();
  this->VertexDegree        = vtkSmartPointer<vtkVertexDegree>::New();
  this->EmptyPolyData       = vtkSmartPointer<vtkPolyData>::New();
  this->EdgeCenters         = vtkSmartPointer<vtkEdgeCenters>::New();
  this->GraphToPoints       = vtkSmartPointer<vtkGraphToPoints>::New();
  this->VertexLabels        = vtkSmartPointer<vtkArrayMap>::New();
  this->EdgeLabels          = vtkSmartPointer<vtkArrayMap>::New();
  this->VertexLabelPriority = vtkSmartPointer<vtkArrayMap>::New();
  this->EdgeLabelPriority   = vtkSmartPointer<vtkArrayMap>::New();
  this->VertexTextProperty  = vtkSmartPointer<vtkTextProperty>::New();
  this->EdgeTextProperty    = vtkSmartPointer<vtkTextProperty>::New();
  this->VertexIcons         = vtkSmartPointer<vtkArrayMap>::New();
  this->EdgeIcons           = vtkSmartPointer<vtkArrayMap>::New();
  this->VertexIconPriority  = vtkSmartPointer<vtkArrayMap>::New();
  this->EdgeIconPriority    = vtkSmartPointer<vtkArrayMap>::New();
  this->Layout              = vtkSmartPointer<vtkGraphLayout>::New();
  this->Coincident          = vtkSmartPointer<vtkPerturbCoincidentVertices>::New();
  this->EdgeLayout          = vtkSmartPointer<vtkEdgeLayout>::New();
  this->GraphToPoly         = vtkSmartPointer<vtkGraphToPolyData>::New();
  this->EdgeMapper          = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->EdgeActor           = vtkSmartPointer<vtkActor>::New();
  this->VertexGlyph         = vtkSmartPointer<vtkGraphToGlyphs>::New();
  this->VertexMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->VertexActor         = vtkSmartPointer<vtkActor>::New();
  this->OutlineGlyph        = vtkSmartPointer<vtkGraphToGlyphs>::New();
  this->OutlineMapper       = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->OutlineActor        = vtkSmartPointer<vtkActor>::New();
  this->VertexScalarBar     = vtkSmartPointer<vtkScalarBarWidget>::New();
  this->EdgeScalarBar       = vtkSmartPointer<vtkScalarBarWidget>::New();

  this->VertexColorArrayNameInternal = 0;
  this->EdgeColorArrayNameInternal = 0;
  this->ScalingArrayNameInternal = 0;
  this->LayoutStrategyName = 0;
  this->EdgeLayoutStrategyName = 0;

  /*
   <graphviz>
   digraph {
     Layout -> Coincident -> EdgeLayout -> VertexDegree -> ApplyColors
     ApplyColors -> VertexGlyph -> VertexMapper -> VertexActor
     ApplyColors -> GraphToPoly -> EdgeMapper -> EdgeActor
     Coincident -> OutlineGlyph -> OutlineMapper -> OutlineActor
     
     VertexDegree -> GraphToPoints
     GraphToPoints -> VertexLabels -> VertexLabelPriority -> "vtkRenderView Labels"
     GraphToPoints -> VertexIcons -> VertexIconPriority -> "vtkRenderView Icons"
     VertexDegree -> EdgeCenters
     EdgeCenters -> EdgeLabels -> EdgeLabelPriority -> "vtkRenderView Labels"
     EdgeCenters -> EdgeIcons -> EdgeIconPriority -> "vtkRenderView Icons"
   }
   </graphviz>
  */

  this->Coincident->SetInputConnection(this->Layout->GetOutputPort());
  this->EdgeLayout->SetInputConnection(this->Coincident->GetOutputPort());
  this->VertexDegree->SetInputConnection(this->EdgeLayout->GetOutputPort());
  this->ApplyColors->SetInputConnection(this->VertexDegree->GetOutputPort());

  // Vertex actor
  this->VertexGlyph->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);

  // Outline actor
  this->OutlineGlyph->SetInputConnection(this->Coincident->GetOutputPort());
  this->OutlineMapper->SetInputConnection(this->OutlineGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);

  // Edge actor
  this->GraphToPoly->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->EdgeMapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);

  this->GraphToPoints->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeCenters->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->VertexLabels->SetInput(this->EmptyPolyData);
  this->EdgeLabels->SetInput(this->EmptyPolyData);
  this->VertexIcons->SetInput(this->EmptyPolyData);
  this->EdgeIcons->SetInput(this->EmptyPolyData);
  this->VertexLabelPriority->SetInputConnection(this->VertexLabels->GetOutputPort());
  this->EdgeLabelPriority->SetInputConnection(this->EdgeLabels->GetOutputPort());
  this->VertexIconPriority->SetInputConnection(this->VertexIcons->GetOutputPort());
  this->EdgeIconPriority->SetInputConnection(this->EdgeIcons->GetOutputPort());

  // Set default parameters
  vtkSmartPointer<vtkDirectedGraph> g =
    vtkSmartPointer<vtkDirectedGraph>::New();
  this->Layout->SetInput(g);
  vtkSmartPointer<vtkFast2DLayoutStrategy> strategy =
    vtkSmartPointer<vtkFast2DLayoutStrategy>::New();
  this->Layout->SetLayoutStrategy(strategy);
  //this->Layout->SetZRange(0.001);
  this->Layout->SetZRange(0.0);
  vtkSmartPointer<vtkArcParallelEdgeStrategy> edgeStrategy =
    vtkSmartPointer<vtkArcParallelEdgeStrategy>::New();
  this->Layout->UseTransformOn();
  this->SetVertexColorArrayName("VertexDegree");
  this->SetVertexLabelArrayName("VertexDegree");
  this->SetVertexLabelPriorityArrayName("VertexDegree");
  this->SetVertexIconArrayName("IconIndex");
  this->SetVertexIconPriorityArrayName("VertexDegree");
  this->EdgeLayout->SetLayoutStrategy(edgeStrategy);

  this->VertexGlyph->FilledOn();
  this->VertexGlyph->SetGlyphType(vtkGraphToGlyphs::VERTEX);
  this->VertexMapper->SetScalarModeToUseCellFieldData();
  this->VertexMapper->SelectColorArray("vtkApplyColors color");
  this->VertexMapper->SetScalarVisibility(true);

  this->OutlineGlyph->FilledOff();
  this->OutlineGlyph->SetGlyphType(vtkGraphToGlyphs::VERTEX);
  this->OutlineMapper->SetScalarVisibility(false);
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->FrontfaceCullingOn();

  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray("vtkApplyColors color");
  this->EdgeMapper->SetScalarVisibility(true);
  this->EdgeActor->SetPosition(0, 0, -0.003);

  this->VertexTextProperty->BoldOn();
  this->VertexTextProperty->SetJustificationToCentered();
  this->VertexTextProperty->SetVerticalJustificationToCentered();
  this->VertexTextProperty->SetFontSize(12);
  this->EdgeTextProperty->BoldOn();
  this->EdgeTextProperty->SetJustificationToCentered();
  this->EdgeTextProperty->SetVerticalJustificationToCentered();
  this->EdgeTextProperty->SetFontSize(10);

  this->VertexIcons->SetFieldType(vtkArrayMap::POINT_DATA);
  this->VertexIcons->SetOutputArrayType(VTK_INT);
  this->VertexIcons->SetOutputArrayName("IconIndex");
  this->VertexIcons->PassArrayOn();
  this->VertexIconPriority->SetFieldType(vtkArrayMap::POINT_DATA);
  this->VertexIconPriority->SetOutputArrayType(VTK_DOUBLE);
  this->VertexIconPriority->SetOutputArrayName("Priority");
  this->VertexIconPriority->PassArrayOn();
  this->EdgeIcons->SetFieldType(vtkArrayMap::POINT_DATA);
  this->EdgeIcons->SetOutputArrayType(VTK_INT);
  this->EdgeIcons->SetOutputArrayName("IconIndex");
  this->EdgeIcons->PassArrayOn();
  this->EdgeIconPriority->SetFieldType(vtkArrayMap::POINT_DATA);
  this->EdgeIconPriority->SetOutputArrayType(VTK_DOUBLE);
  this->EdgeIconPriority->SetOutputArrayName("Priority");
  this->EdgeIconPriority->PassArrayOn();
  this->VertexLabels->SetFieldType(vtkArrayMap::POINT_DATA);
  this->VertexLabels->SetOutputArrayType(VTK_STRING);
  this->VertexLabels->SetOutputArrayName("LabelText");
  this->VertexLabels->PassArrayOn();
  this->VertexLabelPriority->SetFieldType(vtkArrayMap::POINT_DATA);
  this->VertexLabelPriority->SetOutputArrayType(VTK_DOUBLE);
  this->VertexLabelPriority->SetOutputArrayName("Priority");
  this->VertexLabelPriority->PassArrayOn();
  this->EdgeLabels->SetFieldType(vtkArrayMap::POINT_DATA);
  this->EdgeLabels->SetOutputArrayType(VTK_STRING);
  this->EdgeLabels->SetOutputArrayName("LabelText");
  this->EdgeLabels->PassArrayOn();
  this->EdgeLabelPriority->SetFieldType(vtkArrayMap::POINT_DATA);
  this->EdgeLabelPriority->SetOutputArrayType(VTK_DOUBLE);
  this->EdgeLabelPriority->SetOutputArrayName("Priority");
  this->EdgeLabelPriority->PassArrayOn();
  this->VertexScalarBar->GetScalarBarActor()->VisibilityOff();
  this->EdgeScalarBar->GetScalarBarActor()->VisibilityOff();

  vtkSmartPointer<vtkViewTheme> theme =
    vtkSmartPointer<vtkViewTheme>::New();
  this->ApplyViewTheme(theme);
}

vtkRenderedGraphRepresentation::~vtkRenderedGraphRepresentation()
{
  this->SetScalingArrayNameInternal(0);
  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
  this->SetLayoutStrategyName(0);
  this->SetEdgeLayoutStrategyName(0);
}

void vtkRenderedGraphRepresentation::SetupRenderWindow(vtkRenderWindow* win)
{
  this->VertexScalarBar->SetInteractor(win->GetInteractor());
  this->EdgeScalarBar->SetInteractor(win->GetInteractor());
}

void vtkRenderedGraphRepresentation::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabels->SetInputArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabels->SetInputArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexLabelArrayName()
{
  return this->VertexLabels->GetInputArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeLabelArrayName()
{
  return this->EdgeLabels->GetInputArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexLabelPriorityArrayName(const char* name)
{
  this->VertexLabelPriority->SetInputArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeLabelPriorityArrayName(const char* name)
{
  this->EdgeLabelPriority->SetInputArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexLabelPriorityArrayName()
{
  return this->VertexLabelPriority->GetInputArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeLabelPriorityArrayName()
{
  return this->EdgeLabelPriority->GetInputArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexLabelVisibility(bool b)
{
  if (b)
    {
    this->VertexLabels->SetInputConnection(this->GraphToPoints->GetOutputPort());
    }
  else
    {
    this->VertexLabels->SetInput(this->EmptyPolyData);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeLabelVisibility(bool b)
{
  if (b)
    {
    this->EdgeLabels->SetInputConnection(this->EdgeCenters->GetOutputPort());
    }
  else
    {
    this->EdgeLabels->SetInput(this->EmptyPolyData);
    }
}

bool vtkRenderedGraphRepresentation::GetVertexLabelVisibility()
{
  return this->VertexLabels->GetInputConnection(0, 0) ==
         this->GraphToPoints->GetOutputPort();
}

bool vtkRenderedGraphRepresentation::GetEdgeLabelVisibility()
{
  return this->EdgeLabels->GetInputConnection(0, 0) ==
         this->EdgeCenters->GetOutputPort();
}

void vtkRenderedGraphRepresentation::SetEdgeVisibility(bool b)
{
  this->EdgeActor->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetEdgeVisibility()
{
  return this->EdgeActor->GetVisibility() ? true : false;
}

void vtkRenderedGraphRepresentation::SetVertexLabelTextProperty(vtkTextProperty* p)
{
  if (p)
    {
    this->VertexTextProperty->ShallowCopy(p);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeLabelTextProperty(vtkTextProperty* p)
{
  if (p)
    {
    this->EdgeTextProperty->ShallowCopy(p);
    }
}

vtkTextProperty* vtkRenderedGraphRepresentation::GetVertexLabelTextProperty()
{
  return this->VertexTextProperty;
}

vtkTextProperty* vtkRenderedGraphRepresentation::GetEdgeLabelTextProperty()
{
  return this->EdgeTextProperty;
}

void vtkRenderedGraphRepresentation::SetVertexIconArrayName(const char* name)
{
  this->VertexIcons->SetInputArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeIconArrayName(const char* name)
{
  this->EdgeIcons->SetInputArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexIconArrayName()
{
  return this->VertexIcons->GetInputArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeIconArrayName()
{
  return this->EdgeIcons->GetInputArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexIconPriorityArrayName(const char* name)
{
  this->VertexIconPriority->SetInputArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeIconPriorityArrayName(const char* name)
{
  this->EdgeIconPriority->SetInputArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexIconPriorityArrayName()
{
  return this->VertexIconPriority->GetInputArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeIconPriorityArrayName()
{
  return this->EdgeIconPriority->GetInputArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexIconVisibility(bool b)
{
  if (b)
    {
    this->VertexIcons->SetInputConnection(this->GraphToPoints->GetOutputPort());
    }
  else
    {
    this->VertexIcons->SetInput(this->EmptyPolyData);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeIconVisibility(bool b)
{
  if (b)
    {
    this->EdgeIcons->SetInputConnection(this->EdgeCenters->GetOutputPort());
    }
  else
    {
    this->EdgeIcons->SetInput(this->EmptyPolyData);
    }
}

bool vtkRenderedGraphRepresentation::GetVertexIconVisibility()
{
  return this->VertexIcons->GetInputConnection(0, 0) ==
         this->GraphToPoints->GetOutputPort();
}

bool vtkRenderedGraphRepresentation::GetEdgeIconVisibility()
{
  return this->EdgeIcons->GetInputConnection(0, 0) !=
         this->EdgeCenters->GetOutputPort();
}

void vtkRenderedGraphRepresentation::AddVertexIconType(const char* name, int type)
{
  this->VertexIcons->AddToMap(name, type);
}

void vtkRenderedGraphRepresentation::AddEdgeIconType(const char* name, int type)
{
  this->EdgeIcons->AddToMap(name, type);
}

void vtkRenderedGraphRepresentation::ClearVertexIconTypes()
{
  this->VertexIcons->ClearMap();
}

void vtkRenderedGraphRepresentation::ClearEdgeIconTypes()
{
  this->EdgeIcons->ClearMap();
}

void vtkRenderedGraphRepresentation::SetUseVertexIconTypeMap(bool b)
{
  if (b)
    {
    this->VertexIcons->PassArrayOff();
    this->VertexIcons->SetFillValue(-1);
    }
  else
    {
    this->ClearVertexIconTypes();
    this->VertexIcons->PassArrayOn();
    }
}

void vtkRenderedGraphRepresentation::SetUseEdgeIconTypeMap(bool b)
{
  if (b)
    {
    this->EdgeIcons->PassArrayOff();
    this->EdgeIcons->SetFillValue(-1);
    }
  else
    {
    this->ClearEdgeIconTypes();
    this->EdgeIcons->PassArrayOn();
    }
}

bool vtkRenderedGraphRepresentation::GetUseVertexIconTypeMap()
{
  return this->VertexIcons->GetPassArray() ? true : false;
}

bool vtkRenderedGraphRepresentation::GetUseEdgeIconTypeMap()
{
  return this->EdgeIcons->GetPassArray() ? true : false;
}

// TODO: Icon alignment
void vtkRenderedGraphRepresentation::SetVertexIconAlignment(int align)
{
  (void)align;
}

int vtkRenderedGraphRepresentation::GetVertexIconAlignment()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetEdgeIconAlignment(int align)
{
  (void)align;
}

int vtkRenderedGraphRepresentation::GetEdgeIconAlignment()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetColorVerticesByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool vtkRenderedGraphRepresentation::GetColorVerticesByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void vtkRenderedGraphRepresentation::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

const char* vtkRenderedGraphRepresentation::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

void vtkRenderedGraphRepresentation::SetColorEdgesByArray(bool b)
{
  this->ApplyColors->SetUseCellLookupTable(b);
}

bool vtkRenderedGraphRepresentation::GetColorEdgesByArray()
{
  return this->ApplyColors->GetUseCellLookupTable();
}

void vtkRenderedGraphRepresentation::SetEdgeColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(1, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_EDGES, name);
}

const char* vtkRenderedGraphRepresentation::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

// TODO: Implement enable stuff.
void vtkRenderedGraphRepresentation::SetEnableVerticesByArray(bool b)
{
  (void)b;
}

bool vtkRenderedGraphRepresentation::GetEnableVerticesByArray()
{
  return false;
}

void vtkRenderedGraphRepresentation::SetEnabledVerticesArrayName(const char* name)
{
  (void)name;
}

const char* vtkRenderedGraphRepresentation::GetEnabledVerticesArrayName()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetEnableEdgesByArray(bool b)
{
  (void)b;
}

bool vtkRenderedGraphRepresentation::GetEnableEdgesByArray()
{
  return false;
}

void vtkRenderedGraphRepresentation::SetEnabledEdgesArrayName(const char* name)
{
  (void)name;
}

const char* vtkRenderedGraphRepresentation::GetEnabledEdgesArrayName()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetGlyphType(int type)
{
  if (type != this->VertexGlyph->GetGlyphType())
    {
    this->VertexGlyph->SetGlyphType(type);
    this->OutlineGlyph->SetGlyphType(type);
    if (type == vtkGraphToGlyphs::SPHERE)
      {
      this->OutlineActor->GetProperty()->FrontfaceCullingOn();
      }
    else
      {
      this->OutlineActor->GetProperty()->FrontfaceCullingOff();
      }
    }
}

int vtkRenderedGraphRepresentation::GetGlyphType()
{
  return this->VertexGlyph->GetGlyphType();
}

void vtkRenderedGraphRepresentation::SetScaling(bool b)
{
  this->VertexGlyph->SetScaling(b);
  this->OutlineGlyph->SetScaling(b);
}

bool vtkRenderedGraphRepresentation::GetScaling()
{
  return this->VertexGlyph->GetScaling();
}

void vtkRenderedGraphRepresentation::SetScalingArrayName(const char* name)
{
  this->VertexGlyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->OutlineGlyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->SetScalingArrayNameInternal(name);
}

const char* vtkRenderedGraphRepresentation::GetScalingArrayName()
{
  return this->GetScalingArrayNameInternal();
}

void vtkRenderedGraphRepresentation::SetVertexScalarBarVisibility(bool b)
{
  this->VertexScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetVertexScalarBarVisibility()
{
  return this->VertexScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

void vtkRenderedGraphRepresentation::SetEdgeScalarBarVisibility(bool b)
{
  this->EdgeScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetEdgeScalarBarVisibility()
{
  return this->EdgeScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

bool vtkRenderedGraphRepresentation::IsLayoutComplete()
{
  return this->Layout->IsLayoutComplete() ? true : false;
}

void vtkRenderedGraphRepresentation::UpdateLayout()
{
  if (!this->IsLayoutComplete())
    {
    this->Layout->Modified();
    // TODO : Should render here??
    }
}

void vtkRenderedGraphRepresentation::SetLayoutStrategy(vtkGraphLayoutStrategy* s)
{
  if (!s)
    {
    vtkErrorMacro("Layout strategy must not be NULL.");
    return;
    }
  if (vtkRandomLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Random");
    }
  else if (vtkForceDirectedLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Force Directed");
    }
  else if (vtkSimple2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Simple 2D");
    }
  else if (vtkClustering2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Clustering 2D");
    }
  else if (vtkCommunity2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Community 2D");
    }
  else if (vtkFast2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Fast 2D");
    }
  else if (vtkCircularLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Circular");
    }
  else if (vtkTreeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Tree");
    }
  else if (vtkCosmicTreeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Cosmic Tree");
    }
  else if (vtkPassThroughLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Pass Through");
    }
  else if (vtkConeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Cone");
    }
  else
    {
    this->SetLayoutStrategyName("Unknown");
    }
  this->Layout->SetLayoutStrategy(s);
}

vtkGraphLayoutStrategy* vtkRenderedGraphRepresentation::GetLayoutStrategy()
{
  return this->Layout->GetLayoutStrategy();
}

void vtkRenderedGraphRepresentation::SetLayoutStrategy(const char* name)
{
  vtkstd::string str = name;
  vtkstd::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(vtkstd::remove(str.begin(), str.end(), ' '), str.end());
  vtkSmartPointer<vtkGraphLayoutStrategy> strategy =
    vtkSmartPointer<vtkPassThroughLayoutStrategy>::New();
  if (str == "random")
    {
    strategy = vtkSmartPointer<vtkRandomLayoutStrategy>::New();
    }
  else if (str == "forcedirected")
    {
    strategy = vtkSmartPointer<vtkForceDirectedLayoutStrategy>::New();
    }
  else if (str == "simple2d")
    {
    strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
    }
  else if (str == "clustering2d")
    {
    strategy = vtkSmartPointer<vtkClustering2DLayoutStrategy>::New();
    }
  else if (str == "community2d")
    {
    strategy = vtkSmartPointer<vtkCommunity2DLayoutStrategy>::New();
    }
  else if (str == "fast2d")
    {
    strategy = vtkSmartPointer<vtkFast2DLayoutStrategy>::New();
    }
  else if (str == "circular")
    {
    strategy = vtkSmartPointer<vtkCircularLayoutStrategy>::New();
    }
  else if (str == "tree")
    {
    strategy = vtkSmartPointer<vtkTreeLayoutStrategy>::New();
    }
  else if (str == "cosmictree")
    {
    strategy = vtkSmartPointer<vtkCosmicTreeLayoutStrategy>::New();
    }
  else if (str == "cone")
    {
    strategy = vtkSmartPointer<vtkConeLayoutStrategy>::New();
    }
  else if (str != "passthrough")
    {
    vtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
    }
  vtkstd::string type1 = strategy->GetClassName();
  vtkstd::string type2 = this->GetLayoutStrategy()->GetClassName();
  if (type1 != type2)
    {
    this->SetLayoutStrategy(strategy);
    }
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToAssignCoordinates(
  const char* xarr,
  const char* yarr,
  const char* zarr)
{
  vtkAssignCoordinatesLayoutStrategy* s =
    vtkAssignCoordinatesLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkAssignCoordinatesLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetXCoordArrayName(xarr);
  s->SetYCoordArrayName(yarr);
  s->SetZCoordArrayName(zarr);
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToTree(
  bool radial,
  double angle,
  double leafSpacing,
  double logSpacing)
{
  vtkTreeLayoutStrategy* s =
    vtkTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetRadial(radial);
  s->SetAngle(angle);
  s->SetLeafSpacing(leafSpacing);
  s->SetLogSpacingValue(logSpacing);
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToCosmicTree(
  const char* nodeSizeArrayName,
  bool sizeLeafNodesOnly,
  int layoutDepth,
  vtkIdType layoutRoot)
{
  vtkCosmicTreeLayoutStrategy* s =
    vtkCosmicTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkCosmicTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetNodeSizeArrayName(nodeSizeArrayName);
  s->SetSizeLeafNodesOnly(sizeLeafNodesOnly);
  s->SetLayoutDepth(layoutDepth);
  s->SetLayoutRoot(layoutRoot);
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* s)
{
  if (!s)
    {
    vtkErrorMacro("Layout strategy must not be NULL.");
    return;
    }
  if (vtkArcParallelEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Arc Parallel");
    }
  else if (vtkGeoEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Geo");
    }
  else if (vtkPassThroughEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Pass Through");
    }
  else
    {
    this->SetEdgeLayoutStrategyName("Unknown");
    }
  this->EdgeLayout->SetLayoutStrategy(s);
}

vtkEdgeLayoutStrategy* vtkRenderedGraphRepresentation::GetEdgeLayoutStrategy()
{
  return this->EdgeLayout->GetLayoutStrategy();
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(const char* name)
{
  vtkstd::string str = name;
  vtkstd::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(vtkstd::remove(str.begin(), str.end(), ' '), str.end());
  vtkSmartPointer<vtkEdgeLayoutStrategy> strategy =
    vtkSmartPointer<vtkPassThroughEdgeStrategy>::New();
  if (str == "arcparallel")
    {
    strategy = vtkSmartPointer<vtkArcParallelEdgeStrategy>::New();
    }
  else if (str == "geo")
    {
    strategy = vtkSmartPointer<vtkGeoEdgeStrategy>::New();
    }
  else if (str != "passthrough")
    {
    vtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
    }
  vtkstd::string type1 = strategy->GetClassName();
  vtkstd::string type2 = this->GetEdgeLayoutStrategy()->GetClassName();
  if (type1 != type2)
    {
    this->SetEdgeLayoutStrategy(strategy);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategyToGeo(double explodeFactor)
{
  vtkGeoEdgeStrategy* s =
    vtkGeoEdgeStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkGeoEdgeStrategy::New();
    this->SetEdgeLayoutStrategy(s);
    s->Delete();
    }
  s->SetExplodeFactor(explodeFactor);
}

bool vtkRenderedGraphRepresentation::AddToView(vtkView* view)
{
  this->Superclass::AddToView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    this->VertexGlyph->SetRenderer(rv->GetRenderer());
    this->OutlineGlyph->SetRenderer(rv->GetRenderer());
    rv->GetRenderer()->AddActor(this->OutlineActor);
    rv->GetRenderer()->AddActor(this->VertexActor);
    rv->GetRenderer()->AddActor(this->EdgeActor);
    rv->GetRenderer()->AddActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->AddActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->AddLabels(this->VertexLabelPriority->GetOutputPort(), this->VertexTextProperty);
    rv->AddLabels(this->EdgeLabelPriority->GetOutputPort(), this->EdgeTextProperty);
    rv->AddIcons(this->VertexIconPriority->GetOutputPort());
    rv->AddIcons(this->EdgeIconPriority->GetOutputPort());
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeCenters);
    rv->RegisterProgress(this->GraphToPoints);
    rv->RegisterProgress(this->VertexLabels);
    rv->RegisterProgress(this->EdgeLabels);
    rv->RegisterProgress(this->VertexIcons);
    rv->RegisterProgress(this->EdgeIcons);
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeLayout);
    rv->RegisterProgress(this->GraphToPoly);
    rv->RegisterProgress(this->EdgeMapper);
    rv->RegisterProgress(this->VertexGlyph);
    rv->RegisterProgress(this->VertexMapper);
    rv->RegisterProgress(this->OutlineGlyph);
    rv->RegisterProgress(this->OutlineMapper);
    return true;
    }
  return false;
}

bool vtkRenderedGraphRepresentation::RemoveFromView(vtkView* view)
{
  this->Superclass::RemoveFromView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    this->VertexGlyph->SetRenderer(0);
    this->OutlineGlyph->SetRenderer(0);
    rv->GetRenderer()->RemoveActor(this->VertexActor);
    rv->GetRenderer()->RemoveActor(this->OutlineActor);
    rv->GetRenderer()->RemoveActor(this->EdgeActor);
    rv->GetRenderer()->RemoveActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->RemoveActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->RemoveLabels(this->VertexLabels->GetOutputPort());
    rv->RemoveLabels(this->EdgeLabels->GetOutputPort());
    rv->RemoveIcons(this->VertexIcons->GetOutputPort());
    rv->RemoveIcons(this->EdgeIcons->GetOutputPort());
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeCenters);
    rv->UnRegisterProgress(this->GraphToPoints);
    rv->UnRegisterProgress(this->VertexLabels);
    rv->UnRegisterProgress(this->EdgeLabels);
    rv->UnRegisterProgress(this->VertexIcons);
    rv->UnRegisterProgress(this->EdgeIcons);
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeLayout);
    rv->UnRegisterProgress(this->GraphToPoly);
    rv->UnRegisterProgress(this->EdgeMapper);
    rv->UnRegisterProgress(this->VertexGlyph);
    rv->UnRegisterProgress(this->VertexMapper);
    rv->UnRegisterProgress(this->OutlineGlyph);
    rv->UnRegisterProgress(this->OutlineMapper);
    return true;
    }
  return false;
}

void vtkRenderedGraphRepresentation::PrepareForRendering(vtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);

  // Make sure the transform is synchronized between rep and view
  this->Layout->SetTransform(view->GetTransform());
}

vtkSelection* vtkRenderedGraphRepresentation::ConvertSelection(
  vtkView* view, vtkSelection* sel)
{
  //cout << "Initial selection:" << endl;
  //sel->Dump();
  vtkSmartPointer<vtkSelectionNode> vertexNode =
    vtkSmartPointer<vtkSelectionNode>::New();
  vtkSmartPointer<vtkSelectionNode> edgeNode =
    vtkSmartPointer<vtkSelectionNode>::New();
  bool foundEdgeNode = false;

  if (sel->GetNumberOfNodes() > 0)
    {
    for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
      {
      vtkSelectionNode* node = sel->GetNode(i);
      vtkProp* prop = vtkProp::SafeDownCast(
        node->GetProperties()->Get(vtkSelectionNode::PROP()));
      if (node->GetContentType() == vtkSelectionNode::FRUSTUM)
        {
        vertexNode->ShallowCopy(node);
        edgeNode->ShallowCopy(node);
        }
      else if (prop == this->VertexActor.GetPointer())
        {
        vertexNode->ShallowCopy(node);
        }
      else if (prop == this->EdgeActor.GetPointer())
        {
        edgeNode->ShallowCopy(node);
        foundEdgeNode = true;
        }
      }
    }
  // Remove the prop to avoid reference loops
  vertexNode->GetProperties()->Remove(vtkSelectionNode::PROP());
  edgeNode->GetProperties()->Remove(vtkSelectionNode::PROP());

  vtkSelection* converted = vtkSelection::New();
  vtkDataObject* input = this->GetInput();
  if (input)
    {
    bool selectedVerticesFound = false;
    if (vertexNode)
      {
      vtkSmartPointer<vtkSelection> vertexSel =
        vtkSmartPointer<vtkSelection>::New();
      vertexSel->AddNode(vertexNode);
      vtkPolyData* poly = vtkPolyData::SafeDownCast(
        this->VertexGlyph->GetOutput());
      vtkSmartPointer<vtkTable> temp =
        vtkSmartPointer<vtkTable>::New();
      temp->SetRowData(vtkPolyData::SafeDownCast(poly)->GetCellData());
      vtkSelection* polyConverted = 0;
      if (poly->GetCellData()->GetPedigreeIds())
        {
        polyConverted = vtkConvertSelection::ToSelectionType(
          vertexSel, poly, vtkSelectionNode::PEDIGREEIDS);
        }
      else
        {
        polyConverted = vtkConvertSelection::ToSelectionType(
          vertexSel, poly, vtkSelectionNode::INDICES);
        }
      for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
        {
        polyConverted->GetNode(i)->SetFieldType(vtkSelectionNode::VERTEX);
        }
      vtkSelection* vertexConverted = vtkConvertSelection::ToSelectionType(
        polyConverted, input, view->GetSelectionType());
      for (unsigned int i = 0; i < vertexConverted->GetNumberOfNodes(); ++i)
        {
        if (vertexConverted->GetNode(i)->GetSelectionList()->
            GetNumberOfTuples() > 0)
          {
          selectedVerticesFound = true;
          }
        converted->AddNode(vertexConverted->GetNode(i));
        }
      polyConverted->Delete();
      vertexConverted->Delete();
      }
    if (foundEdgeNode && !selectedVerticesFound)
      {
      vtkSmartPointer<vtkSelection> edgeSel =
        vtkSmartPointer<vtkSelection>::New();
      edgeSel->AddNode(edgeNode);
      vtkPolyData* poly = vtkPolyData::SafeDownCast(
        this->GraphToPoly->GetOutput());
      vtkSelection* polyConverted = 0;
      if (poly->GetCellData()->GetPedigreeIds())
        {
        polyConverted = vtkConvertSelection::ToSelectionType(
          edgeSel, poly, vtkSelectionNode::PEDIGREEIDS);
        }
      else
        {
        polyConverted = vtkConvertSelection::ToSelectionType(
          edgeSel, poly, vtkSelectionNode::INDICES);
        }
      for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
        {
        polyConverted->GetNode(i)->SetFieldType(vtkSelectionNode::EDGE);
        }
      vtkSelection* edgeConverted = vtkConvertSelection::ToSelectionType(
        polyConverted, input, view->GetSelectionType());
      for (unsigned int i = 0; i < edgeConverted->GetNumberOfNodes(); ++i)
        {
        converted->AddNode(edgeConverted->GetNode(i));
        }
      polyConverted->Delete();
      edgeConverted->Delete();
      }
    }
  //cout << "Converted selection:" << endl;
  //converted->Dump();
  return converted;
}

void vtkRenderedGraphRepresentation::SetupInputConnections()
{
  this->Superclass::SetupInputConnections();

  this->Layout->SetInput(this->GetInput());
  this->ApplyColors->SetInputConnection(1, this->GetAnnotationConnection());
  this->ApplyColors->SetInputConnection(2, this->GetSelectionConnection());
}

void vtkRenderedGraphRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  vtkLookupTable* plutOld = vtkLookupTable::SafeDownCast(
    this->ApplyColors->GetPointLookupTable());
  if (!theme->LookupMatchesPointTheme(plutOld))
    {
    vtkSmartPointer<vtkLookupTable> plut =
      vtkSmartPointer<vtkLookupTable>::New();
    plut->SetHueRange(theme->GetPointHueRange());
    plut->SetSaturationRange(theme->GetPointSaturationRange());
    plut->SetValueRange(theme->GetPointValueRange());
    plut->SetAlphaRange(theme->GetPointAlphaRange());
    plut->Build();
    this->ApplyColors->SetPointLookupTable(plut);
    }

  vtkLookupTable* clutOld = vtkLookupTable::SafeDownCast(
    this->ApplyColors->GetCellLookupTable());
  if (!theme->LookupMatchesCellTheme(clutOld))
    {
    vtkSmartPointer<vtkLookupTable> clut =
      vtkSmartPointer<vtkLookupTable>::New();
    clut->SetHueRange(theme->GetCellHueRange());
    clut->SetSaturationRange(theme->GetCellSaturationRange());
    clut->SetValueRange(theme->GetCellValueRange());
    clut->SetAlphaRange(theme->GetCellAlphaRange());
    clut->Build();
    this->ApplyColors->SetCellLookupTable(clut);
    }

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());

  int baseSize = theme->GetPointSize();
  int lineWidth = theme->GetLineWidth();
  this->VertexGlyph->SetScreenSize(baseSize);
  this->VertexActor->GetProperty()->SetPointSize(baseSize);
  this->OutlineGlyph->SetScreenSize(baseSize + 2);
  this->OutlineActor->GetProperty()->SetPointSize(baseSize + 2*lineWidth);
  this->OutlineActor->GetProperty()->SetLineWidth(lineWidth);
  this->EdgeActor->GetProperty()->SetLineWidth(lineWidth);

  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());

  this->VertexTextProperty->SetColor(theme->GetVertexLabelColor());
  this->VertexTextProperty->SetLineOffset(-2*baseSize);
  this->EdgeTextProperty->SetColor(theme->GetEdgeLabelColor());
}

void vtkRenderedGraphRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
