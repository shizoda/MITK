/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QMITKDATANODERESETGEOMETRYACTION_H
#define QMITKDATANODERESETGEOMETRYACTION_H

#include <org_mitk_gui_qt_application_Export.h>

#include <QmitkAbstractDataNodeAction.h>

// qt
#include <QAction>

namespace ResetGeometryAction
{
  MITK_QT_APP void Run(berry::IWorkbenchPartSite::Pointer workbenchPartSite,
                       const QList<mitk::DataNode::Pointer>& selectedNodes = QList<mitk::DataNode::Pointer>(),
                       mitk::BaseRenderer* baseRenderer = nullptr);
}

class MITK_QT_APP QmitkDataNodeResetGeometryAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeResetGeometryAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeResetGeometryAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite);

private Q_SLOTS:

  void OnActionTriggered(bool);

protected:

  void InitializeAction() override;

};

#endif // QMITKDATANODEREINITVIEWACTION_H
