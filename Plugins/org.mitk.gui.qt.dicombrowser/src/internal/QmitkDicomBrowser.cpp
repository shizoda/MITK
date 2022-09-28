/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDicomBrowser.h"
#include "mitkPluginActivator.h"

#include <berryIBerryPreferences.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryIQtPreferencePage.h>
#include <berryPlatform.h>

const std::string QmitkDicomBrowser::EDITOR_ID = "org.mitk.editors.dicombrowser";
const QString QmitkDicomBrowser::TEMP_DICOM_FOLDER_SUFFIX = "TmpDicomFolder";

QmitkDicomBrowser::QmitkDicomBrowser()
  : m_DicomDirectoryListener(new QmitkDicomDirectoryListener),
    m_StoreSCPLauncher(new QmitkStoreSCPLauncher(m_StoreSCPLauncherParams)),
    m_Publisher(new QmitkDicomDataEventPublisher())
{
}

QmitkDicomBrowser::~QmitkDicomBrowser()
{
  delete m_DicomDirectoryListener;
  delete m_StoreSCPLauncher;
  delete m_Handler;
  delete m_Publisher;
}

void QmitkDicomBrowser::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  this->InitEventHandler();
  this->OnPreferencesChanged(nullptr);
  this->CreateTemporaryDirectory();
  this->StartDicomDirectoryListener();

  m_Controls.m_ctkDICOMQueryRetrieveWidget->useProgressDialog(true);

  connect(
    m_Controls.tabWidget, &QTabWidget::currentChanged,
    this, &QmitkDicomBrowser::OnTabChanged);

  connect(
    m_Controls.externalDataWidget, &QmitkDicomExternalDataWidget::DicomToDataManager,
    this, &QmitkDicomBrowser::OnViewButtonAddToDataManager);

  connect(
    m_Controls.internalDataWidget, &QmitkDicomLocalStorageWidget::FinishedImport,
    this, &QmitkDicomBrowser::OnDicomImportFinished);

  connect(
    m_Controls.internalDataWidget, &QmitkDicomLocalStorageWidget::DicomToDataManager,
    this, &QmitkDicomBrowser::OnViewButtonAddToDataManager);

  connect(
    m_Controls.externalDataWidget, &QmitkDicomExternalDataWidget::StartDicomImport,
    m_Controls.internalDataWidget, qOverload<const QStringList&>(&QmitkDicomLocalStorageWidget::OnStartDicomImport));
}


void QmitkDicomBrowser::Init(berry::IEditorSite::Pointer site, berry::IEditorInput::Pointer input)
{
    this->SetSite(site);
    this->SetInput(input);
}

berry::IPartListener::Events::Types QmitkDicomBrowser::GetPartEventTypes() const
{
    return Events::CLOSED | Events::HIDDEN | Events::VISIBLE;
}

void QmitkDicomBrowser::OnTabChanged(int page)
{
  if (page == 2) // Query/Retrieve
  {
    auto serverParams = m_Controls.m_ctkDICOMQueryRetrieveWidget->getServerParameters();
    auto storagePort = serverParams["StoragePort"].toString();
    auto storageAET = serverParams["StorageAETitle"].toString();

    if (m_StoreSCPLauncherParams.AETitle != storageAET || m_StoreSCPLauncherParams.Port != storagePort)
    {
      this->StopStoreSCP();
      this->StartStoreSCP();
    }
  }
}

void QmitkDicomBrowser::OnDicomImportFinished()
{
  m_Controls.tabWidget->setCurrentIndex(0);
}

void QmitkDicomBrowser::StartDicomDirectoryListener()
{
  m_DicomDirectoryListener->SetDicomListenerDirectory(m_TempDirectory);
  m_DicomDirectoryListener->SetDicomFolderSuffix(TEMP_DICOM_FOLDER_SUFFIX);

  connect(
    m_DicomDirectoryListener, &QmitkDicomDirectoryListener::StartDicomImport,
    m_Controls.internalDataWidget, qOverload<const QStringList&>(&QmitkDicomLocalStorageWidget::OnStartDicomImport),
    Qt::DirectConnection);
}


void QmitkDicomBrowser::InitEventHandler()
{
  m_Handler = new QmitkDicomEventHandler();
  m_Handler->SubscribeSlots();
}

void QmitkDicomBrowser::OnViewButtonAddToDataManager(QHash<QString, QVariant> eventProperties)
{
  ctkDictionary properties;
  properties["FilesForSeries"] = eventProperties["FilesForSeries"];

  if(eventProperties.contains("Modality"))
    properties["Modality"] = eventProperties["Modality"];

  m_Publisher->PublishSignals(mitk::PluginActivator::getContext());
  m_Publisher->AddSeriesToDataManagerEvent(properties);
}

void QmitkDicomBrowser::StartStoreSCP()
{
  auto serverParams = m_Controls.m_ctkDICOMQueryRetrieveWidget->getServerParameters();
  auto storagePort = serverParams["StoragePort"].toString();
  auto storageAET = serverParams["StorageAETitle"].toString();

  QmitkStoreSCPLauncher::Params params;
  params.Port = storagePort;
  params.AETitle = storageAET;
  params.OutputDirectory = m_TempDirectory;

  m_StoreSCPLauncherParams = params;
  m_StoreSCPLauncher = new QmitkStoreSCPLauncher(m_StoreSCPLauncherParams);

  connect(
    m_StoreSCPLauncher, &QmitkStoreSCPLauncher::StoreSCPStatusChanged,
    this, &QmitkDicomBrowser::OnStoreSCPStatusChanged);

  connect(
    m_StoreSCPLauncher, &QmitkStoreSCPLauncher::StartImport,
    m_Controls.internalDataWidget, qOverload<const QStringList&>(&QmitkDicomLocalStorageWidget::OnStartDicomImport));

  connect(
    m_StoreSCPLauncher, &QmitkStoreSCPLauncher::StoreSCPError,
    m_DicomDirectoryListener, &QmitkDicomDirectoryListener::OnDicomNetworkError,
    Qt::DirectConnection);

  connect(
    m_StoreSCPLauncher, &QmitkStoreSCPLauncher::StoreSCPError,
    this, &QmitkDicomBrowser::OnDicomNetworkError,
    Qt::DirectConnection);
  
  m_StoreSCPLauncher->StartStoreSCP();
}

void QmitkDicomBrowser::OnStoreSCPStatusChanged(const QString& status)
{
  m_Controls.StoreSCPStatusLabel->setText("<img src=':/org.mitk.gui.qt.dicombrowser/network-idle_16.png'> " + status);
}

void QmitkDicomBrowser::OnDicomNetworkError(const QString& status)
{
  m_Controls.StoreSCPStatusLabel->setText("<img src=':/org.mitk.gui.qt.dicombrowser/network-error_16.png'> " + status);
}

void QmitkDicomBrowser::StopStoreSCP()
{
  delete m_StoreSCPLauncher;
}

void QmitkDicomBrowser::SetPluginDirectory()
{
   m_PluginDirectory = mitk::PluginActivator::getContext()->getDataFile("").absolutePath();
   m_PluginDirectory += "/database";
}

void QmitkDicomBrowser::CreateTemporaryDirectory()
{
  m_TempDirectory =
    QDir::tempPath() +
    "/" +
    TEMP_DICOM_FOLDER_SUFFIX +
    "." +
    QTime::currentTime().toString("hhmmsszzz") +
    QString::number(QCoreApplication::applicationPid());

  m_TempDirectory = QDir::toNativeSeparators(m_TempDirectory);

  QDir tmp;
  tmp.mkdir(m_TempDirectory);
}

void QmitkDicomBrowser::OnPreferencesChanged(const berry::IBerryPreferences*)
{
  this->SetPluginDirectory();

  auto* prefService = berry::Platform::GetPreferencesService();
  m_DatabaseDirectory = prefService->GetSystemPreferences()->Node("/org.mitk.views.dicomreader")->Get("default dicom path", m_PluginDirectory);
  m_Controls.internalDataWidget->SetDatabaseDirectory(m_DatabaseDirectory);
}
