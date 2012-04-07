/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppConfig.h"
#include "AppVerify.h"
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "MessageLogResource.h"
#include "ObjectResource.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "PlugInResource.h"
#include "Progress.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "StringUtilities.h"
#include "switchOnEncoding.h"
#include "Test3.h"
#include <limits>

REGISTER_PLUGIN_BASIC(OpticksTutorial, Tutorial3); //why is this required?

namespace 
{
	//defnining a template which updates the min, max and the total by comparison with the data of typename T
   template<typename T>
   void updateStatistics(T* pData, double& min, double& max, double& total)
   {
      min = std::min(min, static_cast<double>(*pData));
      max = std::max(max, static_cast<double>(*pData));
      total += *pData;
   }
};

Tutorial3::Tutorial3() //The semantics of this method is the same as the previous ones.
{
   setDescriptorId("{2073076C-2676-45B9-AA7B-A2607104655C}");
   setName("Tutorial 3");
   setDescription("Accessing cube data.");
   setCreator("Opticks Community");
   setVersion("Sample");
   setCopyright("Copyright (C) 2008, Ball Aerospace & Technologies Corp.");
   setProductionStatus(false);
   setType("Sample");
   setSubtype("Statistics");
   setMenuLocation("[Tutorial]/Tutorial 3");
   setAbortSupported(true);
}

Tutorial3::~Tutorial3()
{
}

bool Tutorial3::getInputSpecification(PlugInArgList* &pInArgList)
{
   VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList()); //verify that the plugins exist and get the argument list.
   pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, "Progress reporter"); //to the current input argument list, add the Progress bar argument.
   pInArgList->addArg<RasterElement>(Executable::DataElementArg(), "Generate statistics for this raster element"); //Add a data element argument also.. so that the data (in terms of pixel values) can be extracted from the raster element. Hence, this is of the typename "RasterElement"
   return true;
}

bool Tutorial3::getOutputSpecification(PlugInArgList*& pOutArgList) //so far, this was null. But now, the plugin ultimately has to output some form of data.
{
   VERIFY(pOutArgList = Service<PlugInManagerServices>()->getPlugInArgList());
   //Append the list of arguments that are supposed to be output by the plugin. This can also be used for chaining the plugins. i.e, the output of one plugin can be given as the input to another.
   pOutArgList->addArg<double>("Minimum", "The minimum value"); //template TypeName - int, float, double, long etc.. syntax: name, description
   pOutArgList->addArg<double>("Maximum", "The maximum value");
   pOutArgList->addArg<unsigned int>("Count", "The number of pixels");
   pOutArgList->addArg<double>("Mean", "The average value");
   return true;
}

bool Tutorial3::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   StepResource pStep("Tutorial 3", "app", "27170298-10CE-4E6C-AD7A-97E8058C29FF");
   if (pInArgList == NULL || pOutArgList == NULL) //check for both values, since this plugin has both input and the output.
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(Executable::ProgressArg()); //to obtain a progress bar interface.
   RasterElement* pCube = pInArgList->getPlugInArgValue<RasterElement> (Executable::DataElementArg());
   //pcube is a pointer to the raster element. i.e, pcube can be used to access the spatial data. For this to work, a dataelement must be initially open. In simple terms, an image must be open to work on. This data is accessible through 'Executable::DataElementArg()'
   if (pCube == NULL) //if no data (image) is open, then pCube will have a null value.
   {
      std::string msg = "A raster cube must be specified."; 
	  //display an error message on the progress bar and finalize it appropriately.
      pStep->finalize(Message::Failure, msg);
      if (pProgress != NULL)
      {
         pProgress->updateProgress(msg, 0, ERRORS);
      }

      return false;
   }
   //A data element is analogous to an open image data.
   //DataElement has a DataDescriptor, RasterElement has a RastorDataDescriptor

   RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pCube->getDataDescriptor()); //RastorElement always has a RastorDataDescriptor. so static casting is safe.
   VERIFY(pDesc != NULL);

   FactoryResource<DataRequest> pRequest; //this resource is a datarequest resouce. pRequest can henceforth be used to access the data elements. i.e, it sets the rows, columns and bands that need to be accessed at a particular time. This is more of an optimization criteria.
   pRequest->setInterleaveFormat(BSQ); //Didnt understand what an interleaving format is, and why it is being set to BSQ.

   DataAccessor pAcc = pCube->getDataAccessor(pRequest.release()); //pAcc is used to access the raster data. Its value is got from the pointer to the raster element (pCube). And the mode of the request is set to release. I clearly didnt understand what removal of ownership of underlying obect meant.

   //dealing with a single band of raster data.
   double min = std::numeric_limits<double>::max(); //this is done so as remove make the code independent of the underlying platform. Here, min is being set to global max and max is being set to global numerical min (-ve max).
   double max = -min;
   double total = 0.0;
   for (unsigned int row = 0; row < pDesc->getRowCount(); ++row)
   {

	   //OK, heres the idea. first check for all the errors and return false appropriately. THEN, if everything is right, go for the computation!

      if (isAborted()) //on every loop, check whether the status is aborted. But isnt there any better interrupt-oriented way to achieve this instead of polling?
		  //maybe user-cliked cancel or other plugin aborts this plugin.
      {
         std::string msg = getName() + " has been aborted."; //display message on the Progress tab and finalize. This is same as the previous modules.
         pStep->finalize(Message::Abort, msg);
         if (pProgress != NULL)
         {
            pProgress->updateProgress(msg, 0, ABORT);
         }

         return false;
      }

      if (!pAcc.isValid()) //if the accessed cube data is valid.
      {
         std::string msg = "Unable to access the cube data."; //same as previous
         pStep->finalize(Message::Failure, msg);
         if (pProgress != NULL)
         {
            pProgress->updateProgress(msg, 0, ERRORS);
         }

         return false;
      }


	  //everything is OK. now, continue with the core logic of the plugin.
      if (pProgress != NULL)
      {
         pProgress->updateProgress("Calculating statistics", row * 100 / pDesc->getRowCount(), NORMAL); 
		 //pointToNote - the progress is based on the number of rows already processed. NOT the total number of pixels processed.
		 //continuosly get the percentage of rows processed and keep updating the progress bar.
      }

	  //process columnwise
      for (unsigned int col = 0; col < pDesc->getColumnCount(); ++col)
      {
		  //casts the column data obtained into the a type specified by the first arg.
		  //switchOnEncoding semantcis: tpye, var_args(functor, data, values to operate on)
		  //updateStatistics will update the min, max and the total. DOUBT: shouldnt we pass a reference or a pointer to this function?
         switchOnEncoding(pDesc->getDataType(), updateStatistics, pAcc->getColumn(), min, max, total);
         pAcc->nextColumn(); //make the internal pointer to proceed to the next column. next time the function pAcc->getColumn() is called, the value from the next column data is returned. similar logic for rows.
      }
      pAcc->nextRow();
   }
   unsigned int count = pDesc->getColumnCount() * pDesc->getRowCount(); //total number of pixels.
   double mean = total / count;

   if (pProgress != NULL)
   {
      std::string msg = "Minimum value: " + StringUtilities::toDisplayString(min) + "\n"
                      + "Maximum value: " + StringUtilities::toDisplayString(max) + "\n"
                      + "Number of pixels: " + StringUtilities::toDisplayString(count) + "\n"
                      + "Average: " + StringUtilities::toDisplayString(mean);
      pProgress->updateProgress(msg, 100, NORMAL);
   }

   pStep->addProperty("Minimum", min);
   pStep->addProperty("Maximum", max);
   pStep->addProperty("Count", count);
   pStep->addProperty("Mean", mean);
   
   pOutArgList->setPlugInArgValue("Minimum", &min);
   pOutArgList->setPlugInArgValue("Maximum", &max);
   pOutArgList->setPlugInArgValue("Count", &count);
   pOutArgList->setPlugInArgValue("Mean", &mean);

   pStep->finalize();
   return true;
}
