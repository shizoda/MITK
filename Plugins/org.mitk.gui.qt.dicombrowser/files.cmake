set(CACHED_RESOURCE_FILES
  resources/dicom.svg
  plugin.xml
)

set(QRC_FILES
  resources/dicom.qrc
)

set(CPP_FILES
  mitkPluginActivator.cpp
  QmitkDicomBrowser.cpp
  QmitkDicomDataEventPublisher.cpp
  QmitkDicomDirectoryListener.cpp
  QmitkDicomEventHandler.cpp
  QmitkDicomPreferencePage.cpp
  QmitkStoreSCPLauncher.cpp
)

set(MOC_H_FILES
  mitkPluginActivator.h
  QmitkDicomDataEventPublisher.h
  QmitkDicomBrowser.h
  QmitkDicomDirectoryListener.h
  QmitkDicomEventHandler.h
  QmitkDicomPreferencePage.h
  QmitkStoreSCPLauncher.h
)

set(UI_FILES
  QmitkDicomBrowserControls.ui
)

set(src_internal_path "src/internal/")
list(TRANSFORM CPP_FILES PREPEND ${src_internal_path})
list(TRANSFORM MOC_H_FILES PREPEND ${src_internal_path})
list(TRANSFORM UI_FILES PREPEND ${src_internal_path})
