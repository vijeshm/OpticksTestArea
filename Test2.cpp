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
#include "MessageLogResource.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "PlugInResource.h"
#include "Progress.h"
#include "StringUtilities.h"
#include "Test2.h"
#ifdef WIN_API
#include <windows.h>
#else
#include <unistd.h>
#endif

//This entire plugin itself is called recursively!

REGISTER_PLUGIN_BASIC(OpticksTutorial, Tutorial2);

Tutorial2::Tutorial2()
{
   setDescriptorId("{1F6316EB-8A03-4034-8B3D-31FADB7AF727}"); //two plugins cannot have same IDs. This is used to uniquely identify the plugin
   setName("Tutorial 2"); //appears on the prompt-like screen
   setDescription("Using resources and services."); //describes the plugin
   setCreator("Opticks Community");
   setVersion("Sample");
   setCopyright("Copyright (C) 2008, Ball Aerospace & Technologies Corp.");
   setProductionStatus(false); //"Not for production use"
   setType("Sample");
   setMenuLocation("[Tutorial]/Tutorial 2"); //modifying this would change the way the submenus are displayed on opticks. This gives a hierarchical submenu structure
   setAbortSupported(false);
   allowMultipleInstances(true); //default: false. Setting this to true allows the plugin to call itself recursively. And ofcourse, the plugin should know the depth / state of the recursion. Hence, they must be passed on as input parameters.
}

Tutorial2::~Tutorial2()
{
}

bool Tutorial2::getInputSpecification(PlugInArgList*& pInArgList)
{
   VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
   pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, "Progress reporter"); //semantics of the parameters: name, value, description
   pInArgList->addArg<int>("Count", 9, "How many times should the plug-in recurse?");
   pInArgList->addArg<int>("Depth", 1, "This is the recursive depth. Not usually set by the initial caller.");
   return true;
}

bool Tutorial2::getOutputSpecification(PlugInArgList*& pOutArgList)
{
   pOutArgList = NULL;
   return true;
}

bool Tutorial2::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   StepResource pStep("Tutorial 2", "app", "A8FEFCB3-5D08-4670-B47E-CC533A932737"); //this info is also used by the message log window in order to identify the plugin from which it was logged. semantics: description, { component(name), key } - unique ID
   if (pInArgList == NULL)
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(Executable::ProgressArg());
   int count;
   int depth;
   pInArgList->getPlugInArgValue("Count", count); //read from the input arguments into a local variable.
   pInArgList->getPlugInArgValue("Depth", depth);
   if (count < 1 || count >= 10 || depth < 1 || depth >= 10)
   {
      std::string msg = "Count and depth must be between 1 and 9 inclusive."; //error message
      pStep->finalize(Message::Failure, msg); //finalize the step by saying that the message is of Type Failure and printing the message.
      if (pProgress != NULL)
      {
         pProgress->updateProgress(msg, 0, ERRORS); //update the progrss window with the error message.
      }

      return false; //returns false since the app failed to execute
   }
   if (depth > count)
   {
      std::string msg = "Depth must be <= count"; //this seems perfectly logical. the depth can never be greater than count. Its either lesser or equal.
      pStep->finalize(Message::Failure, msg); //In such a case, log this error and finalize the code.
      if (pProgress != NULL) //"IF" is there is a progress message. This implies all plugins need not have error messages.
		  // The logical part of this code is the same as the above segment, except for the logging message.
      {
         pProgress->updateProgress(msg, 0, ERRORS);
      }

      return false;
   }
   pStep->addProperty("Depth", depth); //What does this step do??

   if (pProgress != NULL) //IF there is a progress bar defined, then update it to indicate the current value of "count" and "depth"
   {
	   if (depth == 5) //At the fifth recursive call, try to just display a test message. It'll appear for just a fraction of a second. so, go for _sleep() method.
	   {
		   pProgress->updateProgress("Whats up Bob?", depth * 100 / count, NORMAL);
		   _sleep(1000);
	   }
		
      pProgress->updateProgress("Tutorial 2: Count " + StringUtilities::toDisplayString(count)
         + " Depth " + StringUtilities::toDisplayString(depth), depth * 100 / count, NORMAL);
   }
   // sleep to simulate some work - This is used to support multiplatform (windows / linux) features for sleep.
#ifdef WIN_API
   _sleep(1000);
#else
   sleep(1);
#endif

   if (depth < count)
   {
      ExecutableResource pSubCall("Tutorial 2", std::string(), pProgress, false); //a type of plugin resource used to access the executable interface.. semantics?? plugin-name, menu command, progress bar reference, batch.... this creates a binding between the plugin names and the resources.
      VERIFY(pSubCall->getPlugIn() != NULL); //getPlugIn returns the pointer to the plugin object. we're using recursion here. Hence, this can never be null. However, in other cases, this can return a null value if the plugin doesnt exist.
      pSubCall->getInArgList().setPlugInArgValue("Count", &count); //this is for preparing for the next recursive call. Here, we're changing the input argument list. 

	  //Hmmm.. I wonder if this plugin would still work if the above line was commented. We arent changing the count variable anyway. - Yes, it does.

      pSubCall->getInArgList().setPlugInArgValue("Depth", &++depth); //increment the depth at every stage.
      if (!pSubCall->execute()) //recursive call.
      {
         // sub-call has already posted an error message
         return false;
      }
   }

   pStep->finalize(); //called after the step has completed its functionality of logging.
   return true;
}
