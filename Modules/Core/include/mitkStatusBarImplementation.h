/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef MITKSTATUSBARIMPLEMENTATION_H
#define MITKSTATUSBARIMPLEMENTATION_H
#include <MitkCoreExports.h>
#include <mitkCommon.h>

namespace mitk
{
  //##Documentation
  //## @brief GUI independent Interface for all Gui depentent implementations of a StatusBar.
  class MITKCORE_EXPORT StatusBarImplementation
  {
  public:
    mitkClassMacroNoParent(StatusBarImplementation)

      //##Documentation
      //## @brief Constructor
      StatusBarImplementation(){};
    //##Documentation
    //## @brief Destructor
    virtual ~StatusBarImplementation(){};

    //##Documentation
    //## @brief Send a string to the applications StatusBar
    virtual void DisplayText(const char *t) = 0;

    //##Documentation
    //## @brief Send a string with a time delay to the applications StatusBar
    virtual void DisplayText(const char *t, int ms) = 0;
    virtual void DisplayErrorText(const char *t) = 0;
    virtual void DisplayWarningText(const char *t) = 0;
    virtual void DisplayWarningText(const char *t, int ms) = 0;
    virtual void DisplayGenericOutputText(const char *t) = 0;
    virtual void DisplayDebugText(const char *t) = 0;
    virtual void DisplayGreyValueText(const char *t) = 0;

    //##Documentation
    //## @brief removes any temporary message being shown.
    virtual void Clear() = 0;

    //##Documentation
    //## @brief Set the SizeGrip of the window
    //## (the triangle in the lower right Windowcorner for changing the size)
    //## to enabled or disabled
    virtual void SetSizeGripEnabled(bool enable) = 0;
  };

} // end namespace mitk
#endif /* define MITKSTATUSBARIMPLEMENTATION_H */
