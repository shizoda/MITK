/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkStoreSCPLauncherBuilder_h
#define mitkStoreSCPLauncherBuilder_h

#include <QString>

namespace mitk
{
  /**
  * \brief Build arguments for QmitkStoreSCPLauncher.
  */
  struct StoreSCPLauncherBuilder
  {
    QString Port = "105";
    QString AETitle = "STORESCP";
    QString TransferSyntax = "+x=";
    QString OtherNetworkOptions = "-pm";
    QString Mode = "-v";
    QString OutputDirectory;
  };
}

#endif
