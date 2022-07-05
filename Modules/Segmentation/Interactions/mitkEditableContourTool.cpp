/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkEditableContourTool.h"
#include <mitkToolManager.h>


mitk::EditableContourTool::EditableContourTool() : FeedbackContourTool("EditableContourTool") {}

mitk::EditableContourTool::~EditableContourTool()
{
  this->ReleaseHelperObjects();
  this->ReleaseInteractors();
}


void mitk::EditableContourTool::ConnectActionsAndFunctions()
{
  CONNECT_FUNCTION("InitObject", OnInitContour);
  CONNECT_FUNCTION("AddPoint", OnAddPoint);
  CONNECT_FUNCTION("CtrlAddPoint", OnAddPoint);
  CONNECT_FUNCTION("Drawing", OnDrawing);
  CONNECT_FUNCTION("EndDrawing", OnEndDrawing);
  CONNECT_FUNCTION("FinishContour", OnFinish);
  CONNECT_FUNCTION("CtrlMovePoint", OnMouseMoved);
}

void mitk::EditableContourTool::Activated()
{
  Superclass::Activated();
  this->ResetToStartState();
  this->EnableContourInteraction(true);
}

void mitk::EditableContourTool::Deactivated()
{
  this->ConfirmSegmentation();
  Superclass::Deactivated();
}

void mitk::EditableContourTool::ConfirmSegmentation(bool resetStatMachine)
{
  auto referenceImage = this->GetReferenceData();
  auto workingImage = this->GetWorkingData();

  if (nullptr != referenceImage && nullptr != workingImage)
  {
    std::vector<SliceInformation> sliceInfos;

    const auto currentTimePoint =
      mitk::RenderingManager::GetInstance()->GetTimeNavigationController()->GetSelectedTimePoint();
    TimeStepType workingImageTimeStep = workingImage->GetTimeGeometry()->TimePointToTimeStep(currentTimePoint);

    if (m_Contour.IsNull() || m_Contour->IsEmpty())
      return;

    auto workingSlice =
      this->GetAffectedImageSliceAs2DImage(m_PlaneGeometry, workingImage, workingImageTimeStep)->Clone();
    sliceInfos.emplace_back(workingSlice, m_PlaneGeometry, workingImageTimeStep);

    auto projectedContour = ContourModelUtils::ProjectContourTo2DSlice(workingSlice, m_Contour);
    int activePixelValue = ContourModelUtils::GetActivePixelValue(workingImage);

    ContourModelUtils::FillContourInSlice(projectedContour, workingSlice, workingImage, activePixelValue);

    this->WriteBackSegmentationResults(sliceInfos);
  }

  this->ReleaseHelperObjects();
  this->ReleaseInteractors();
  if (resetStatMachine) this->ResetToStartState();
}

void mitk::EditableContourTool::ClearSegmentation() 
{
  this->ReleaseHelperObjects();
  this->ReleaseInteractors();
  this->ResetToStartState();
}

bool mitk::EditableContourTool::IsPositionEventInsideImageRegion(mitk::InteractionPositionEvent *positionEvent,
                                                            mitk::BaseData *data)
{
  bool isPositionEventInsideImageRegion =
    nullptr != data && data->GetGeometry()->IsInside(positionEvent->GetPositionInWorld());

  if (!isPositionEventInsideImageRegion)
    MITK_WARN("EditableContourTool") << "PositionEvent is outside ImageRegion!";

  return isPositionEventInsideImageRegion;
}

mitk::Point3D mitk::EditableContourTool::PrepareInitContour(const Point3D& clickedPoint)
{ //default implementation does nothing
  return clickedPoint;
}

void mitk::EditableContourTool::OnInitContour(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);
  if (nullptr == positionEvent)
    return;

  auto workingDataNode = this->GetWorkingDataNode();

  if (m_Contour.IsNotNull())
  {
    this->ConfirmSegmentation(false);
  }

  if (!IsPositionEventInsideImageRegion(positionEvent, workingDataNode->GetData()))
  {
    this->ResetToStartState();
    return;
  }

  m_LastEventSender = positionEvent->GetSender();
  m_LastEventSlice = m_LastEventSender->GetSlice();

  m_Contour = this->CreateNewContour();
  m_ContourNode = mitk::DataNode::New();
  m_ContourNode->SetData(m_Contour);
  m_ContourNode->SetName("working contour node");
  m_ContourNode->SetProperty("layer", IntProperty::New(100));
  m_ContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_ContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_ContourNode->AddProperty("contour.color", ColorProperty::New(1.0f, 1.0f, 0.0f), nullptr, true);
  m_ContourNode->AddProperty("contour.points.color", ColorProperty::New(1.0f, 0.0f, 0.1f), nullptr, true);
  m_ContourNode->AddProperty("contour.controlpoints.show", BoolProperty::New(true), nullptr, true);

  m_PreviewContour = this->CreateNewContour();
  m_PreviewContourNode = mitk::DataNode::New();
  m_PreviewContourNode->SetData(m_PreviewContour);
  m_PreviewContourNode->SetName("active preview node");
  m_PreviewContourNode->SetProperty("layer", IntProperty::New(101));
  m_PreviewContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_PreviewContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_PreviewContourNode->AddProperty("contour.color", ColorProperty::New(0.1f, 1.0f, 0.1f), nullptr, true);
  m_PreviewContourNode->AddProperty("contour.width", mitk::FloatProperty::New(4.0f), nullptr, true);


  m_ClosureContour = this->CreateNewContour();
  m_ClosureContourNode = mitk::DataNode::New();
  m_ClosureContourNode->SetData(m_ClosureContour);
  m_ClosureContourNode->SetName("active closure node");
  m_ClosureContourNode->SetProperty("layer", IntProperty::New(101));
  m_ClosureContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_ClosureContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_ClosureContourNode->AddProperty("contour.color", ColorProperty::New(0.0f, 1.0f, 0.1f), nullptr, true);
  m_ClosureContourNode->AddProperty("contour.width", mitk::FloatProperty::New(2.0f), nullptr, true);

  m_CurrentRestrictedArea = this->CreateNewContour();

  auto dataStorage = this->GetToolManager()->GetDataStorage();
  dataStorage->Add(m_ContourNode, workingDataNode);
  dataStorage->Add(m_PreviewContourNode, workingDataNode);
  dataStorage->Add(m_ClosureContourNode, workingDataNode);

  m_ReferenceDataSlice = this->GetAffectedReferenceSlice(positionEvent);

  auto origin = m_ReferenceDataSlice->GetSlicedGeometry()->GetOrigin();
  m_ReferenceDataSlice->GetSlicedGeometry()->WorldToIndex(origin, origin);
  m_ReferenceDataSlice->GetSlicedGeometry()->IndexToWorld(origin, origin);
  m_ReferenceDataSlice->GetSlicedGeometry()->SetOrigin(origin);

  // Remember PlaneGeometry to determine if events were triggered in the same plane
  m_PlaneGeometry = interactionEvent->GetSender()->GetCurrentWorldPlaneGeometry();

  // Map click to pixel coordinates
  auto click = positionEvent->GetPositionInWorld();

  click = this->PrepareInitContour(click);

  this->InitializePreviewContour(click);
  // Set initial start point
  m_Contour->AddVertex(click, true);
  m_PreviewContour->AddVertex(click, false);
  m_ClosureContour->AddVertex(click);

  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::EditableContourTool::FinalizePreviewContour(const Point3D& clickedPoint)
{ // Remove duplicate first vertex, it's already contained in m_Contour
  m_PreviewContour->RemoveVertexAt(0);

  m_PreviewContour->SetControlVertexAt(m_PreviewContour->GetNumberOfVertices() - 1);
}

void mitk::EditableContourTool::InitializePreviewContour(const Point3D& clickedPoint)
{ //default implementation only clears the preview and sets the start point
  m_PreviewContour = this->CreateNewContour();
  m_PreviewContour->AddVertex(clickedPoint);
  m_PreviewContourNode->SetData(m_PreviewContour);
}

void mitk::EditableContourTool::UpdatePreviewContour(const Point3D& clickedPoint)
{ //default implementation draws just a simple line to position
  if (m_PreviewContour->GetNumberOfVertices() > 2)
  {
    this->InitializePreviewContour(m_Contour->GetVertexAt(m_Contour->GetNumberOfVertices() - 1)->Coordinates);
  }

  if (m_PreviewContour->GetNumberOfVertices() == 2)
  {
    m_PreviewContour->RemoveVertexAt(m_PreviewContour->GetNumberOfVertices()-1);
  }

  m_PreviewContour->AddVertex(clickedPoint);
}

void mitk::EditableContourTool::OnAddPoint(StateMachineAction*, InteractionEvent* interactionEvent)
{
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  this->FinalizePreviewContour(positionEvent->GetPositionInWorld());

  // Merge contours
  m_Contour->Concatenate(m_PreviewContour);

  this->InitializePreviewContour(positionEvent->GetPositionInWorld());

  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::EditableContourTool::OnDrawing(StateMachineAction*, InteractionEvent* interactionEvent)
{
  auto* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
  if (!positionEvent)
    return;

  m_PreviewContourNode->SetVisibility(false);

  m_Contour->AddVertex(positionEvent->GetPositionInWorld(), false);
  UpdateClosureContour(positionEvent->GetPositionInWorld());
  m_CurrentRestrictedArea->AddVertex(positionEvent->GetPositionInWorld());

  assert(positionEvent->GetSender()->GetRenderWindow());
  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::EditableContourTool::OnEndDrawing(StateMachineAction*, InteractionEvent* interactionEvent)
{
  auto* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
  if (!positionEvent)
    return;

  if (m_CurrentRestrictedArea->GetNumberOfVertices() > 1)
  {
    auto restrictedArea = m_CurrentRestrictedArea->Clone();
    m_RestrictedAreas.push_back(restrictedArea);
  }
  m_CurrentRestrictedArea = this->CreateNewContour();
  m_PreviewContourNode->SetVisibility(true);
  m_Contour->SetControlVertexAt(m_Contour->GetNumberOfVertices() - 1);

  this->InitializePreviewContour(positionEvent->GetPositionInWorld());

  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::EditableContourTool::OnMouseMoved(StateMachineAction*, InteractionEvent* interactionEvent)
{
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  this->UpdatePreviewContour(positionEvent->GetPositionInWorld());
  this->UpdateClosureContour(positionEvent->GetPositionInWorld());

  RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::EditableContourTool::OnFinish(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  this->FinalizePreviewContour(positionEvent->GetPositionInWorld());

  this->FinishTool();

  // Merge contours
  m_Contour->Concatenate(m_PreviewContour);

  auto numberOfTimesteps = static_cast<int>(m_Contour->GetTimeSteps());

  for (int i = 0; i <= numberOfTimesteps; ++i)
    m_Contour->Close(i);

  this->ReleaseHelperObjects(false);
}

void mitk::EditableContourTool::ReleaseHelperObjects(bool includeWorkingContour)
{
  this->RemoveHelperObjectsFromDataStorage(includeWorkingContour);

  if (includeWorkingContour)
  {
    m_ContourNode = nullptr;
    m_Contour = nullptr;

    m_CurrentRestrictedArea = nullptr;
    m_RestrictedAreas.clear();
  }

  m_PreviewContourNode = nullptr;
  m_PreviewContour = nullptr;

  m_ClosureContourNode = nullptr;
  m_ClosureContour = nullptr;
}

void mitk::EditableContourTool::RemoveHelperObjectsFromDataStorage(bool includeWorkingContour)
{
  auto dataStorage = this->GetToolManager()->GetDataStorage();

  if (nullptr == dataStorage)
    return;

  if (includeWorkingContour)
  {
    if (m_ContourNode.IsNotNull())
      dataStorage->Remove(m_ContourNode);
  }

  if (m_PreviewContourNode.IsNotNull())
    dataStorage->Remove(m_PreviewContourNode);

  if (m_ClosureContourNode.IsNotNull())
    dataStorage->Remove(m_ClosureContourNode);

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

mitk::ContourModel::Pointer mitk::EditableContourTool::CreateNewContour() const
{
  auto workingData = this->GetWorkingData();
  if (nullptr == workingData)
  {
    this->InteractiveSegmentationBugMessage(
      "Cannot create new contour. No valid working data is set. Application is in invalid state.");
    mitkThrow() << "Cannot create new contour. No valid working data is set. Application is in invalid state.";
  }

  auto contour = ContourModel::New();

  // generate a time geometry that is always visible as the working contour should always be.
  auto contourTimeGeometry = ProportionalTimeGeometry::New();
  contourTimeGeometry->SetStepDuration(std::numeric_limits<TimePointType>::max());
  contourTimeGeometry->SetTimeStepGeometry(contour->GetTimeGeometry()->GetGeometryForTimeStep(0)->Clone(), 0);
  contour->SetTimeGeometry(contourTimeGeometry);

  return contour;
}

void mitk::EditableContourTool::UpdateClosureContour(mitk::Point3D endpoint) 
{
  if (m_ClosureContour->GetNumberOfVertices() > 2)
  {
    m_ClosureContour = this->CreateNewContour();
    m_ClosureContourNode->SetData(m_ClosureContour);
  }

  if (m_ClosureContour->GetNumberOfVertices() == 0)
  {
    m_ClosureContour->AddVertex(m_Contour->GetVertexAt(0)->Coordinates);
    m_ClosureContour->Update();
  }

  if (m_ClosureContour->GetNumberOfVertices() == 2)
  {
    m_ClosureContour->RemoveVertexAt(0);
  }

  m_ClosureContour->AddVertexAtFront(endpoint);
}

void mitk::EditableContourTool::EnableContourInteraction(bool on)
{
  for (const auto& interactor : m_ContourInteractors)
    interactor->EnableInteraction(on);
}

void mitk::EditableContourTool::ReleaseInteractors()
{
  this->EnableContourInteraction(false);
  m_ContourInteractors.clear();
}
