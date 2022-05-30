/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkImageToContourFilter.h"
#include "mitkImageAccessByItk.h"
#include "mitkImageCast.h"
#include "mitkVtkRepresentationProperty.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include <itkImageDuplicator.h>

#include <mitkIOUtil.h>
#include "itkImageFileWriter.h"
// #include "itkImageRegionIterator.h"
#include <itkThresholdImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <mitkBinaryThresholdULTool.h>
#include <itkChangeLabelImageFilter.h>
#include <itkImageFileWriter.h>

mitk::ImageToContourFilter::ImageToContourFilter()
{
  this->m_UseProgressBar = false;
  this->m_ProgressStepSize = 1;
  this->m_SliceGeometry = nullptr;
}

mitk::ImageToContourFilter::~ImageToContourFilter()
{
}

void mitk::ImageToContourFilter::GenerateData()
{
  mitk::Image::ConstPointer sliceImage = ImageToSurfaceFilter::GetInput();

  if (!sliceImage)
  {
    MITK_ERROR << "mitk::ImageToContourFilter: No input available. Please set the input!" << std::endl;
    itkExceptionMacro("mitk::ImageToContourFilter: No input available. Please set the input!");
    return;
  }

  if (sliceImage->GetDimension() > 2 || sliceImage->GetDimension() < 2)
  {
    MITK_ERROR << "mitk::ImageToImageFilter::GenerateData() works only with 2D images. Please assure that your input "
                  "image is 2D!"
               << std::endl;
    itkExceptionMacro(
      "mitk::ImageToImageFilter::GenerateData() works only with 2D images. Please assure that your input image is 2D!");
    return;
  }

  m_SliceGeometry = sliceImage->GetGeometry();

  AccessFixedDimensionByItk(sliceImage,Itk2DContourExtraction, 2);

  // Setting progressbar
  if (this->m_UseProgressBar)
    mitk::ProgressBar::GetInstance()->Progress(this->m_ProgressStepSize);
}

template<typename TPixel, unsigned int VImageDimension>
void ExtractImageWithPixelValue(const itk::Image<TPixel,VImageDimension>* im, TPixel pixelVal)
{
  // itk::ImageIterator  it( im, im->GetRequestedRegion() );
  using ImageType=itk::Image<TPixel,VImageDimension>;
  // using IteratorType=itk::ImageIterator<ImageType>;

  using DuplicatorType = itk::ImageDuplicator< ImageType >;
  auto duplicator = DuplicatorType::New();
  duplicator->SetInputImage(im);
  duplicator->Update();
  typename ImageType::Pointer clonedImage = duplicator->GetOutput();
  std::cout << "pixelVal: " << pixelVal << "\n";
  

  using ThresholdFilterType=itk::ThresholdImageFilter<ImageType>;
  auto thresholdFilter=ThresholdFilterType::New();
  thresholdFilter->SetInput(im);
  thresholdFilter->ThresholdOutside(pixelVal-0.1, pixelVal+0.1);
  thresholdFilter->SetOutsideValue(0);
  thresholdFilter->Update();
  auto threshImage=thresholdFilter->GetOutput();

  // IteratorType  resultIter( clonedImage.GetPointer(), clonedImage->GetRequestedRegion() );
  // resultIter.GoToBegin();
  // while( !resultIter.IsAtEnd() )
  // {
  //   if(resultIter.Get()!=pixelVal)
  //   {
  //     resultIter.Set(0);
  //   }
      
  //   ++resultIter;
  // }

  // regionIndex[0]=0;
  // regionIndex[1]=0;


}
template<typename TPixel, unsigned int VImageDimension>
void extractContoursWithValue(const itk::Image<TPixel,VImageDimension>* sliceImage, TPixel contourValue)
{
  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typename ImageType::Pointer resultImage = ImageType::New();
  resultImage->SetRegions(sliceImage->GetLargestPossibleRegion());
  resultImage->Allocate();

  typename ImageType::RegionType imgRegion=sliceImage->GetLargestPossibleRegion();
  using imageRegionConstIteratorType=itk::ImageRegionConstIterator<ImageType>;
  using imageRegionIteratorType=itk::ImageRegionIterator<ImageType>;
  imageRegionConstIteratorType imIter(sliceImage,sliceImage->GetLargestPossibleRegion());
  imageRegionIteratorType resultImgIter(resultImage,resultImage->GetLargestPossibleRegion());
  auto rgn=sliceImage->GetLargestPossibleRegion();
  imIter.GoToBegin();
  resultImgIter.GoToBegin();
  // ++imIter;
  for (; !imIter.IsAtEnd(),!resultImgIter.IsAtEnd(); ++imIter,++resultImgIter)
  {
    if(fabs(imIter.Get()-contourValue)<1e-2)
    {
      resultImgIter.Set(imIter.Get());
    }
    else
    {
      resultImgIter.Set(0);
    }
  }
}

template<typename TPixel, unsigned int VImageDimension>
typename itk::Image<TPixel,VImageDimension>::Pointer ExtractLabelledContours(const itk::Image<TPixel,VImageDimension>* sliceImage,TPixel m_ContourValue)
{
  typedef itk::Image<TPixel, VImageDimension> ImageType;
  //  THERE HAS TO BE A BETTER WAY TO DO THIS
  /*
    Copy the pixels equal to the contour value only.
    There seems to be an itkContourExtraction2DImageFilter in itk has a LabelsOn switch which seems to do this as is.
    MITK should also have some way to simplify this code below.
  */
  typename ImageType::Pointer image = ImageType::New();
  image->SetRegions(sliceImage->GetLargestPossibleRegion());
  image->Allocate();
  typename ImageType::RegionType imgRegion=sliceImage->GetLargestPossibleRegion();
  using imageRegionConstIteratorType=itk::ImageRegionConstIterator<ImageType>;
  using imageRegionIteratorType=itk::ImageRegionIterator<ImageType>;
  imageRegionConstIteratorType imIter(sliceImage,sliceImage->GetLargestPossibleRegion());
  imageRegionIteratorType resultImgIter(image,image->GetLargestPossibleRegion());
  auto rgn=sliceImage->GetLargestPossibleRegion();
  imIter.GoToBegin();
  resultImgIter.GoToBegin();
  for (; !imIter.IsAtEnd(),!resultImgIter.IsAtEnd(); ++imIter,++resultImgIter)
  {
    if(fabs(imIter.Get()-m_ContourValue)<1e-2)
    {
      resultImgIter.Set(imIter.Get());
    }
    else
    {
      resultImgIter.Set(0);
    }
  }
  return image;
}


template <typename TPixel, unsigned int VImageDimension>
void mitk::ImageToContourFilter::Itk2DContourExtraction(const itk::Image<TPixel, VImageDimension> *sliceImage)
{
  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typedef itk::ContourExtractor2DImageFilter<ImageType> ContourExtractor;

  typedef itk::ConstantPadImageFilter<ImageType, ImageType> PadFilterType;
  typename PadFilterType::Pointer padFilter = PadFilterType::New();
  typename ImageType::SizeType lowerExtendRegion;
  lowerExtendRegion[0] = 1;
  lowerExtendRegion[1] = 1;

  typename ImageType::SizeType upperExtendRegion;
  upperExtendRegion[0] = 1;
  upperExtendRegion[1] = 1;
  

  typename ImageType::Pointer resultImage=ExtractLabelledContours<TPixel,VImageDimension>(sliceImage,m_ContourValue);
  /*
   * We need to pad here, since the ITK contour extractor fails if the
   * segmentation touches more than one image edge.
   * By padding the image for one row at each edge we overcome this issue
   */
  padFilter->SetInput(resultImage);
  padFilter->SetConstant(0);
  padFilter->SetPadLowerBound(lowerExtendRegion);
  padFilter->SetPadUpperBound(upperExtendRegion);

  typename ContourExtractor::Pointer contourExtractor = ContourExtractor::New();
  contourExtractor->SetInput(padFilter->GetOutput());
  if(fabs(m_ContourValue)<0)
  {
    m_ContourValue=0.5;
  }
  else
  {
    m_ContourValue=m_ContourValue-1.0;
  }
  contourExtractor->SetContourValue(m_ContourValue);
  contourExtractor->Update();

  unsigned int foundPaths = contourExtractor->GetNumberOfOutputs();

  vtkSmartPointer<vtkPolyData> contourSurface = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> polygons = vtkSmartPointer<vtkCellArray>::New();

  unsigned int pointId(0);


  for (unsigned int i = 0; i < foundPaths; i++)
  {
    const ContourPath *currentPath = contourExtractor->GetOutput(i)->GetVertexList();

    vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
    polygon->GetPointIds()->SetNumberOfIds(currentPath->Size());

    Point3D currentPoint;
    Point3D currentWorldPoint;

    for (unsigned int j = 0; j < currentPath->Size(); j++)
    {
      currentPoint[0] = currentPath->ElementAt(j)[0];
      currentPoint[1] = currentPath->ElementAt(j)[1];
      currentPoint[2] = 0;

      m_SliceGeometry->IndexToWorld(currentPoint, currentWorldPoint);

      points->InsertPoint(pointId, currentWorldPoint[0], currentWorldPoint[1], currentWorldPoint[2]);
      polygon->GetPointIds()->SetId(j, pointId);
      pointId++;

    } // for2

    polygons->InsertNextCell(polygon);
  } // for1

  contourSurface->SetPoints(points);
  contourSurface->SetPolys(polygons);
  contourSurface->BuildLinks();
  Surface::Pointer finalSurface = this->GetOutput();

  finalSurface->SetVtkPolyData(contourSurface);
 }

void mitk::ImageToContourFilter::GenerateOutputInformation()
{
  Superclass::GenerateOutputInformation();
}

void mitk::ImageToContourFilter::SetUseProgressBar(bool status)
{
  this->m_UseProgressBar = status;
}

void mitk::ImageToContourFilter::SetProgressStepSize(unsigned int stepSize)
{
  this->m_ProgressStepSize = stepSize;
}

void mitk::ImageToContourFilter::SetContourValue(float value)
{
  this->m_ContourValue = value;
}