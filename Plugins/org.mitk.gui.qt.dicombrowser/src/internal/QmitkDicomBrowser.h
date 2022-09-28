/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QmitkDicomBrowser_h
#define QmitkDicomBrowser_h

#include <berryIBerryPreferences.h>
#include <berryIPartListener.h>
#include <berryISelectionListener.h>
#include <berryQtEditorPart.h>

#include "QmitkDicomEventHandler.h"
#include "QmitkDicomDataEventPublisher.h"
#include "QmitkDicomDirectoryListener.h"
#include "QmitkStoreSCPLauncher.h"

#include <org_mitk_gui_qt_dicombrowser_Export.h>
#include <ui_QmitkDicomBrowserControls.h>

#include <QString>
#include <QThread>
#include <QVariant>

/**
* \brief QmitkDicomBrowser is an editor providing functionality for DICOM storage and import and query/retrieve functionality.
*
* \sa berry::IPartListener
* \ingroup ${plugin_target}_internal
*/
class DICOM_EXPORT QmitkDicomBrowser : public berry::QtEditorPart, virtual public berry::IPartListener
{
  Q_OBJECT

public:
  static const std::string EDITOR_ID;
  static const QString TEMP_DICOM_FOLDER_SUFFIX;

  berryObjectMacro(QmitkDicomBrowser)

  QmitkDicomBrowser();
  ~QmitkDicomBrowser() override;

  /**
  * \brief Initialize the editor.
  */
  void Init(berry::IEditorSite::Pointer site, berry::IEditorInput::Pointer input) override;

  void SetFocus() override {};
  void DoSave() override {}
  void DoSaveAs() override {}
  bool IsDirty() const override { return false; }
  bool IsSaveAsAllowed() const override { return false; }

  virtual void OnPreferencesChanged(const berry::IBerryPreferences* prefs);

signals:

  /**
  * \brief Emitted when a DICOM directory for import was selected.
  */
  void StartDicomImport(const QString&);

private:

  /// \brief Called when import is finished.
  void OnDicomImportFinished();

  /// \brief Called when Query/Retrieve or Import Folder was clicked.
  void OnTabChanged(int);

  /// \brief Called when view button is clicked. Sends out an event for adding the current selected file to the data storage.
  void OnViewButtonAddToDataManager(QHash<QString, QVariant> eventProperties);

  /// \brief Called when status of DICOM storage provider changes.
  void OnStoreSCPStatusChanged(const QString& status);

  /// \brief Called when dicom storage provider emits a network error.
  void OnDicomNetworkError(const QString& status);

  /// \brief Start DICOM storage provider.
  void StartStoreSCP();

  /// \brief Stop DICOM storage provider.
  void StopStoreSCP();

  /// \brief Initialize event handler.
  void InitEventHandler();

  /// \brief Create directory in which temporary DICOM objects are stored.
  void CreateTemporaryDirectory();

  /// \brief Start DICOM directory listener.
  void StartDicomDirectoryListener();

  void CreateQtPartControl(QWidget *parent) override;

  /// \brief Set plugin directory.
  void SetPluginDirectory();

  Events::Types GetPartEventTypes() const override;

  ctkFileDialog* m_ImportDialog;
  Ui::QmitkDicomBrowserControls m_Controls;
  QmitkDicomDirectoryListener* m_DicomDirectoryListener;
  QmitkStoreSCPLauncher::Params m_StoreSCPLauncherParams;
  QmitkStoreSCPLauncher* m_StoreSCPLauncher;
  QmitkDicomEventHandler* m_Handler;
  QmitkDicomDataEventPublisher* m_Publisher;
  QString m_PluginDirectory;
  QString m_TempDirectory;
  QString m_DatabaseDirectory;
};

#endif
