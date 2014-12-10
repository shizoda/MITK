/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D
#define IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D

#include <MitkCoreExports.h>
#include "mitkSubImageSelector.h"

namespace mitk {

//##Documentation
//## @brief Provides access to a volume at a specific time of the input image
//##
//## If the input is generated by a ProcessObject, only the required data is
//## requested.
//## @ingroup Process
class MITKCORE_EXPORT ImageTimeSelector : public SubImageSelector
{
public:
  mitkClassMacro(ImageTimeSelector,SubImageSelector);

  itkFactorylessNewMacro(Self)
  itkCloneMacro(Self)

  itkGetConstMacro(TimeNr,int);
  itkSetMacro(TimeNr,int);

  itkGetConstMacro(ChannelNr,int);
  itkSetMacro(ChannelNr,int);

protected:
  ImageTimeSelector();

  virtual ~ImageTimeSelector();

  virtual void GenerateOutputInformation();

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateData();

  int m_TimeNr;

  int m_ChannelNr;
};

} // namespace mitk



#endif /* IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D */
