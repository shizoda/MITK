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

// semantic relation module
#include "mitkSemanticRelations.h"
#include "mitkSemanticRelationException.h"
#include "mitkUIDGenerator.h"

// multi label module
#include <mitkLabelSetImage.h>

// mitk core
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>

// c++
#include <algorithm>

mitk::SemanticRelations::SemanticRelations(mitk::DataStorage::Pointer dataStorage)
  : m_DataStorage(dataStorage)
{
  m_RelationStorage = std::make_shared<mitk::RelationStorage>();
  m_RelationStorage->SetDataStorage(m_DataStorage);
}

mitk::SemanticRelations::~SemanticRelations()
{
  // nothing here
}

std::shared_ptr<mitk::RelationStorage> mitk::SemanticRelations::GetRelationStorage()
{
  return m_RelationStorage;
}

/************************************************************************/
/* functions to get instances / attributes                              */
/************************************************************************/

std::vector<SemanticTypes::Lesion> mitk::SemanticRelations::GetAllLesionsOfCase(const SemanticTypes::CaseID& caseID) const
{
  return m_RelationStorage->GetAllLesionsOfCase(caseID);
}

std::vector<SemanticTypes::Lesion> mitk::SemanticRelations::GetAllLesionsOfCase(const SemanticTypes::CaseID& caseID, const SemanticTypes::ControlPoint& controlPoint) const
{
  // prepare node predicate to find image nodes
  mitk::TNodePredicateDataType<mitk::Image>::Pointer isImage = mitk::TNodePredicateDataType<mitk::Image>::New();
  mitk::NodePredicateProperty::Pointer isBinary = mitk::NodePredicateProperty::New("binary", mitk::BoolProperty::New(true));

  // prepare node predicate to filter segmentation nodes
  NodePredicateAnd::Pointer segmentationPredicate = NodePredicateAnd::New();
  segmentationPredicate->AddPredicate(TNodePredicateDataType<LabelSetImage>::New());
  segmentationPredicate->AddPredicate(NodePredicateNot::New(NodePredicateProperty::New("helper object")));

  NodePredicateOr::Pointer validImages = NodePredicateOr::New();
  validImages->AddPredicate(isImage);

  NodePredicateAnd::Pointer imagePredicate = NodePredicateAnd::New();
  imagePredicate->AddPredicate(validImages);
  imagePredicate->AddPredicate(NodePredicateNot::New(segmentationPredicate));
  imagePredicate->AddPredicate(NodePredicateNot::New(NodePredicateAnd::New(isBinary, isImage)));
  imagePredicate->AddPredicate(NodePredicateNot::New(NodePredicateProperty::New("helper object")));

  std::vector<SemanticTypes::Lesion> allLesions = GetAllLesionsOfCase(caseID);

  // filter the lesions: use only those, where the associated data refers to the given control point using a lambda function
  allLesions.erase(std::remove_if(allLesions.begin(), allLesions.end(),
    [&controlPoint, &caseID, &imagePredicate, this](const SemanticTypes::Lesion& lesion)
  {
    std::vector<DataNode::Pointer> allDataOfLesion;
    try
    {
      if (m_DataStorage.IsNotNull())
      {
        // 1. get all segmentations that define the lesion
        // 2. retrieve the parent node (source) of the found segmentation node
        // 3. use the parent node (the image) to compare the control points (intersect)
        std::vector<DataNode::Pointer> allSegmentationsOfLesion = GetAllSegmentationsOfLesion(caseID, lesion);
        for (const auto& segmentationNode : allSegmentationsOfLesion)
        {
          // get parent node of the current segmentation node with the node predicate
          DataStorage::SetOfObjects::ConstPointer parentNodes = m_DataStorage->GetSources(segmentationNode, imagePredicate, false);
          for (auto it = parentNodes->Begin(); it != parentNodes->End(); ++it)
          {
            DataNode::Pointer dataNode = it->Value();
            allDataOfLesion.push_back(it->Value());
          }
        }
      }
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::vector<DataNode::Pointer> allDataOfControlPoint;
    try
    {
      allDataOfControlPoint = GetAllDataOfControlPoint(caseID, controlPoint);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::sort(allDataOfLesion.begin(), allDataOfLesion.end());
    std::sort(allDataOfControlPoint.begin(), allDataOfControlPoint.end());
    std::vector<DataNode::Pointer> allDataIntersection;
    // set intersection removes duplicated nodes, since 'allDataOfControlPoint' only contains at most one of each node
    std::set_intersection(allDataOfLesion.begin(), allDataOfLesion.end(),
      allDataOfControlPoint.begin(), allDataOfControlPoint.end(), std::back_inserter(allDataIntersection));
    return allDataIntersection.empty();
  }),
    allLesions.end());

  return allLesions;
}

std::vector<SemanticTypes::Lesion> mitk::SemanticRelations::GetAllLesionsInImage(const DataNode* imageNode) const
{
  if (nullptr == imageNode)
  {
    MITK_WARN << "Not a valid image data node.";
    return std::vector<SemanticTypes::Lesion>();
  }

  std::vector<SemanticTypes::Lesion> allLesionsInImage;
  // prepare node predicate to find images
  NodePredicateAnd::Pointer segmentationPredicate = NodePredicateAnd::New();
  segmentationPredicate->AddPredicate(TNodePredicateDataType<LabelSetImage>::New());
  segmentationPredicate->AddPredicate(NodePredicateNot::New(NodePredicateProperty::New("helper object")));

  if (m_DataStorage.IsNotNull())
  {
    // get child nodes of the current node with the segmentation predicate
    DataStorage::SetOfObjects::ConstPointer segmentationNodes = m_DataStorage->GetDerivations(imageNode, segmentationPredicate, false);
    for (auto it = segmentationNodes->Begin(); it != segmentationNodes->End(); ++it)
    {
      DataNode* segmentationNode = it->Value();
      try
      {
        SemanticTypes::Lesion representedLesion = GetRepresentedLesion(segmentationNode);
        allLesionsInImage.push_back(representedLesion);
      }
      catch (const SemanticRelationException&)
      {
        continue;
      }
    }
  }
  return allLesionsInImage;
}

SemanticTypes::Lesion mitk::SemanticRelations::GetRepresentedLesion(const DataNode* segmentationNode) const
{
  if (nullptr == segmentationNode)
  {
    MITK_WARN << "Not a valid segmentation node.";
    return SemanticTypes::Lesion();
  }

  SemanticTypes::Lesion representedLesion;
  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(segmentationNode);
  SemanticTypes::ID segmentationID = NodeIdentifier::GetIDFromData(segmentationNode);
  representedLesion = m_RelationStorage->GetRepresentedLesion(caseID, segmentationID);

  if (representedLesion.UID.empty())
  {
    mitkThrowException(SemanticRelationException) << "Could not find a represented lesion instance for the given segmentation node " << segmentationNode->GetName();
  }
  else
  {
    return representedLesion;
  }
}

bool mitk::SemanticRelations::IsRepresentingALesion(const DataNode* segmentationNode) const
{
  SemanticTypes::Lesion representedLesion;
  try
  {
    representedLesion = GetRepresentedLesion(segmentationNode);
    return true;
  }
  catch (const SemanticRelationException&)
  {
    return false;
  }
}

std::vector<mitk::DataNode::Pointer> mitk::SemanticRelations::GetAllSegmentationsOfLesion(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion) const
{
  if (InstanceExists(caseID, lesion))
  {
    // lesion exists, retrieve all case segmentations from the storage
    std::vector<DataNode::Pointer> allSegmentationsOfLesion = m_RelationStorage->GetAllSegmentationsOfCase(caseID);

    // filter all segmentations: check for semantic relation with the given lesion using a lambda function
    allSegmentationsOfLesion.erase(std::remove_if(allSegmentationsOfLesion.begin(), allSegmentationsOfLesion.end(),
      [&lesion, this](DataNode::Pointer segmentation)
    {
      try
      {
        SemanticTypes::Lesion representedLesion = GetRepresentedLesion(segmentation);
        return lesion.UID != representedLesion.UID;
      }
      catch (const SemanticRelationException&)
      {
        return true;
      }
    }),
      allSegmentationsOfLesion.end());

    return allSegmentationsOfLesion;
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "Could not find an existing lesion instance for the given caseID " << caseID << " and lesion " << lesion.UID << ".";
  }
}

bool mitk::SemanticRelations::InstanceExists(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion) const
{
  std::vector<SemanticTypes::Lesion> allLesions = GetAllLesionsOfCase(caseID);

  // filter all lesions: check for equality with the given lesion using a lambda function
  const auto existingLesion = std::find_if(allLesions.begin(), allLesions.end(),
    [&lesion](const SemanticTypes::Lesion& currentLesion) { return currentLesion.UID == lesion.UID; });

  if (existingLesion != allLesions.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

std::vector<SemanticTypes::ControlPoint> mitk::SemanticRelations::GetAllControlPointsOfCase(const SemanticTypes::CaseID& caseID) const
{
  return m_RelationStorage->GetAllControlPointsOfCase(caseID);
}

std::vector<SemanticTypes::ControlPoint> mitk::SemanticRelations::GetAllControlPointsOfCase(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion) const
{
  // prepare node predicate to find image nodes
  mitk::TNodePredicateDataType<mitk::Image>::Pointer isImage = mitk::TNodePredicateDataType<mitk::Image>::New();
  mitk::NodePredicateProperty::Pointer isBinary = mitk::NodePredicateProperty::New("binary", mitk::BoolProperty::New(true));

  // prepare node predicate to filter segmentation nodes
  NodePredicateAnd::Pointer segmentationPredicate = NodePredicateAnd::New();
  segmentationPredicate->AddPredicate(TNodePredicateDataType<LabelSetImage>::New());
  segmentationPredicate->AddPredicate(NodePredicateNot::New(NodePredicateProperty::New("helper object")));

  NodePredicateOr::Pointer validImages = NodePredicateOr::New();
  validImages->AddPredicate(isImage);

  NodePredicateAnd::Pointer imagePredicate = NodePredicateAnd::New();
  imagePredicate->AddPredicate(validImages);
  imagePredicate->AddPredicate(NodePredicateNot::New(segmentationPredicate));
  imagePredicate->AddPredicate(NodePredicateNot::New(NodePredicateAnd::New(isBinary, isImage)));
  imagePredicate->AddPredicate(NodePredicateNot::New(NodePredicateProperty::New("helper object")));

  std::vector<SemanticTypes::ControlPoint> allControlPoints = GetAllControlPointsOfCase(caseID);

  // filter the control points: use only those, where the associated data has a segmentation that refers to the given lesion using a lambda function
  allControlPoints.erase(std::remove_if(allControlPoints.begin(), allControlPoints.end(),
    [&lesion, &caseID, &imagePredicate, this](const SemanticTypes::ControlPoint& controlPoint)
  {
    std::vector<DataNode::Pointer> allDataOfControlPoint;
    try
    {
      allDataOfControlPoint = GetAllDataOfControlPoint(caseID, controlPoint);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::vector<DataNode::Pointer> allDataOfLesion;
    try
    {
      if (m_DataStorage.IsNotNull())
      {
        // 1. get all segmentations that define the lesion
        // 2. retrieve the parent node (source) of the found segmentation node
        // 3. use the parent node (the image) to compare the control points (intersect)
        std::vector<DataNode::Pointer> allSegmentationsOfLesion = GetAllSegmentationsOfLesion(caseID, lesion);
        for (const auto& segmentationNode : allSegmentationsOfLesion)
        {
          // get parent node of the current segmentation node with the node predicate
          DataStorage::SetOfObjects::ConstPointer parentNodes = m_DataStorage->GetSources(segmentationNode, imagePredicate, false);
          for (auto it = parentNodes->Begin(); it != parentNodes->End(); ++it)
          {
            DataNode::Pointer dataNode = it->Value();
            allDataOfLesion.push_back(it->Value());
          }
        }
      }
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::sort(allDataOfControlPoint.begin(), allDataOfControlPoint.end());
    std::sort(allDataOfLesion.begin(), allDataOfLesion.end());
    std::vector<DataNode::Pointer> allDataIntersection;
    // set intersection removes duplicated nodes, since 'allDataOfControlPoint' only contains at most one of each node
    std::set_intersection(allDataOfControlPoint.begin(), allDataOfControlPoint.end(),
      allDataOfLesion.begin(), allDataOfLesion.end(), std::back_inserter(allDataIntersection));
    return allDataIntersection.empty();
  }),
    allControlPoints.end());

  return allControlPoints;
}

std::vector<SemanticTypes::ControlPoint> mitk::SemanticRelations::GetAllControlPointsOfCase(const SemanticTypes::CaseID& caseID, const SemanticTypes::InformationType& informationType) const
{
  std::vector<SemanticTypes::ControlPoint> allControlPoints = GetAllControlPointsOfCase(caseID);

  // filter the control points: use only those, where the associated data refers to the given information type using a lambda function
  allControlPoints.erase(std::remove_if(allControlPoints.begin(), allControlPoints.end(),
    [&informationType, &caseID, this](const SemanticTypes::ControlPoint& controlPoint)
  {
    std::vector<DataNode::Pointer> allDataOfControlPoint;
    try
    {
      allDataOfControlPoint = GetAllDataOfControlPoint(caseID, controlPoint);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::vector<DataNode::Pointer> allDataOfInformationType;
    try
    {
      allDataOfInformationType = GetAllDataOfInformationType(caseID, informationType);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::sort(allDataOfControlPoint.begin(), allDataOfControlPoint.end());
    std::sort(allDataOfInformationType.begin(), allDataOfInformationType.end());
    std::vector<DataNode::Pointer> allDataIntersection;
    std::set_intersection(allDataOfControlPoint.begin(), allDataOfControlPoint.end(),
      allDataOfInformationType.begin(), allDataOfInformationType.end(), std::back_inserter(allDataIntersection));
    return allDataIntersection.empty();
  }),
    allControlPoints.end());

  return allControlPoints;
}

SemanticTypes::ControlPoint mitk::SemanticRelations::GetControlPointOfData(const DataNode* dataNode) const
{
  if (nullptr == dataNode)
  {
    MITK_WARN << "Not a valid data node.";
    return SemanticTypes::ControlPoint();
  }

  SemanticTypes::ControlPoint controlPoint;
  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(dataNode);
  SemanticTypes::ID dataNodeID = NodeIdentifier::GetIDFromData(dataNode);
  return m_RelationStorage->GetControlPointOfData(caseID, dataNodeID);
}

std::vector<mitk::DataNode::Pointer> mitk::SemanticRelations::GetAllDataOfControlPoint(const SemanticTypes::CaseID& caseID, const SemanticTypes::ControlPoint& controlPoint) const
{
  if (InstanceExists(caseID, controlPoint))
  {
    // control point exists, retrieve all images from the storage
    std::vector<DataNode::Pointer> allDataOfControlPoint = m_RelationStorage->GetAllImagesOfCase(caseID);

    // filter all images to remove the ones with a different control point using a lambda function
    allDataOfControlPoint.erase(std::remove_if(allDataOfControlPoint.begin(), allDataOfControlPoint.end(),
      [&controlPoint, this](DataNode::Pointer imageNode) { return controlPoint.UID != GetControlPointOfData(imageNode).UID; }),
    allDataOfControlPoint.end());

    return allDataOfControlPoint;
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "Could not find an existing control point instance for the given caseID " << caseID << " and control point " << controlPoint.UID << ".";
  }
}

bool mitk::SemanticRelations::InstanceExists(const SemanticTypes::CaseID& caseID, const SemanticTypes::ControlPoint& controlPoint) const
{
  std::vector<SemanticTypes::ControlPoint> allControlPoints = GetAllControlPointsOfCase(caseID);

  // filter all control points: check for equality with the given control point using a lambda function
  const auto existingControlPoint = std::find_if(allControlPoints.begin(), allControlPoints.end(),
    [&controlPoint](const SemanticTypes::ControlPoint& currentControlPoint) { return currentControlPoint.UID == controlPoint.UID; });

  if (existingControlPoint != allControlPoints.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

std::vector<SemanticTypes::InformationType> mitk::SemanticRelations::GetAllInformationTypesOfCase(const SemanticTypes::CaseID& caseID) const
{
  return m_RelationStorage->GetAllInformationTypesOfCase(caseID);
}

std::vector<SemanticTypes::InformationType> mitk::SemanticRelations::GetAllInformationTypesOfCase(const SemanticTypes::CaseID& caseID, const SemanticTypes::ControlPoint& controlPoint) const
{
  std::vector<SemanticTypes::InformationType> allInformationTypes = GetAllInformationTypesOfCase(caseID);

  // filter the information types: use only those, where the associated data refers to the given control point using a lambda function
  allInformationTypes.erase(std::remove_if(allInformationTypes.begin(), allInformationTypes.end(),
    [&controlPoint, &caseID, this](const SemanticTypes::InformationType& informationType)
  {
    std::vector<DataNode::Pointer> allDataOfInformationType;
    try
    {
      allDataOfInformationType = GetAllDataOfInformationType(caseID, informationType);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::vector<DataNode::Pointer> allDataOfControlPoint;
    try
    {
      allDataOfControlPoint = GetAllDataOfControlPoint(caseID, controlPoint);
    }
    catch (const SemanticRelationException&)
    {
      return true;
    }
    std::sort(allDataOfInformationType.begin(), allDataOfInformationType.end());
    std::sort(allDataOfControlPoint.begin(), allDataOfControlPoint.end());
    std::vector<DataNode::Pointer> allDataIntersection;
    std::set_intersection(allDataOfInformationType.begin(), allDataOfInformationType.end(),
      allDataOfControlPoint.begin(), allDataOfControlPoint.end(), std::back_inserter(allDataIntersection));
    return allDataIntersection.empty();
  }),
    allInformationTypes.end());

  return allInformationTypes;
}

SemanticTypes::InformationType mitk::SemanticRelations::GetInformationTypeOfImage(const DataNode* imageNode) const
{
  if (nullptr == imageNode)
  {
    MITK_WARN << "Not a valid image data node.";
    return SemanticTypes::InformationType();
  }

  SemanticTypes::InformationType informationType;
  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(imageNode);
  SemanticTypes::ID imageID = NodeIdentifier::GetIDFromData(imageNode);
  return m_RelationStorage->GetInformationTypeOfImage(caseID, imageID);
}

std::vector<mitk::DataNode::Pointer> mitk::SemanticRelations::GetAllDataOfInformationType(const SemanticTypes::CaseID& caseID, const SemanticTypes::InformationType& informationType) const
{
  if (InstanceExists(caseID, informationType))
  {
    // information type exists, retrieve all images from the storage
    std::vector<DataNode::Pointer> allDataOfInformationType = m_RelationStorage->GetAllImagesOfCase(caseID);

    // filter all images to remove the ones with a different information type using a lambda function
    allDataOfInformationType.erase(std::remove_if(allDataOfInformationType.begin(), allDataOfInformationType.end(),
      [&informationType, this](DataNode::Pointer imageNode) { return informationType != GetInformationTypeOfImage(imageNode); }),
      allDataOfInformationType.end());

    return allDataOfInformationType;
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "Could not find an existing information type for the given caseID " << caseID << " and information type " << informationType;
  }
}

bool mitk::SemanticRelations::InstanceExists(const SemanticTypes::CaseID& caseID, const SemanticTypes::InformationType& informationType) const
{
  std::vector<SemanticTypes::InformationType> allInformationTypes = GetAllInformationTypesOfCase(caseID);

  // filter all information types: check for equality with the given information type using a lambda function
  const auto existingInformationType = std::find_if(allInformationTypes.begin(), allInformationTypes.end(),
    [&informationType](const SemanticTypes::InformationType& currentInformationType) { return currentInformationType == informationType; });

  if (existingInformationType != allInformationTypes.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

std::vector<SemanticTypes::CaseID> mitk::SemanticRelations::GetAllCaseIDs() const
{
  return m_RelationStorage->GetAllCaseIDs();
}

/************************************************************************/
/* functions to add / remove instances / attributes                     */
/************************************************************************/

void mitk::SemanticRelations::AddLesion(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion)
{
  if (InstanceExists(caseID, lesion))
  {
    mitkThrowException(SemanticRelationException) << "The lesion " << lesion.UID << " to add already exists for the given case.";
  }
  else
  {
    m_RelationStorage->AddLesion(caseID, lesion);
  }
}

void mitk::SemanticRelations::OverwriteLesion(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion)
{
  if (InstanceExists(caseID, lesion))
  {
    m_RelationStorage->OverwriteLesion(caseID, lesion);
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The lesion " << lesion.UID << " to overwrite does not exist for the given case.";
  }
}

void mitk::SemanticRelations::AddLesionAndLinkData(const DataNode* segmentationNode, const SemanticTypes::Lesion& lesion)
{
  if (nullptr == segmentationNode)
  {
    MITK_WARN << "Not a valid segmentation node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(segmentationNode);
  try
  {
    AddLesion(caseID, lesion);
  }
  catch (SemanticRelationException& e)
  {
    mitkReThrow(e);
  }

  try
  {
    LinkSegmentationToLesion(segmentationNode, lesion);
  }
  catch (SemanticRelationException& e)
  {
    mitkReThrow(e);
  }
}

void mitk::SemanticRelations::LinkSegmentationToLesion(const DataNode* segmentationNode, const SemanticTypes::Lesion& lesion)
{
  if (nullptr == segmentationNode)
  {
    MITK_WARN << "Not a valid segmentation node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(segmentationNode);
  if (InstanceExists(caseID, lesion))
  {
    SemanticTypes::ID segmentationID = NodeIdentifier::GetIDFromData(segmentationNode);
    m_RelationStorage->LinkSegmentationToLesion(caseID, segmentationID, lesion);
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The lesion " << lesion.UID << " to link does not exist for the given case.";
  }
}

void mitk::SemanticRelations::RemoveLesion(const SemanticTypes::CaseID& caseID, const SemanticTypes::Lesion& lesion)
{
  if (InstanceExists(caseID, lesion))
  {
    std::vector<DataNode::Pointer> allSegmentationsOfLesion = GetAllSegmentationsOfLesion(caseID, lesion);
    if (allSegmentationsOfLesion.empty())
    {
      m_RelationStorage->RemoveLesion(caseID, lesion);
    }
    else
    {
      mitkThrowException(SemanticRelationException) << "The lesion " << lesion.UID << " to remove is still referred to by a segmentation node. Lesion will not be removed.";
    }
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The lesion " << lesion.UID << " to remove does not exist for the given case.";
  }
}

void mitk::SemanticRelations::AddControlPointAndLinkData(const DataNode* dataNode, const SemanticTypes::ControlPoint& controlPoint)
{
  if (nullptr == dataNode)
  {
    MITK_WARN << "Not a valid data node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(dataNode);
  if (InstanceExists(caseID, controlPoint))
  {
    mitkThrowException(SemanticRelationException) << "The control point " << controlPoint.UID << " to add already exists for the given case.";
  }
  else
  {
    bool insideControlPoint = ControlPointManager::InsideControlPoint(dataNode, controlPoint);
    if (insideControlPoint)
    {
      bool overlapping = CheckOverlappingControlPoint(caseID, controlPoint);
      if (overlapping)
      {
        mitkThrowException(SemanticRelationException) << "The control point " << controlPoint.UID << " to add overlaps with an already existing control point";
      }
      else
      {
        m_RelationStorage->AddControlPoint(caseID, controlPoint);
      }
    }
    else
    {
      mitkThrowException(SemanticRelationException) << "The control point " << controlPoint.UID << " to add does not contain the date of the given data node.";
    }
  }

  try
  {
    LinkDataToControlPoint(dataNode, controlPoint);
  }
  catch (SemanticRelationException& e)
  {
    mitkReThrow(e);
  }
}

void mitk::SemanticRelations::OverwriteControlPointAndLinkData(const DataNode* dataNode, const SemanticTypes::ControlPoint& controlPoint)
{
  if (nullptr == dataNode)
  {
    MITK_WARN << "Not a valid data node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(dataNode);

  std::vector<SemanticTypes::ControlPoint> allControlPoints = GetAllControlPointsOfCase(caseID);
  const auto existingControlPoint = std::find_if(allControlPoints.begin(), allControlPoints.end(),
    [&controlPoint](const SemanticTypes::ControlPoint& currentControlPoint) { return currentControlPoint.UID == controlPoint.UID; });

  if (existingControlPoint != allControlPoints.end())
  {
    bool insideControlPoint = ControlPointManager::InsideControlPoint(dataNode, controlPoint);
    if (insideControlPoint)
    {
      bool sameStartPoint = controlPoint.startPoint == (*existingControlPoint).startPoint;
      bool sameEndPoint = controlPoint.endPoint == (*existingControlPoint).endPoint;
      if (!sameStartPoint && !sameEndPoint)
      {
        mitkThrowException(SemanticRelationException) << "The overwriting control point " << controlPoint.UID << " differs in the start date and in the end date from the original control point.";
      }
      if (sameStartPoint && sameEndPoint)
      {
        mitkThrowException(SemanticRelationException) << "The overwriting control point " << controlPoint.UID << " does not differ from the original control point.";
      }

      bool overlapping = CheckOverlappingControlPoint(caseID, controlPoint);
      if (overlapping)
      {
        mitkThrowException(SemanticRelationException) << "The overwriting control point " << controlPoint.UID << " overlaps with an already existing control point";
      }
      else
      {
        m_RelationStorage->OverwriteControlPoint(caseID, controlPoint);

        try
        {
          LinkDataToControlPoint(dataNode, controlPoint);
        }
        catch (SemanticRelationException& e)
        {
          mitkReThrow(e);
        }
      }
    }
    else
    {
      mitkThrowException(SemanticRelationException) << "The data to link does not lie inside the given control point.";
    }
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The control point " << controlPoint.UID << " to link does not exist for the given case.";
  }
}

void mitk::SemanticRelations::LinkDataToControlPoint(const DataNode* dataNode, const SemanticTypes::ControlPoint& controlPoint)
{
  if (nullptr == dataNode)
  {
    MITK_WARN << "Not a valid data node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(dataNode);
  if (InstanceExists(caseID, controlPoint))
  {
    SemanticTypes::ID dataID = NodeIdentifier::GetIDFromData(dataNode);
    bool insideControlPoint = ControlPointManager::InsideControlPoint(dataNode, controlPoint);
    if (insideControlPoint)
    {
      m_RelationStorage->LinkDataToControlPoint(caseID, dataID, controlPoint);
    }
    else
    {
      mitkThrowException(SemanticRelationException) << "The data to link does not lie inside the given control point " << controlPoint.UID << " .";
    }
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The control point " << controlPoint.UID << " to link does not exist for the given case.";
  }
}

void mitk::SemanticRelations::UnlinkDataFromControlPoint(const DataNode* dataNode, const SemanticTypes::ControlPoint& controlPoint)
{
  if (nullptr == dataNode)
  {
    MITK_WARN << "Not a valid data node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(dataNode);
  if (InstanceExists(caseID, controlPoint))
  {
    SemanticTypes::ID dataID = NodeIdentifier::GetIDFromData(dataNode);
    m_RelationStorage->UnlinkDataFromControlPoint(caseID, dataID);

    try
    {
      std::vector<DataNode::Pointer> allDataOfControlPoint = GetAllDataOfControlPoint(caseID, controlPoint);
      if (allDataOfControlPoint.empty())
      {
        // no more data is linked to the specific control point
        // the control point can be removed from the storage
        m_RelationStorage->RemoveControlPointFromCase(caseID, controlPoint);
      }
      else
      {
        // some data is still linked to this control point
        // the control point can not be removed, but has to be adjusted to fit the remaining data
        SemanticTypes::ControlPoint adjustedControlPoint = ControlPointManager::GenerateControlPoint(allDataOfControlPoint);
        // set the UIDs to be the same, so that all references still work
        adjustedControlPoint.UID = controlPoint.UID;
        adjustedControlPoint.startPoint.UID = controlPoint.startPoint.UID;
        adjustedControlPoint.endPoint.UID = controlPoint.endPoint.UID;
        m_RelationStorage->OverwriteControlPoint(caseID, adjustedControlPoint);
      }
    }
    catch (SemanticRelationException& e)
    {
      mitkReThrow(e);
    }
  }
  else
  {
    mitkThrowException(SemanticRelationException) << "The control point to unlink does not exist for the given case.";
  }
}

void mitk::SemanticRelations::AddInformationTypeToImage(const DataNode* imageNode, const SemanticTypes::InformationType& informationType)
{
  if (nullptr == imageNode)
  {
    MITK_WARN << "Not a valid image node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(imageNode);
  SemanticTypes::ID imageID = NodeIdentifier::GetIDFromData(imageNode);
  m_RelationStorage->AddInformationTypeToImage(caseID, imageID, informationType);
}

void mitk::SemanticRelations::RemoveInformationTypeFromImage(const DataNode* imageNode, const SemanticTypes::InformationType& informationType)
{
  if (nullptr == imageNode)
  {
    MITK_WARN << "Not a valid image node.";
    return;
  }

  SemanticTypes::CaseID caseID = NodeIdentifier::GetCaseIDFromData(imageNode);
  SemanticTypes::ID imageID = NodeIdentifier::GetIDFromData(imageNode);
  m_RelationStorage->RemoveInformationTypeFromImage(caseID, imageID);

  std::vector<std::string> allImageIDsVectorValue = m_RelationStorage->GetAllImageIDsOfCase(caseID);
  for (const auto otherImageID : allImageIDsVectorValue)
  {
    SemanticTypes::InformationType otherInformationType = m_RelationStorage->GetInformationTypeOfImage(caseID, otherImageID);
    if (otherInformationType == informationType)
    {
      // found the information type in another image -> cannot remove the information type from the case
      return;
    }
  }

  // given information type was not referred by any other image of the case -> the information type can be removed from the case
  m_RelationStorage->RemoveInformationTypeFromCase(caseID, informationType);
}

/************************************************************************/
/* private functions                                                    */
/************************************************************************/
bool mitk::SemanticRelations::CheckOverlappingControlPoint(const SemanticTypes::CaseID& caseID, const SemanticTypes::ControlPoint& controlPoint)
{
  std::vector<SemanticTypes::ControlPoint> allControlPoints = GetAllControlPointsOfCase(caseID);
  if (allControlPoints.empty())
  {
    return false;
  }

  std::sort(allControlPoints.begin(), allControlPoints.end());
  const auto existingControlPoint = std::find_if(allControlPoints.begin(), allControlPoints.end(),
    [&controlPoint](const SemanticTypes::ControlPoint& currentControlPoint) { return currentControlPoint.UID == controlPoint.UID; });

  if (existingControlPoint != allControlPoints.end())
  {
    // case overwriting: control point already contained
    // check the neighboring control points for overlap
    bool overlapWithNext = false;
    bool overlapWithPrevious = false;

    const auto nextControlPoint = std::next(existingControlPoint, 1);
    if (nextControlPoint != allControlPoints.end())
    {
      overlapWithNext = ControlPointManager::CheckForOverlap(controlPoint, *nextControlPoint);
    }
    if (existingControlPoint != allControlPoints.begin())
    {
      overlapWithPrevious = ControlPointManager::CheckForOverlap(controlPoint, *std::prev(existingControlPoint, 1));
    }
    return overlapWithNext || overlapWithPrevious; // return true if at least one overlap is detected
  }
  else
  {
    // case adding: control point not contained
    // find the neighboring control points and check for overlap
    const auto nextControlPoint = std::find_if(allControlPoints.begin(), allControlPoints.end(),
      [&controlPoint](const SemanticTypes::ControlPoint& currentControlPoint) { return currentControlPoint.startPoint >= controlPoint.endPoint; });

    if (nextControlPoint != allControlPoints.end())
    {
      bool overlapWithNext = ControlPointManager::CheckForOverlap(controlPoint, *nextControlPoint);
      bool overlapWithPrevious = false;
      if (nextControlPoint != allControlPoints.begin())
      {
        overlapWithPrevious = ControlPointManager::CheckForOverlap(controlPoint, *std::prev(nextControlPoint, 1));
      }

      return overlapWithNext || overlapWithPrevious; // return true if at least one overlap is detected
    }
  }
  return false;
}
