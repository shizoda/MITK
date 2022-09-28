/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkStoreSCPLauncherBuilder.h"

QmitkStoreSCPLauncherBuilder::QmitkStoreSCPLauncherBuilder()
{
}

QmitkStoreSCPLauncherBuilder::~QmitkStoreSCPLauncherBuilder()
{
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddPort(const QString& port)
{
  m_Port = port;
  return this;
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddAETitle(const QString& aeTitle)
{
  m_AETitle = aeTitle;
  return this;
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddTransferSyntax(const QString& transferSyntax)
{
  m_TransferSyntax = transferSyntax;
  return this;
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddOtherNetworkOptions(const QString& otherNetworkOptions)
{
  m_OtherNetworkOptions = otherNetworkOptions;
  return this;
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddMode(const QString& mode)
{
  m_Mode = mode;
  return this;
}

QmitkStoreSCPLauncherBuilder* QmitkStoreSCPLauncherBuilder::AddOutputDirectory(const QString& outputDirectory)
{
  m_OutputDirectory = outputDirectory;
  return this;
}

QString QmitkStoreSCPLauncherBuilder::GetPort()
{
  return m_Port;
}

QString QmitkStoreSCPLauncherBuilder::GetAETitle()
{
  return m_AETitle;
}

QString QmitkStoreSCPLauncherBuilder::GetTransferSyntax()
{
  return m_TransferSyntax;
}

QString QmitkStoreSCPLauncherBuilder::GetOtherNetworkOptions()
{
  return m_OtherNetworkOptions;
}

QString QmitkStoreSCPLauncherBuilder::GetMode()
{
  return m_Mode;
}

QString QmitkStoreSCPLauncherBuilder::GetOutputDirectory()
{
  return m_OutputDirectory;
}
