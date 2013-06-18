/* 
 *  Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  File:    main.cpp
 *  Author:  Hilton Bristow
 *  Created: Jun 27, 2012
 */

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <dirent.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include "PartsBasedDetector.hpp"
#include "Candidate.hpp"
#include "FileStorageModel.hpp"
#include "MatlabIOModel.hpp"
#include "Visualize.hpp"
#include "types.hpp"
#include "nms.hpp"
#include "Rect3.hpp"
#include "DistanceTransform.hpp"
using namespace cv;
using namespace std;
using namespace boost::filesystem;


int main(int argc, char** argv) {

	// check arguments
	if (argc != 3 && argc != 4) {
		printf("Usage: DPMDirDetector model_file directory (optional) -v\n");
		exit(-1);
	}

	// determine the type of model to read
	boost::scoped_ptr<Model> model;
	string ext = boost::filesystem::path(argv[1]).extension().string();
	if (ext.compare(".xml") == 0 || ext.compare(".yaml") == 0) {
		model.reset(new FileStorageModel);
	}
	else if (ext.compare(".mat") == 0) {
		model.reset(new MatlabIOModel);
	}
	else {
		printf("Unsupported model format: %s\n", ext.c_str());
		exit(-2);
	}
	bool ok = model->deserialize(argv[1]);
	if (!ok) {
		printf("Error deserializing file\n");
		exit(-3);
	}

 	//create the PartsBasedDetector and distribute the model parameters
	PartsBasedDetector<float> pbd;
	pbd.distributeModel(*model);
	

	bool vis = false;
	if (argc == 4) {
		if (strcmp(argv[3], "-v") == 0) vis = true;
	}
	
	path p (argv[2]);   // p reads clearer than argv[1] in the following code

	int imagecnt = 0;
	int poscnt = 0;
	Mat_<float> depth;
	
  	try
  	{
    if (exists(p)){    // does p actually exist?
    
      if (is_regular_file(p))        // is p a regular file?   
        cout << p << " is a file of size is " << file_size(p) << " not a directory!\n";

      else if (is_directory(p)) {     // is p a directory?
      
        cout << p << "\n";

        DIR *pDIR;
        struct dirent *entry;
        const char * chardir = (p.string()).c_str();
        Mat im;
        string suffix = ".jpg";
        size_t lensuffix = suffix.size();
        string filename;
        string filepath;
        size_t lenfilename;
        string impath;
        stringstream ss;
        string d_path = p.string();
        double t;
		vector<Candidate> candidates;
		
		//needed to visualize
		Mat canvas;
		
        if( pDIR=opendir(chardir) ){
        		//loop through directory and get all .jpg
                while(entry = readdir(pDIR)){
                    filename = entry->d_name;
                    lenfilename = filename.size();
                    //check files whose name is larger than the suffix
                    if(lenfilename > lensuffix + 1){
                        filepath = d_path + filename;
                        im = imread(filepath.c_str());
                        if (!im.empty()) {
                            imagecnt++;
                            //cout << filename << "\t";
                            t = (double)getTickCount();
                            candidates.clear();
                            pbd.detect(im, depth, candidates, vis);
                            //printf("Detection time: %f\n", ((double)getTickCount() - t)/getTickFrequency());
                            if(candidates.size() != 0){
                                poscnt++;
                                //Candidate::nonMaximaSuppression(im, candidates, 0.5);
                            }
                            //printf("%ld\n", candidates.size());

                            // if -v display the best candidates
                            if(vis){
                                Visualize visualize(model->name());
                                SearchSpacePruning<float> ssp;

                                if (candidates.size() > 0) {
                                    Candidate::sort(candidates);
                                    //Candidate::nonMaximaSuppression(im, candidates, 0.2);
                                    //Do Not visualize
                                    visualize.candidates(im, candidates, canvas, true);
                                       visualize.image(canvas);
                                    waitKey();
                                }
                            }

                    }
                    else {

                    }

                    }


                }
                closedir(pDIR);
        }
        
        
      }

      else
        cout << p << " exists, but is neither a regular file nor a directory\n";
    }
    else
      cout << p << " does not exist\n";
  }

  catch (const filesystem_error& ex) {
    cout << ex.what() << '\n';
  }	
	
	printf("%s had %d images %d positive detects\n",argv[1],imagecnt,poscnt);	
	return 0;
}
