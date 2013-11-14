/******************************************************************************
 * CVAC Software Disclaimer
 * 
 * This software was developed at the Naval Postgraduate School, Monterey, CA,
 * by employees of the Federal Government in the course of their official duties.
 * Pursuant to title 17 Section 105 of the United States Code this software
 * is not subject to copyright protection and is in the public domain. It is 
 * an experimental system.  The Naval Postgraduate School assumes no
 * responsibility whatsoever for its use by other parties, and makes
 * no guarantees, expressed or implied, about its quality, reliability, 
 * or any other characteristic.
 * We would appreciate acknowledgement and a brief notification if the software
 * is used.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above notice,
 *       this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Naval Postgraduate School, nor the name of
 *       the U.S. Government, nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE NAVAL POSTGRADUATE SCHOOL (NPS) AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NPS OR THE U.S. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
#include "DPMDetectionI.h"
#include <iostream>
#include <vector>

#include <Ice/Communicator.h>
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <util/processRunSet.h>
#include <util/FileUtils.h>
#include <util/DetectorDataArchive.h>
#include <util/ServiceManI.h>

using namespace Ice;
using namespace cvac;


///////////////////////////////////////////////////////////////////////////////
// This is called by IceBox to get the service to communicate with.
extern "C"
{
  /**
   * Create the detector service via a ServiceManager.  The 
   * ServiceManager handles all the icebox interactions.  Pass the constructed
   * detector instance to the ServiceManager.  The ServiceManager obtains the
   * service name from the config.icebox file as follows. Given this
   * entry:
   * IceBox.Service.BOW_Detector=bowICEServer:create --Ice.Config=config.service
   * ... the name of the service is BOW_Detector.
   */
  ICE_DECLSPEC_EXPORT IceBox::Service* create(CommunicatorPtr communicator)
  {
    DPMDetectionI *dpm = new DPMDetectionI();
    ServiceManagerI *sMan = new ServiceManagerI( dpm, dpm );
    dpm->setServiceManager( sMan );
    return sMan;
  }
}


///////////////////////////////////////////////////////////////////////////////

DPMDetectionI::DPMDetectionI()
  : mServiceMan( NULL )
  , gotModel( false )
{
}

DPMDetectionI::~DPMDetectionI()
{
}

void DPMDetectionI::setServiceManager( ServiceManager* sman )
{
    mServiceMan = sman;
}

void DPMDetectionI::starting()
{
  // check if the config.service file contains a trained model; if so, read it.
  string modelfile = mServiceMan->getModelFileFromConfig();

  if (modelfile.empty())
  {
    localAndClientMsg(VLogger::DEBUG, NULL, "No trained model file specified in service config.\n" );
  }
  else
  {
    localAndClientMsg(VLogger::DEBUG, NULL, "Will read trained model file as specified in service config: %s\n",
                      modelfile.c_str());
    gotModel = readModelFile( modelfile );
    if (!gotModel)
    {
      localAndClientMsg(VLogger::WARN, NULL, "Failed to read pre-configured trained model "
                        "from: %s; will continue but now require client to send trained model\n",
                        modelfile.c_str());
    }
  }
}  

/** try to read the trained model from the specified file.  The 
 * path must be a file system path, not a CVAC path.
 */
bool DPMDetectionI::readModelFile( const string& archivefile )
{
  if( !fileExists(archivefile))
  {
    localAndClientMsg(VLogger::ERROR, NULL, "Failed to initialize because the trained model "
                      "cannot be found: %s\n", archivefile.c_str());
    return false;
  }

  // get sandbox
  string connectName = "TODO_fixme"; //getClientConnectionName(current);
  string serviceName = mServiceMan->getServiceName();
  string clientName = mServiceMan->getSandbox()->createClientName(serviceName,
                                                                  connectName);                           
  string clientDir = mServiceMan->getSandbox()->createClientDir(clientName);

  // unarchive model file
  DetectorDataArchive dda;
  dda.unarchive( archivefile, clientDir );
  // This detector only needs an XML file
  string modelXML = dda.getFile( XMLID );
  if (modelXML.empty())
  {
      localAndClientMsg(VLogger::ERROR, NULL,
                        "Could not find XML result file in zip file.\n");
      return false;
  }
  // Get only the filename part
  //modelXML = getFileName(modelXML);  TODO: why???  it doesn't work without the path. matz.
  
  boost::scoped_ptr<Model> fsmodel;   
  fsmodel.reset(new FileStorageModel);                
  if(!fsmodel->deserialize( modelXML )) 
  {
      localAndClientMsg(VLogger::ERROR, NULL, "Failed to initialize because the file %s has a problem\n",
                        modelXML.c_str());
      return false;
  }

  pbd.distributeModel(*fsmodel);

  return true;
}
      
bool DPMDetectionI::initialize(const DetectorProperties& props, const FilePath& trainedModel,
                               const Current& current)
{
    std::string expandedSubfolder;
    std::string archiveFilePath;
    std::vector<std::string> fileNameforUsageOrders;
    std::string fileNameToBeUsed;

    // todo: initialize props

    // a model presented during the "process" call takes precedence
    //  over one specified in a config file
    if(trainedModel.filename.empty())
    {
      if (!gotModel)
      {
        localAndClientMsg(VLogger::ERROR, NULL, "No trained model available, aborting.\n" );
        return false;
      }
      // ok, go on with pre-configured model
    }
    else
    {
      string modelfile = getFSPath( trainedModel, mServiceMan->getDataDir() );
      gotModel = readModelFile( modelfile );
      if (!gotModel)
      {
        localAndClientMsg(VLogger::ERROR, NULL, "Failed to initialize because explicitly specified trained model "
                          "cannot be found or loaded: %s\n", modelfile.c_str());
        return false;
      }
    }

    return true;
}

std::string DPMDetectionI::getName(const Current& current)
{
  return mServiceMan->getServiceName();
}

std::string DPMDetectionI::getDescription(const Current& current)
{
        return "Deformable Parts Model";
}

DetectorProperties DPMDetectionI::getDetectorProperties(const Current& current)
{       
        return props;
}

ResultSet DPMDetectionI::processSingleImg(DetectorPtr detector,const char* fullfilename)
{       
        ResultSet _resSet;      

        // Detail the current file being processed (DEBUG_1)
        std::string _ffullname = std::string(fullfilename);

        localAndClientMsg(VLogger::DEBUG_1, NULL, "==========================\n");
        localAndClientMsg(VLogger::DEBUG_1, NULL, "%s is processing.\n", _ffullname.c_str());
        DPMDetectionI* _pDPM = static_cast<DPMDetectionI*>(detector.get());

        cv::Mat _depth;
        cv::Mat _img = cv::imread(_ffullname.c_str());
        if (_img.empty())
        {
                localAndClientMsg(VLogger::WARN, NULL, "Failed to detect because the file %s does not exist\n",_ffullname.c_str());
                return _resSet;
        }
        std::vector<Candidate> _candidates;     
        float confidence = 0.0f;
        _pDPM->pbd.detect(_img, _depth, _candidates);


        localAndClientMsg(VLogger::DEBUG_1, NULL, "# of Candidates for %s: %d\n", _ffullname.c_str(), _candidates.size());

        bool result = ((_candidates.size()>0)?true:false);      //this threshold can be adjusted for obtaining a strict result
        string result_statement = (result)?"Positive":"Negative";
        localAndClientMsg(VLogger::DEBUG_1, NULL, "Detection, %s as Class: %s\n", _ffullname.c_str(),result_statement.c_str());


        Result _tResult;
        _tResult.original = NULL;       //it would be updated in the function processRunSet
        Labelable *labelable = new Labelable();
        labelable->confidence = confidence;
        labelable->lab.hasLabel = true;
        labelable->lab.name = result_statement; 

        _tResult.foundLabels.push_back(labelable);
        _resSet.results.push_back(_tResult);
        
        return _resSet;
}

void DPMDetectionI::process(const Identity &client,const RunSet& runset,
                            const FilePath& model, const DetectorProperties& props,
                            const Current& current)
{
  DetectorCallbackHandlerPrx _callback = DetectorCallbackHandlerPrx::uncheckedCast(
      current.con->createProxy(client)->ice_oneway());
  DoDetectFunc func = DPMDetectionI::processSingleImg;

  bool initialized = this->initialize( props, model, current );
  if (!initialized)
  {
    localAndClientMsg(VLogger::ERROR, NULL, "Detector not initialized, not processing.\n");
    return;
  }
  
  try {
    processRunSet(this, _callback, func, runset, mServiceMan->getDataDir(), mServiceMan);
  }
  catch (exception e) {
    localAndClientMsg(VLogger::ERROR, NULL, "Detector could not process given file-path.\n");
  }
}

bool DPMDetectionI::cancel(const Identity &client, const Current& current)
{
  localAndClientMsg(VLogger::WARN, NULL, "cancel not implemented.");
  return false;
}
