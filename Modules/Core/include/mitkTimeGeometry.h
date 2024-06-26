/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef TimeGeometry_h
#define TimeGeometry_h

// ITK
#include <itkObject.h>
// MITK
#include "mitkOperationActor.h"
#include <MitkCoreExports.h>
#include <mitkBaseGeometry.h>
#include <mitkCommon.h>

namespace mitk
{
  typedef mitk::ScalarType TimePointType;
  typedef std::size_t TimeStepType;

  /**
  * \brief Manages the geometries of a data object for each time step
  *
  * This class is an abstract class. The concrete implementation
  * depends on the way the different time steps are managed.
  *
  * The time is defined either by a time step or a time point. Time steps
  * are non-negative integers starting from 0. A time point is a ScalarType value
  * which gives the passed time since start in ms. Be aware that the starting
  * point is not fixed so it is possible that the same time point  defines two
  * different time depending on the start time of the used time geometry.
  *
  * \addtogroup geometry
  */
  class MITKCORE_EXPORT TimeGeometry : public itk::Object, public OperationActor
  {
  protected:
    TimeGeometry();
    ~TimeGeometry() override;

    /**
    * \brief Contains a bounding box which includes all time steps
    */
    BoundingBox::Pointer m_BoundingBox;

    /**
    * \brief Makes a deep copy of the current object
    */
    LightObject::Pointer InternalClone() const override;

  public:
    mitkClassMacroItkParent(TimeGeometry, itk::Object);
    itkCloneMacro(Self);
    itkCreateAnotherMacro(Self);

      /**
      * \brief Returns the number of time steps.
      *
      * Returns the number of time steps for which
      * geometries are saved. The number of time steps
      * is also the upper bound of the time steps. The
      * minimum time steps is always 0.
      */
      virtual TimeStepType CountTimeSteps() const = 0;
    /**
    * \brief Returns the first time point for which the object is valid.
    *
    * Returns the first valid time point for this geometry. If only one
    * time steps available it usually goes from -max to +max. The time point
    * is given in ms.
    */
    virtual TimePointType GetMinimumTimePoint() const = 0;
    /**
    * \brief Returns the last time point for which the object is valid
    *
    * Gives the last time point for which a valid geometry is saved in
    * this time geometry. The time point is given in ms.
    */
    virtual TimePointType GetMaximumTimePoint() const = 0;

    /**
    * \brief Returns the first time point for which the object is valid.
    *
    * Returns the first valid time point for the given TimeStep. The time point
    * is given in ms.
    */
    virtual TimePointType GetMinimumTimePoint(TimeStepType step) const = 0;
    /**
    * \brief Returns the last time point for which the object is valid
    *
    * Gives the last time point for the Geometry specified by the given TimeStep. The time point is given in ms.
    */
    virtual TimePointType GetMaximumTimePoint(TimeStepType step) const = 0;

    /**
    * \brief Get the time bounds (in ms)
    */
    virtual TimeBounds GetTimeBounds() const = 0;

    /**
    * \brief Get the time bounds for the given TimeStep (in ms)
    */
    virtual TimeBounds GetTimeBounds(TimeStepType step) const = 0;
    /**
    * \brief Tests if a given time point is covered by this object
    *
    * Returns true if a geometry can be returned for the given time
    * point and falls if not. The time point must be given in ms.
    */
    virtual bool IsValidTimePoint(TimePointType timePoint) const = 0;
    /**
    * \brief Test for the given time step if a geometry is available
    *
    * Returns true if a geometry is defined for the given time step.
    * Otherwise false is returned.
    * The time step is defined as positive number.
    */
    virtual bool IsValidTimeStep(TimeStepType timeStep) const = 0;

    /**
    * \brief Converts a time step to a time point
    *
    * Converts a time step to a time point in a way that
    * the new time point indicates the same geometry as the time step.
    * If the original time steps does not point to a valid geometry,
    * a time point is calculated that also does not point to a valid
    * geometry, but no exception is raised.
    */
    virtual TimePointType TimeStepToTimePoint(TimeStepType timeStep) const = 0;
    /**
    * \brief Converts a time point to the corresponding time step
    *
    * Converts a time point to a time step in a way that
    * the new time step indicates the same geometry as the time point.
    * If a negative invalid time point is given always time step 0 is
    * returned. If an positive invalid time step is given an invalid
    * time step will be returned.
    */
    virtual TimeStepType TimePointToTimeStep(TimePointType timePoint) const = 0;

    /**
    * \brief Returns the geometry of a specific time point
    *
    * Returns the geometry which defines the given time point. If
    * the given time point is invalid an null-pointer is returned.
    *
    * The pointer to the returned geometry may point to the saved
    * geometry but this is not necessarily the case. So a change to
    * the returned geometry may or may not afflict the geometry for the
    * time point or all time points depending on the used implementation
    * of TimeGeometry.
    */
    virtual BaseGeometry::Pointer GetGeometryForTimePoint(TimePointType timePoint) const = 0;
    /**
    * \brief Returns the geometry which corresponds to the given time step
    *
    * Returns the geometry which defines the given time step. If
    * the given time step is invalid an null-pointer is returned.
    *
    * The pointer to the returned geometry may point to the saved
    * geometry but this is not necessarily the case. So a change to
    * the returned geometry may or may not afflict the geometry for the
    * time step or all time steps depending on the used implementation
    * of TimeGeometry.
    */
    virtual BaseGeometry::Pointer GetGeometryForTimeStep(TimeStepType timeStep) const = 0;

    /**
    * \brief Returns a clone of the geometry of a specific time point
    *
    * If an invalid time step is given (e.g. no geometry is defined for this time step)
    * a null-pointer will be returned.
    */
    virtual BaseGeometry::Pointer GetGeometryCloneForTimeStep(TimeStepType timeStep) const = 0;

    /**
    * \brief Sets the geometry for a given time step
    *
    * Sets the geometry for the given time steps. This may also afflects other
    * time steps, depending on the implementation of TimeGeometry.
    */
    virtual void SetTimeStepGeometry(BaseGeometry *geometry, TimeStepType timeStep) = 0;

    /**
    * \brief Expands to the given number of time steps
    *
    * Expands to the given number of time steps. Each new created time
    * step is filled with an empty geometry.
    * Shrinking is not supported!
    */
    virtual void Expand(TimeStepType size) = 0;

    /**
    * \brief Replaces the geometry instances with clones ot the passed geometry.
    *
    * Replaces the geometries of all time steps with clones of the passed
    * geometry. Replacement strategy depends on the implementation of TimeGeometry
    * sub class.
    * @remark The time points itself stays untouched. Use this method if you want
    * to change the spatial properties of a TimeGeometry and preserve the time
    * "grid".
    */
    virtual void ReplaceTimeStepGeometries(const BaseGeometry *geometry) = 0;

    /**
    * \brief Tests if all necessary information are set and the object is valid
    */
    virtual bool IsValid() const = 0;
    /**
    * \brief Get the position of the corner number \a id (in world coordinates)
    *
    * See SetImageGeometry for how a corner is defined on images.
    */
    Point3D GetCornerPointInWorld(int id) const;

    /**
    * \brief Get the position of a corner (in world coordinates)
    *
    * See SetImageGeometry for how a corner is defined on images.
    */
    Point3D GetCornerPointInWorld(bool xFront = true, bool yFront = true, bool zFront = true) const;

    /**
    * \brief Get the center of the bounding-box in mm
    */
    Point3D GetCenterInWorld() const;

    /**
    * \brief Get the squared length of the diagonal of the bounding-box in mm
    */
    double GetDiagonalLength2InWorld() const;

    /**
    * \brief Get the length of the diagonal of the bounding-box in mm
    */
    double GetDiagonalLengthInWorld() const;

    /**
    * \brief Test whether the point \a p (world coordinates in mm) is inside the bounding box
    */
    bool IsWorldPointInside(const mitk::Point3D &p) const;

    /**
    * \brief Updates the bounding box to cover the area used in all time steps
    *
    * The bounding box is updated by this method. The new bounding box
    * covers an area which includes all bounding boxes during
    * all times steps.
    */
    void UpdateBoundingBox();

    /**
    * \brief Returns a bounding box that covers all time steps
    */
    BoundingBox *GetBoundingBoxInWorld() const { return m_BoundingBox; }
    /**
    * \brief Returns the world bounds of the object that cover all time steps
    */
    BoundingBox::BoundsArrayType GetBoundsInWorld() const { return m_BoundingBox->GetBounds(); }
    /**
    * \brief Returns the Extend of the bounding in the given direction
    */
    ScalarType GetExtentInWorld(unsigned int direction) const;

    /**
    * \brief Initializes the TimeGeometry
    */
    virtual void Initialize();

    /**
    * \brief Updates the geometry
    */
    void Update();

    /**
    * \brief Updates everything except the Bounding box
    *
    * This class should be overwritten by child classes.
    * The method is called when Update() is required.
    */
    virtual void UpdateWithoutBoundingBox(){};

    /**
    * \brief Executes the given operation on all time steps
    */
    void ExecuteOperation(Operation *op) override;

    void PrintSelf(std::ostream &os, itk::Indent indent) const override;
  }; // end class TimeGeometry

  /**
  * @brief Equal A function comparing two instances of TimeGeometry for being identical.
  *
  * @ingroup MITKTestingAPI
  *
  * The function compares two instances of TimeGeometries in all their aspects.
  *
  * The parameter eps is a tolerance value for all methods which are internally used for comparison.
  * If you want to use different tolerance values for different parts of the geometry, feel free to use
  * the other comparison methods and write your own implementation of Equal.
  *
  * @param rightHandSide Compare this against leftHandSide.
  * @param leftHandSide Compare this against rightHandSide.
  * @param eps Tolerance for comparison. You can use mitk::eps in most cases.
  * @param verbose Flag indicating if the user wants detailed console output or not.
  *
  * @return True, if all comparison are true. False in any other case.
  */
  MITKCORE_EXPORT bool Equal(const mitk::TimeGeometry &leftHandSide,
                             const mitk::TimeGeometry &rightHandSide,
                             ScalarType eps,
                             bool verbose);

  /**
  * @brief Compare two instances of TimeGeometry
  *
  * @ingroup MITKTestingAPI
  *
  * The function compares two instances of TimeGeometries in all their aspects.
  *
  * The parameter eps is a tolerance value for all methods which are internally used for comparison.
  * If you want to use different tolerance values for different parts of the geometry, feel free to use
  * the other comparison methods and write your own implementation of Equal.
  *
  * @param leftHandSide Compare this against rightHandSide.
  * @param rightHandSide Compare this against leftHandSide.
  * @param coordinateEps Tolerance for comparison of all spatial and temporal aspects (spacing, origin and grid alignment, time points).
  * You can use mitk::eps in most cases.
  * @param directionEps Tolerance for comparison of all directional aspects (axis). You can use mitk::eps in most cases.
  * @param verbose Flag indicating if the user wants detailed console output or not.
  *
  * @return True, if all comparisons are true. False in any other case.
  */
  MITKCORE_EXPORT bool Equal(const mitk::TimeGeometry& leftHandSide,
    const mitk::TimeGeometry& rightHandSide,
    ScalarType coordinateEps,
    ScalarType directionEps,
    bool verbose);

} // end namespace MITK
#endif // TimeGeometry_h
