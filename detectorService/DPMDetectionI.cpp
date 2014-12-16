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

//using namespace Ice;
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
    DPMDetectionI *dpm = new DPMDetectionI();
    ServiceManagerI *sMan = new ServiceManagerI( dpm, dpm );
    dpm->setServiceManager( sMan );
    return sMan;
  }
}

///////////////////////////////////////////////////////////////////////////////

DPMDetectionI::DPMDetectionI()
  :fInitialized(false),filepathDefaultModel("")
{
  mServiceMan = NULL;
}

DPMDetectionI::~DPMDetectionI()
{
}

void DPMDetectionI::setServiceManager(cvac::ServiceManagerI *sman)
{
  mServiceMan = sman;
}

void DPMDetectionI::starting()
{
  m_CVAC_DataDir = mServiceMan->getDataDir();

  filepathDefaultModel = mServiceMan->getModelFileFromConfig();

  if(filepathDefaultModel.empty())
    localAndClientMsg(VLogger::DEBUG, NULL,
    "No trained model file specified in service config.\n" );
  else
  {
    localAndClientMsg(VLogger::DEBUG, NULL,
      "Will read trained model file as specified in service config: %s\n",
      filepathDefaultModel.c_str());
  }
}

void DPMDetectionI::stopping()
{
}

bool DPMDetectionI::initialize(cvac::DetectorDataArchive* _dda,
                               const ::cvac::FilePath &file,
                               const::Ice::Current &current)
{
  // Set CVAC verbosity according to ICE properties
  Ice::PropertiesPtr props = (current.adapter->getCommunicator()->getProperties());
  string verbStr = props->getProperty("CVAC.ServicesVerbosity");
  if (!verbStr.empty())
    getVLogger().setLocalVerbosityLevel( verbStr );

  // Get the default CVAC data directory as defined in the config file    
  std::string connectName = getClientConnectionName(current);
  std::string clientName = mServiceMan->getSandbox()->createClientName(mServiceMan->getServiceName(),
    connectName);                               
  std::string clientDir = mServiceMan->getSandbox()->createClientDir(clientName);

  string zipfilepath;
  if(filepathDefaultModel.empty() || file.filename.empty() == false)  //use default model file only if we did not get one.
    zipfilepath = getFSPath(file, m_CVAC_DataDir);    
  else
  {
    if (pathAbsolute(filepathDefaultModel))
      zipfilepath = filepathDefaultModel;
    else
      zipfilepath = m_CVAC_DataDir + "/" + filepathDefaultModel;
  } 
  _dda->unarchive(zipfilepath, clientDir);

  string modelXML = _dda->getFile( XMLID );
  if (modelXML.empty())
  {
      localAndClientMsg(VLogger::ERROR, NULL,
                        "Could not find XML result file in the zip file.\n");
      fInitialized = false;
      return false;
  }
  // Get only the filename part
  //modelXML = getFileName(modelXML);  TODO: why???  it doesn't work without the path. matz.  
  boost::scoped_ptr<Model> fsmodel;   
  fsmodel.reset(new FileStorageModel);                
  if(!fsmodel->deserialize( modelXML )) 
  {
      localAndClientMsg(VLogger::ERROR, NULL, 
        "Failed to initialize because the file %s has a problem\n",
        modelXML.c_str());
      fInitialized = false;
      return false;
  }

  pbd.distributeModel(*fsmodel);
  fInitialized = true;
  return true;
}

bool DPMDetectionI::isInitialized()
{
  return fInitialized;
}

void DPMDetectionI::destroy(const ::Ice::Current& current)
{
  fInitialized = false;
}

std::string DPMDetectionI::getName(const ::Ice::Current& curren)
{
  return mServiceMan->getServiceName();
}

std::string DPMDetectionI::getDescription(const ::Ice::Current& curren)
{
  return "Deformable Parts Model - Empty Description";
}

bool DPMDetectionI::cancel(const Ice::Identity &client, const ::Ice::Current& current)
{
  stopping();
  mServiceMan->waitForStopService();
  if(mServiceMan->isStopCompleted())
    return true;
  else 
    return false;
}

DetectorProperties DPMDetectionI::getDetectorProperties(const ::Ice::Current& current)
{
  //TODO get the real detector properties but for now return an empty one.
  DetectorProperties props;
  return props;
}

void DPMDetectionI::process(const Ice::Identity &client,
                            const ::RunSet& runset, const ::cvac::FilePath &trainedModelFile,  
                            const::cvac::DetectorProperties &props,
                            const ::Ice::Current& current)
{
  DetectorCallbackHandlerPrx callback = 
    DetectorCallbackHandlerPrx::uncheckedCast(current.con->createProxy(client)->ice_oneway());

  DetectorDataArchive dda;
  initialize(&dda, trainedModelFile, current);

  if(!isInitialized())
  {
    localAndClientMsg(VLogger::ERROR, callback, "DPMDetectionI not initialized, aborting.\n");
  }

  //////////////////////////////////////////////////////////////////////////
  // Setup - RunsetConstraints
  cvac::RunSetConstraint mRunsetConstraint;  
  mRunsetConstraint.addType("jpg");
//   mRunsetConstraint.addType("png");
//   mRunsetConstraint.addType("tif");  
//   mRunsetConstraint.addType("jpeg");
  mRunsetConstraint.addType("JPG");
//   mRunsetConstraint.addType("PNG");
//   mRunsetConstraint.addType("TIF");  
//   mRunsetConstraint.addType("JPEG");
  // End - RunsetConstraints

  //////////////////////////////////////////////////////////////////////////
  // Start - RunsetWrapper
  mServiceMan->setStoppable();  
  cvac::RunSetWrapper mRunsetWrapper(&runset,m_CVAC_DataDir,mServiceMan);
  mServiceMan->clearStop();
  if(!mRunsetWrapper.isInitialized())
  {
    localAndClientMsg(VLogger::ERROR, callback,
      "RunsetWrapper is not initialized, aborting.\n");    
    return;
  }
  // End - RunsetWrapper

  //////////////////////////////////////////////////////////////////////////
  // Start - RunsetIterator
  int nSkipFrames = 150;  //the number of skip frames
  mServiceMan->setStoppable();
  cvac::RunSetIterator mRunsetIterator(&mRunsetWrapper,mRunsetConstraint,
    mServiceMan,callback,nSkipFrames);
  mServiceMan->clearStop();
  if(!mRunsetIterator.isInitialized())
  {
    localAndClientMsg(VLogger::ERROR, callback,
      "RunSetIterator is not initialized, aborting.\n");
    return;
  } 
  // End - RunsetIterator

  mServiceMan->setStoppable();
  while(mRunsetIterator.hasNext())
  {
    if((mServiceMan != NULL) && (mServiceMan->stopRequested()))
    {        
      mServiceMan->stopCompleted();
      break;
    }

    cvac::Labelable& labelable = *(mRunsetIterator.getNext());
    {
      bool _resFlag(false);
      std::string _resStr;
      std::vector<Candidate> objects = detectObjects( callback, labelable, _resFlag,_resStr);
      addResult(mRunsetIterator.getCurrentResult(),labelable,objects,_resFlag,_resStr);      
    }
  }  
  callback->foundNewResults(mRunsetIterator.getResultSet());
  mServiceMan->clearStop();
}

/** run the cascade on the image described in lbl,
 *  return the objects (rectangles) that were found
 */
std::vector<Candidate> DPMDetectionI::detectObjects(const CallbackHandlerPrx& _callback,
                                                    const cvac::Labelable& _lbl,
                                                    bool& _resFlag,
                                                    std::string& _resStr)
{
  FilePath fpath = RunSetWrapper::getFilePath(_lbl);
  string tfilepath = getFSPath( fpath, m_CVAC_DataDir );


  localAndClientMsg(VLogger::DEBUG, _callback, "while detecting objects in %s\n",
                    tfilepath.c_str());

  _resStr = "";
  _resFlag = false;

  std::vector<Candidate> _candidates;
  cv::Mat _depth;
  cv::Mat _img = cv::imread(tfilepath.c_str());
  if (_img.empty())//no file or not supported format
  {
    std::string msgout;
    msgout = "The file \"" + tfilepath + 
      "\" has a problem (no file or not supported format). "+
      "So, it will not be processed.\n";
    localAndClientMsg(VLogger::WARN, NULL,msgout.c_str());
    _resStr = "Error: no file or not supported format";
    _resFlag = false;
  }
  else
  { 
    float confidence = 1.0f;
    DPMDetectionI* _pDPM = static_cast<DPMDetectionI*>(this);
    _pDPM->pbd.detect(_img, _depth, _candidates);
    _resStr = "";
    _resFlag = true;
  }
  return _candidates; 
}

void DPMDetectionI::addResult(cvac::Result& _res,
                              cvac::Labelable& _converted,
                              std::vector<Candidate> _candidates,
                              bool _resFlag,std::string _resStr)
{
  LabelablePtr labelable = new Labelable();
  
  if(!_resFlag)//when there is a problem
  {
    labelable->lab.name = _resStr;
    labelable->confidence = 1.0f;
    labelable->lab.hasLabel = false;
  }
  else  //when the input file is processed without a problem
  {
    if(_candidates.size()<1)
    {
      labelable->lab.name = "negative";
      labelable->confidence = 1.0f;
      labelable->lab.hasLabel = true;
    }
    else
    {
      labelable->lab.name = "positive";
      labelable->confidence = 1.0f;
      labelable->lab.hasLabel = true;
    }
  }
  _res.foundLabels.push_back(labelable);
}

