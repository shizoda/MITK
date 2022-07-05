/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkEditbaleContourTool_h_Included
#define mitkEditbaleContourTool_h_Included

#include <mitkFeedbackContourTool.h>

namespace mitk
{

  class MITKSEGMENTATION_EXPORT EditableContourTool : public FeedbackContourTool
  {
  public:
    mitkClassMacro(EditableContourTool, FeedbackContourTool);

    /// \brief Convert current contour to binary segmentations.
    virtual void ConfirmSegmentation(bool resetStatMachine = true);

    /// \brief Delete all current contours.
    virtual void ClearSegmentation();

  protected:
    EditableContourTool();
    ~EditableContourTool() override;

    void ConnectActionsAndFunctions() override;

    void Activated() override;
    void Deactivated() override;
   
    virtual Point3D PrepareInitContour(const Point3D& clickedPoint);
    virtual void FinalizePreviewContour(const Point3D& clickedPoint);
    virtual void InitializePreviewContour(const Point3D& clickedPoint);
    virtual void UpdatePreviewContour(const Point3D& clickedPoint);

    /// \brief Initialize tool.
    virtual void OnInitContour(StateMachineAction *, InteractionEvent *interactionEvent);

    /// \brief Add a control point and finish current segment.
    virtual void OnAddPoint(StateMachineAction *, InteractionEvent *interactionEvent);

    /// \brief Draw a contour according to the mouse movement when mouse button is pressed and mouse is moved.
    virtual void OnDrawing(StateMachineAction *, InteractionEvent *interactionEvent);

    virtual void OnEndDrawing(StateMachineAction *, InteractionEvent *interactionEvent);

    /// \brief Computation of the preview contour.
    virtual void OnMouseMoved(StateMachineAction *, InteractionEvent *interactionEvent);

    /// \brief Finish EditableContour tool.
    virtual void OnFinish(StateMachineAction *, InteractionEvent *interactionEvent);

    /// \brief Finish contour interaction.
    virtual void FinishTool() = 0;

    void EnableContourInteraction(bool on);

    void ReleaseInteractors();

    virtual void ReleaseHelperObjects(bool includeWorkingContour = true);

    virtual void RemoveHelperObjectsFromDataStorage(bool includeWorkingContour = true);

    ContourModel::Pointer CreateNewContour() const;

    virtual void UpdateClosureContour(mitk::Point3D endpoint);

    bool IsPositionEventInsideImageRegion(InteractionPositionEvent *positionEvent, BaseData *data);

    mitk::ContourModel::Pointer m_Contour;
    mitk::DataNode::Pointer m_ContourNode;

    mitk::ContourModel::Pointer m_PreviewContour;
    mitk::DataNode::Pointer m_PreviewContourNode;

    mitk::ContourModel::Pointer m_ClosureContour;
    mitk::DataNode::Pointer m_ClosureContourNode;

    mitk::ContourModel::Pointer m_CurrentRestrictedArea;
    std::vector<mitk::ContourModel::Pointer> m_RestrictedAreas;

    /** Slice of the reference data the tool is currently actively working on to
    define contours.*/
    mitk::Image::Pointer m_ReferenceDataSlice;

    PlaneGeometry::ConstPointer m_PlaneGeometry;

    std::vector<mitk::DataInteractor::Pointer> m_ContourInteractors;
  };
}

#endif
