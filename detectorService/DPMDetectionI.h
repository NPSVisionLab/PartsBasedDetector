#ifndef _DPMDetectionI_H__
/*****************************************************************************
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
 ****************************************************************************/
#define _DPMDetectionI_H__

#include <Data.h>
#include <Services.h>

#include <Ice/Ice.h>
#include <IceBox/IceBox.h>
#include <IceUtil/UUID.h>
#include <util/processRunSet.h>
#include <util/ServiceMan.h>
#include <util/ServiceManI.h>
#include <util/RunSetIterator.h>
#include <util/DetectorDataArchive.h>

//#include <opencv2/opencv.hpp>

#include <PartsBasedDetector.hpp>
#include <Candidate.hpp>
#include <FileStorageModel.hpp>
#include <Visualize.hpp> //only for visualization of results

class DPMDetectionI : public cvac::Detector, public cvac::StartStop
{
public:
  DPMDetectionI();
  ~DPMDetectionI();

  std::string m_CVAC_DataDir; // Store an absolute path to the detector data files

public:
  virtual void process(const Ice::Identity &client,const ::cvac::RunSet& runset,
                       const ::cvac::FilePath &detectorData,
                       const ::cvac::DetectorProperties &props,
                       const ::Ice::Current& current);
  virtual std::string getName(const ::Ice::Current& current);
  virtual std::string getDescription(const ::Ice::Current& current);
  virtual bool cancel(const Ice::Identity &client, const ::Ice::Current& current);
  virtual cvac::DetectorProperties getDetectorProperties(const Ice::Current& current);
  void setServiceManager(cvac::ServiceManagerI *sman);
  virtual void starting();
  virtual void stopping();

protected:
  bool initialize(cvac::DetectorDataArchive* _dda,
                  const ::cvac::FilePath &file,
                  const::Ice::Current &current);
  bool isInitialized();
  virtual void destroy(const ::Ice::Current& current);

private:
  cvac::ServiceManager *mServiceMan;
  PartsBasedDetector<double> pbd;  
  bool  fInitialized;
  std::string filepathDefaultModel;

  std::vector<Candidate> detectObjects(const cvac::CallbackHandlerPrx& _callback,
                                      const cvac::Labelable& _lbl,
                                      bool& _resFlag,std::string& _resStr);
  void addResult(cvac::Result& _res,cvac::Labelable& _converted,
                 std::vector<Candidate> _candidates,bool _resFlag,std::string _resStr);

//     static cvac::ResultSet processSingleImg(cvac::DetectorPtr detector,const char* fullfilename);    
//     std::string getPathDetectorData();
//     bool initialize(const ::cvac::DetectorProperties& props,
//                     const ::cvac::FilePath& model, const ::Ice::Current& current);
};

#endif //_DPMDetectionI_H__
