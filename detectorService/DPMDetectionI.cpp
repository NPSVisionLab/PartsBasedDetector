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
using namespace cvac;


///////////////////////////////////////////////////////////////////////////////
// This is called by IceBox to get the service to communicate with.
extern "C"
{
	//
	// ServiceManager handles all the icebox interactions so we construct
    // it and set a pointer to our detector.
	//
	ICE_DECLSPEC_EXPORT IceBox::Service* create(Ice::CommunicatorPtr communicator)
	{
        ServiceManager *sMan = new ServiceManager();
        DPMDetectionI *dpm = new DPMDetectionI(sMan);
        sMan->setService(dpm, "DPM_Detector");
        return (::IceBox::Service*) sMan->getIceService();
	}
}


///////////////////////////////////////////////////////////////////////////////

DPMDetectionI::DPMDetectionI(ServiceManager *sman)
: fInitialized(false)
{
    mServiceMan = sman;
}

DPMDetectionI::~DPMDetectionI()
{
	fInitialized = false;
}
                          // Client verbosity
void DPMDetectionI::initialize(::Ice::Int verbosity, const ::DetectorData& data, const ::Ice::Current& current)
{	
	// Get the default CVAC data directory as defined in the config file
    m_CVAC_DataDir = mServiceMan->getDataDir();	
	

	std::string archiveFilePath; 
	if ((data.file.directory.relativePath.length() > 1 && data.file.directory.relativePath[1] == ':' )||
		data.file.directory.relativePath[0] == '/' ||
		data.file.directory.relativePath[0] == '\\')
	{  // absolute path
		archiveFilePath = data.file.directory.relativePath + "/" + data.file.filename;
	} 
	else
	{ // prepend our prefix
		archiveFilePath = (m_CVAC_DataDir + "/" + data.file.directory.relativePath + "/" + data.file.filename);
	}

	std::vector<std::string> fileNameStrings =  expandSeq_fromFile(archiveFilePath, getName(current));

	// Need to strip off extra zeros
	std::string directory = std::string(getCurrentWorkingDirectory().c_str());
	std::string name = getName(current);
	std::string dpath;
	dpath.reserve(directory.length() + name.length() + 3);
	dpath += directory;
	dpath += std::string("/");
	dpath += ".";
	dpath += name;

	
	ifstream infile;
	string _fulllogfile = dpath + "/" + logfile_detectorData;
	infile.open(_fulllogfile.c_str());
	if(!infile.is_open())
	{
		localAndClientMsg(VLogger::WARN, NULL, "Failed to initialize because the file %s does not exist\n",_fulllogfile.c_str());
		return;
	}

	string _aline;
	getline(infile,_aline);
	boost::scoped_ptr<Model> model;	
	model.reset(new FileStorageModel);		
	string _fullline = dpath+ "/" + _aline;
	if(!model->deserialize(_fullline)) 
	{
		localAndClientMsg(VLogger::WARN, NULL, "Failed to initialize because the file %s has a problem\n",_fullline.c_str());
		return;
	}

	pbd.distributeModel(*model);
	fInitialized = true;	
}



bool DPMDetectionI::isInitialized(const ::Ice::Current& current)
{
	return fInitialized;
}
 
void DPMDetectionI::destroy(const ::Ice::Current& current)
{
	fInitialized = false;
}
std::string DPMDetectionI::getName(const ::Ice::Current& current)
{
	return "DPM_Detector";
}
std::string DPMDetectionI::getDescription(const ::Ice::Current& current)
{
	return "Deformable Parts Model detector";
}

void DPMDetectionI::setVerbosity(::Ice::Int verbosity, const ::Ice::Current& current)
{

}

DetectorData DPMDetectionI::createCopyOfDetectorData(const ::Ice::Current& current)
{	
	DetectorData data;
	return data;
}

DetectorPropertiesPrx DPMDetectionI::getDetectorProperties(const ::Ice::Current& current)
{	
	return NULL;
}

ResultSetV2 DPMDetectionI::processSingleImg(DetectorPtr detector,const char* fullfilename)
{	
	ResultSetV2 _resSet;	

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

//void DPMDetectionI::process(const ::DetectorCallbackHandlerPrx& callbackHandler,const ::RunSet& runset,const ::Ice::Current& current)
void DPMDetectionI::process(const Ice::Identity &client,const ::RunSet& runset,const ::Ice::Current& current)
{
	DetectorCallbackHandlerPrx _callback = DetectorCallbackHandlerPrx::uncheckedCast(current.con->createProxy(client)->ice_oneway());
  DoDetectFunc func = DPMDetectionI::processSingleImg;

  try {
    processRunSet(this, _callback, func, runset, m_CVAC_DataDir, mServiceMan);
  }
  catch (exception e) {
    localAndClientMsg(VLogger::ERROR, NULL, "$$$ Detector could not process given file-path.");
  }
}
