/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

// Qmitk
#include "QmitkRenderWindow.h"
#include "QmitkSliceNavigationListener.h"

#include "mitkIRenderWindowPart.h"

// Qt
#include <QTimer>
#include <QMessageBox>


///**********************************************
QmitkSliceNavigationListener::QmitkSliceNavigationListener() : m_renderWindowPart(nullptr),
m_PendingSliceChangedEvent(false),
m_CurrentSelectedPosition(std::numeric_limits<mitk::Point3D::ValueType>::lowest()),
m_CurrentSelectedTimePoint(std::numeric_limits<mitk::TimePointType>::lowest())
{
}

QmitkSliceNavigationListener::~QmitkSliceNavigationListener()
{
  this->RemoveAllObservers();
}

mitk::TimePointType QmitkSliceNavigationListener::GetCurrentSelectedTimePoint() const
{
  return m_CurrentSelectedTimePoint;
}

mitk::Point3D QmitkSliceNavigationListener::GetCurrentSelectedPosition() const
{
  return m_CurrentSelectedPosition;
}

void QmitkSliceNavigationListener::OnSliceChangedDelayed()
{
  m_PendingSliceChangedEvent = false;

  emit SliceChanged();

  if (nullptr != m_renderWindowPart)
  {
    const auto newSelectedPosition = m_renderWindowPart->GetSelectedPosition();
    const auto newSelectedTimePoint = m_renderWindowPart->GetSelectedTimePoint();

    if (newSelectedPosition != m_CurrentSelectedPosition)
    {
      m_CurrentSelectedPosition = newSelectedPosition;
      emit SelectedPositionChanged(newSelectedPosition);
    }

    if (newSelectedTimePoint != m_CurrentSelectedTimePoint)
    {
      m_CurrentSelectedTimePoint = newSelectedTimePoint;
      emit SelectedTimePointChanged(newSelectedTimePoint);
    }
  }
}

void QmitkSliceNavigationListener::OnSliceChangedInternal(const itk::EventObject&)
{
  // Since there are always 3 events arriving (one for each render window) every time the slice
  // or time changes, the slot OnSliceChangedDelayed is triggered - and only if it hasn't been
  // triggered yet - so it is only executed once for every slice/time change.
  if (!m_PendingSliceChangedEvent)
  {
    m_PendingSliceChangedEvent = true;

    QTimer::singleShot(0, this, SLOT(OnSliceChangedDelayed()));
  }
}

void QmitkSliceNavigationListener::OnSliceNavigationControllerDeleted(const itk::Object* sender, const itk::EventObject& /*e*/)
{
  const mitk::SliceNavigationController* sendingSlicer =
    dynamic_cast<const mitk::SliceNavigationController*>(sender);

  this->RemoveObservers(sendingSlicer);
}

void QmitkSliceNavigationListener::RenderWindowPartActivated(mitk::IRenderWindowPart* renderWindowPart)
{
  if (m_renderWindowPart != renderWindowPart)
  {
    m_renderWindowPart = renderWindowPart;

    if (!InitObservers())
    {
      QMessageBox::information(nullptr, "Error", "Unable to set up the event observers. The " \
        "plot will not be triggered on changing the crosshair, " \
        "position or time step.");
    }

    m_CurrentSelectedPosition = m_renderWindowPart->GetSelectedPosition();
    m_CurrentSelectedTimePoint = m_renderWindowPart->GetSelectedTimePoint();
  }
}

void QmitkSliceNavigationListener::RenderWindowPartDeactivated(mitk::IRenderWindowPart* renderWindowPart)
{
  m_renderWindowPart = nullptr;
  this->RemoveAllObservers(renderWindowPart);
}

bool QmitkSliceNavigationListener::InitObservers()
{
  bool result = true;

  typedef QHash<QString, QmitkRenderWindow*> WindowMapType;
  WindowMapType windowMap = m_renderWindowPart->GetQmitkRenderWindows();

  auto i = windowMap.begin();

  while (i != windowMap.end())
  {
    mitk::SliceNavigationController* sliceNavController =
      i.value()->GetSliceNavigationController();

    if (sliceNavController)
    {
      itk::ReceptorMemberCommand<QmitkSliceNavigationListener>::Pointer cmdSliceEvent =
        itk::ReceptorMemberCommand<QmitkSliceNavigationListener>::New();
      cmdSliceEvent->SetCallbackFunction(this, &QmitkSliceNavigationListener::OnSliceChangedInternal);
      int tag = sliceNavController->AddObserver(
        mitk::SliceNavigationController::GeometrySliceEvent(nullptr, 0),
        cmdSliceEvent);

      m_ObserverMap.insert(std::make_pair(sliceNavController, ObserverInfo(sliceNavController, tag,
        i.key().toStdString(), m_renderWindowPart)));

      itk::ReceptorMemberCommand<QmitkSliceNavigationListener>::Pointer cmdTimeEvent =
        itk::ReceptorMemberCommand<QmitkSliceNavigationListener>::New();
      cmdTimeEvent->SetCallbackFunction(this, &QmitkSliceNavigationListener::OnSliceChangedInternal);
      tag = sliceNavController->AddObserver(
        mitk::SliceNavigationController::GeometryTimeEvent(nullptr, 0),
        cmdTimeEvent);

      m_ObserverMap.insert(std::make_pair(sliceNavController, ObserverInfo(sliceNavController, tag,
        i.key().toStdString(), m_renderWindowPart)));

      itk::MemberCommand<QmitkSliceNavigationListener>::Pointer cmdDelEvent =
        itk::MemberCommand<QmitkSliceNavigationListener>::New();
      cmdDelEvent->SetCallbackFunction(this,
        &QmitkSliceNavigationListener::OnSliceNavigationControllerDeleted);
      tag = sliceNavController->AddObserver(
        itk::DeleteEvent(), cmdDelEvent);

      m_ObserverMap.insert(std::make_pair(sliceNavController, ObserverInfo(sliceNavController, tag,
        i.key().toStdString(), m_renderWindowPart)));
    }

    ++i;

    result = result && sliceNavController;
  }

  return result;
}

void QmitkSliceNavigationListener::RemoveObservers(const mitk::SliceNavigationController* deletedSlicer)
{
  std::pair < ObserverMapType::const_iterator, ObserverMapType::const_iterator> obsRange =
    m_ObserverMap.equal_range(deletedSlicer);

  for (ObserverMapType::const_iterator pos = obsRange.first; pos != obsRange.second; ++pos)
  {
    pos->second.controller->RemoveObserver(pos->second.observerTag);
  }

  m_ObserverMap.erase(deletedSlicer);
}

void QmitkSliceNavigationListener::RemoveAllObservers(mitk::IRenderWindowPart* deletedPart)
{
  for (ObserverMapType::const_iterator pos = m_ObserverMap.begin(); pos != m_ObserverMap.end();)
  {
    ObserverMapType::const_iterator delPos = pos++;

    if (deletedPart == nullptr || deletedPart == delPos->second.renderWindowPart)
    {
      delPos->second.controller->RemoveObserver(delPos->second.observerTag);
      m_ObserverMap.erase(delPos);
    }
  }
}

QmitkSliceNavigationListener::ObserverInfo::ObserverInfo(mitk::SliceNavigationController* controller, int observerTag,
  const std::string& renderWindowName, mitk::IRenderWindowPart* part) : controller(controller), observerTag(observerTag),
  renderWindowName(renderWindowName), renderWindowPart(part)
{
}
