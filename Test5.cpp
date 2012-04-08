/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DesktopServices.h"
#include "MessageLogResource.h"
#include "ObjectResource.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "Progress.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterUtilities.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "switchOnEncoding.h"
#include "Test5.h"
#include <limits>

REGISTER_PLUGIN_BASIC(OpticksTutorial, Tutorial5); //didnt understand this..

namespace
{
   template<typename T>
   void edgeDetection(T* pData, DataAccessor pSrcAcc, int row, int col, int rowSize, int colSize) //templated function. reason: switchOnEncoding can be set to its type. It introduces more flexibility into the code.
   {
		//This function is applied for a particular pixel.
	   //data accessor objects do not check for bounds while processing. Hence, they may throw exceptions. The next set of statements provide the bounds for 
      int prevCol = std::max(col - 1, 0);
      int prevRow = std::max(row - 1, 0);
      int nextCol = std::min(col + 1, colSize - 1);
      int nextRow = std::min(row + 1, rowSize - 1);
      
	  //access the pixel values at ALL the positions in the window.
	  //The values are stored in pSrcAcc in an internal data structure. Am I right?
	  //If so, the method getColumn() extracts and delivers the value, in the third statement.
	  //ensure that the pixel value read is valid. (how? bounds of 0-255 on an 8-bit image?)

      pSrcAcc->toPixel(prevRow, prevCol);
      VERIFYNRV(pSrcAcc.isValid());
      T upperLeftVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(prevRow, col);
      VERIFYNRV(pSrcAcc.isValid());
      T upVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(prevRow, nextCol);
      VERIFYNRV(pSrcAcc.isValid());
      T upperRightVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(row, prevCol);
      VERIFYNRV(pSrcAcc.isValid());
      T leftVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(row, nextCol);
      VERIFYNRV(pSrcAcc.isValid());
      T rightVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(nextRow, prevCol);
      VERIFYNRV(pSrcAcc.isValid());
      T lowerLeftVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(nextRow, col);
      VERIFYNRV(pSrcAcc.isValid());
      T downVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

      pSrcAcc->toPixel(nextRow, nextCol);
      VERIFYNRV(pSrcAcc.isValid());
      T lowerRightVal = *reinterpret_cast<T*>(pSrcAcc->getColumn());

	  //calculate the weighted average along the y-axis / rows
      double gx = -1.0 * upperLeftVal + -2.0 * leftVal + -1.0 * lowerLeftVal + 1.0 * upperRightVal + 2.0 *
         rightVal + 1.0 * lowerRightVal;

	  //calculate the weighted average along the x-axis / columns
      double gy = -1.0 * lowerLeftVal + -2.0 * downVal + -1.0 * lowerRightVal + 1.0 * upperLeftVal + 2.0 *
         upVal + 1.0 * upperRightVal;

	  //These two components are perpendicular to one another. Hence, magnitude of their vector addition leads to:
      double magnitude = sqrt(gx * gx + gy * gy);

	  //pData is a pointer a type T. Note that T can be an integer, float, double etc.
      *pData = static_cast<T>(magnitude);
   }
};

Tutorial5::Tutorial5() //"The usual"
{
   setDescriptorId("{BE00BBC3-A1E3-4b0d-8780-1B5D9A8620CC}");
   setName("Tutorial 5");
   setVersion("Sample");
   setDescription("Calculate and return an edge detection raster element for first band "
      "of the provided raster element.");
   setCreator("Opticks Community");
   setCopyright("Copyright (C) 2008, Ball Aerospace & Technologies Corp.");
   setProductionStatus(false);
   setType("Sample");
   setSubtype("Edge Detection");
   setMenuLocation("[Tutorial]/Tutorial 5");
   setAbortSupported(true);
}

Tutorial5::~Tutorial5()//"The usual"
{
}

bool Tutorial5::getInputSpecification(PlugInArgList*& pInArgList)
{
   VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
   pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, "Progress reporter"); //we need a progress bar for this to indicate the percentage of completion
   pInArgList->addArg<RasterElement>(Executable::DataElementArg(), "Perform edge detection on this data element"); // since we're applying an edge detection algorithm, we need to add the raster element to the input argument list.
   return true;
}

bool Tutorial5::getOutputSpecification(PlugInArgList*& pOutArgList)
{
   VERIFY(pOutArgList = Service<PlugInManagerServices>()->getPlugInArgList());
   pOutArgList->addArg<RasterElement>("Result", NULL); //output is also a raster element. it is referenced using "Result" and initialized to null.
   return true;
}

bool Tutorial5::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   StepResource pStep("Tutorial 5", "app", "5EA0CC75-9E0B-4c3d-BA23-6DB7157BBD54");
   if (pInArgList == NULL || pOutArgList == NULL) //"The usual"
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(Executable::ProgressArg()); //initialize the progress panel
   RasterElement* pCube = pInArgList->getPlugInArgValue<RasterElement>(Executable::DataElementArg());  //pCube is the Rastering element. It is finally used to create a data accessor element and a data desriptor element.
   if (pCube == NULL) //if no image data is open
   {
      std::string msg = "A raster cube must be specified.";
      pStep->finalize(Message::Failure, msg);
      if (pProgress != NULL) 
      {
         pProgress->updateProgress(msg, 0, ERRORS);
      }
      return false;
   }
   RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pCube->getDataDescriptor());
   VERIFY(pDesc != NULL); //ensure that this data isnt NULL. If it is, an appropriate error is logged and the code is exited.

   if (pDesc->getDataType() == INT4SCOMPLEX || pDesc->getDataType() == FLT8COMPLEX)
	   //these are the two available complex types. But what exactly is a complex type in the first place?
   {	//log the appropriate message on the progress bar and exit.
      std::string msg = "Edge detection cannot be performed on complex types.";
      pStep->finalize(Message::Failure, msg);
      if (pProgress != NULL) 
      {
         pProgress->updateProgress(msg, 0, ERRORS);
      }
      return false;
   }

   FactoryResource<DataRequest> pRequest; //same as #4
   pRequest->setInterleaveFormat(BSQ);
   DataAccessor pSrcAcc = pCube->getDataAccessor(pRequest.release());

   ModelResource<RasterElement> pResultCube(RasterUtilities::createRasterElement(pCube->getName() +
      "_Edge_Detection_Result", pDesc->getRowCount(), pDesc->getColumnCount(), pDesc->getDataType())); //I didnt quite get the meaning of this.
   if (pResultCube.get() == NULL)
   {
      std::string msg = "A raster cube could not be created.";
      pStep->finalize(Message::Failure, msg);
      if (pProgress != NULL) 
      {
         pProgress->updateProgress(msg, 0, ERRORS);
      }
      return false;
   }
   FactoryResource<DataRequest> pResultRequest;
   pResultRequest->setWritable(true); //this setting allows us to change the underlying data that is obtained via the data accessor. This allows us to store the edge detection result.
   DataAccessor pDestAcc = pResultCube->getDataAccessor(pResultRequest.release());

   for (unsigned int row = 0; row < pDesc->getRowCount(); ++row) //traverse row-wise
   {
      if (pProgress != NULL) //update the progress bar with the fraction of rows computed already.
      {
         pProgress->updateProgress("Calculating result", row * 100 / pDesc->getRowCount(), NORMAL);
      }

	  //handle the two errors that are likely to occur.
      if (isAborted())
      {
         std::string msg = getName() + " has been aborted.";
         pStep->finalize(Message::Abort, msg);
         if (pProgress != NULL)
         {
            pProgress->updateProgress(msg, 0, ABORT);
         }
         return false;
      }

      if (!pDestAcc.isValid())
      {
         std::string msg = "Unable to access the cube data.";
         pStep->finalize(Message::Failure, msg);
         if (pProgress != NULL) 
         {
            pProgress->updateProgress(msg, 0, ERRORS);
         }
         return false;
      }
      for (unsigned int col = 0; col < pDesc->getColumnCount(); ++col)
      {
         switchOnEncoding(pDesc->getDataType(), edgeDetection, pDestAcc->getColumn(), pSrcAcc, row, col,
            pDesc->getRowCount(), pDesc->getColumnCount()); //This internally calls the edgeDetection function defined in the namespace with the following arguments as parameters (in their respective order)
         pDestAcc->nextColumn();
      }

      pDestAcc->nextRow();
   }

   if (!isBatch()) //If its not processed in batch. But I didnt exactly understand what batch processing means.
   {
      Service<DesktopServices> pDesktop; //invoke desktop services,

      SpatialDataWindow* pWindow = static_cast<SpatialDataWindow*>(pDesktop->createWindow(pResultCube->getName(),
         SPATIAL_DATA_WINDOW)); //SpatialDataWindow creates a new window inside opticks. for this, pDesktop->createWindow is called with the arguments - window Name, type.

      SpatialDataView* pView = (pWindow == NULL) ? NULL : pWindow->getSpatialDataView(); //what exactly is a SpatialDataView? Is it the rasterized version?
      if (pView == NULL)
      {
         std::string msg = "Unable to create view.";
         pStep->finalize(Message::Failure, msg);
         if (pProgress != NULL) 
         {
            pProgress->updateProgress(msg, 0, ERRORS);
         }
         return false;
      }

      pView->setPrimaryRasterElement(pResultCube.get()); //didnt get as to why these have to be done.
      pView->createLayer(RASTER, pResultCube.get());
   }

   if (pProgress != NULL)
   {
      pProgress->updateProgress("Tutorial5 is compete.", 100, NORMAL);
   }

   //pOutArgList->setPlugInArgValue("Tutorial5_Result", pResultCube.release()); //This is used to set the raster element as the output argument. 
   //Commenting this line should result in the output not being displayed. Right? Yes.

   pStep->finalize();
   return true;
}
