/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "Progress.h"
#include "Test1.h"
#include <Windows.h>

REGISTER_PLUGIN_BASIC(OpticksTutorial, Tutorial1);

Tutorial1::Tutorial1()
{
   setDescriptorId("{5D8F4DD0-9B20-42B1-A060-589DFBC85D00}");
   setName("set using setName()");
   setDescription("set using setDescription()");
   setCreator("set using setCreator()");
   setVersion("set using Version()");
   setCopyright("set using setCopyRight()");
   setProductionStatus(false);
   setType("set using setType()s");
   setMenuLocation("[Set Extension Name Here]/Is this 1?");
   setAbortSupported(false);
}

Tutorial1::~Tutorial1()
{
}

bool Tutorial1::getInputSpecification(PlugInArgList*& pInArgList)
{
   VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
   pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, "Progress reporter");
   return true;
}

bool Tutorial1::getOutputSpecification(PlugInArgList*& pOutArgList)
{
   pOutArgList = NULL;
   return true;
}

bool Tutorial1::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   if (pInArgList == NULL)
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(Executable::ProgressArg());
   if (pProgress != NULL)
   {
	  //int i = 100;
	  for (int i=0; i<=100; i++)
	  {
		  pProgress->updateProgress("Hello World! Says Vijesh...", i, NORMAL);
		  pProgress->updateProgress("This demonstrates display of a warning.", i, WARNING);
		  if (i%25==0)
		  {
			  pProgress->updateProgress("This demonstrates display of an error.", i, ERRORS);
		  }
		  Sleep(100);
	  }
   }
   return true;
}
