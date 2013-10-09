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

using namespace Ice;
using namespace cvac;


///////////////////////////////////////////////////////////////////////////////
// This is called by IceBox to get the service to communicate with.
extern "C"
{
  /**
   * Create the detector service via a ServiceManager.  The 
   * ServiceManager handles all the icebox interactions.  Pass the constructed
   * detector instance as well as the service name to the ServiceManager.
   * The name is obtained from the config.icebox file as follows. Given this
   * entry:
   * IceBox.Service.BOW_Detector=bowICEServer:create --Ice.Config=config.service
   * ... the name of the service is BOW_Detector.
   */
  ICE_DECLSPEC_EXPORT IceBox::Service* create(CommunicatorPtr communicator)
  {
    // TODO: get name from icebox.config file via the "start" function in ServiceMan
    ServiceManager *sMan = new ServiceManager();
    DPMDetectionI *dpm = new DPMDetectionI(sMan);    
    sMan->setService(dpm, dpm->getName());
    return (::IceBox::Service*) sMan->getIceService();
  }
}


///////////////////////////////////////////////////////////////////////////////

DPMDetectionI::DPMDetectionI(ServiceManager *sman)
{
    mServiceMan = sman;
}

DPMDetectionI::~DPMDetectionI()
{
}

bool DPMDetectionI::checkExistenceDetectorData()
{    
  std::string tFilePath = getPathDetectorData();
    if(fileExists(tFilePath))
      return true;
    else
      return false;
}

std::string DPMDetectionI::getPathDetectorData()
{
    std::string tSearchStr = getName() + ".DetectorData"; //"DPM_ShipDetector_v1": this name should be changed manually in this file.
    PropertiesPtr props = Application::communicator()->getProperties();
    return props->getProperty(tSearchStr); //Should it be an absolute? 
}
                          
bool DPMDetectionI::initialize(const DetectorProperties& props, const FilePath& trainedModel,
                               const Current& current)
{
    std::string expandedSubfolder;
    std::string archiveFilePath;
    std::vector<std::string> fileNameforUsageOrders;
    std::string fileNameToBeUsed;

    // todo: initialize props

    if(trainedModel.filename.empty())  //if there is no specific detectorData
    {
      if(checkExistenceDetectorData())
      {
        archiveFilePath = getPathDetectorData();
        expandedSubfolder = getFileDirectory(archiveFilePath) + "_";  
        // TODO        fileNameforUsageOrders = expandSeq_fromFile(archiveFilePath, expandedSubfolder);
        fileNameToBeUsed = fileNameforUsageOrders[0];  //only the first xml file will be used
      }
      else
      {
        localAndClientMsg(VLogger::ERROR, NULL, "Failed to initialize because there is no detector data\n");
        return false;
      }
    }
    else  //if detectorData is not empty
    {
      // Get the default CVAC data directory as defined in the config file
      m_CVAC_DataDir = mServiceMan->getDataDir();

      // Use utils un-compression to get zip file names
      // Filepath is relative to 'CVAC_DataDir'     
      if ( (trainedModel.directory.relativePath.length() > 1 
        && trainedModel.directory.relativePath[1] == ':' )||
        trainedModel.directory.relativePath[0] == '/' ||
        trainedModel.directory.relativePath[0] == '\\')
      {  // absolute path
        // TODO: don't permit absolute paths!  get rid of if/else and just
        // check in getFSPath that it's relative
        archiveFilePath = trainedModel.directory.relativePath + "/" + trainedModel.filename;
      } 
      else
      { // prepend our prefix, this routine is copied from getFSPath function in BowICEI.cpp
        if (trainedModel.directory.relativePath.empty())
          archiveFilePath = m_CVAC_DataDir+"/"+trainedModel.filename;
        else
          archiveFilePath = m_CVAC_DataDir+"/"+trainedModel.directory.relativePath+"/"+trainedModel.filename;
      }

      expandedSubfolder = archiveFilePath + "_";  //getName(current)
      // TODO      fileNameforUsageOrders = expandSeq_fromFile(archiveFilePath, expandedSubfolder);
      fileNameToBeUsed = fileNameforUsageOrders[0];  //only the first xml file will be used          
    }

    boost::scoped_ptr<Model> fsmodel;	
    fsmodel.reset(new FileStorageModel);		
    std::string tStrXML = expandedSubfolder+ "/" + fileNameToBeUsed;
    if(!fsmodel->deserialize(tStrXML)) 
    {
      localAndClientMsg(VLogger::ERROR, NULL, "Failed to initialize because the file %s has a problem\n",
                        tStrXML.c_str());
      return false;
    }

    pbd.distributeModel(*fsmodel);
    return true;
}



std::string DPMDetectionI::getName(const Current& current)
{
  return getName();
}

std::string DPMDetectionI::getName()
{
	return "DPM_Detector";
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

	bool result = ((_candidates.size()>0)?true:false);	//this threshold can be adjusted for obtaining a strict result
	string result_statement = (result)?"Positive":"Negative";
	localAndClientMsg(VLogger::DEBUG_1, NULL, "Detection, %s as Class: %s\n", _ffullname.c_str(),result_statement.c_str());


	Result _tResult;
	_tResult.original = NULL;	//it would be updated in the function processRunSet
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
    localAndClientMsg(VLogger::ERROR, NULL, "Detector not initialized, not processing.");
    return;
  }
  
  try {
    processRunSet(this, _callback, func, runset, m_CVAC_DataDir, mServiceMan);
  }
  catch (exception e) {
    localAndClientMsg(VLogger::ERROR, NULL, "$$$ Detector could not process given file-path.");
  }
}

bool DPMDetectionI::cancel(const Identity &client, const Current& current)
{
  localAndClientMsg(VLogger::WARN, NULL, "cancel not implemented.");
  return false;
}
