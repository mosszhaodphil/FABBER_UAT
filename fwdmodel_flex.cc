/*  fwdmodel_flex.cc - FLEX model

    Michael Chappell, IBME PUMMA & FMRIB Image Analysis Group

    Copyright (C) 2011 University of Oxford  */

/*  Part of FSL - FMRIB's Software Library
    http://www.fmrib.ox.ac.uk/fsl
    fsl@fmrib.ox.ac.uk
    
    Developed at FMRIB (Oxford Centre for Functional Magnetic Resonance
    Imaging of the Brain), Department of Clinical Neurology, Oxford
    University, Oxford, UK
    
    
    LICENCE
    
    FMRIB Software Library, Release 5.0 (c) 2012, The University of
    Oxford (the "Software")
    
    The Software remains the property of the University of Oxford ("the
    University").
    
    The Software is distributed "AS IS" under this Licence solely for
    non-commercial use in the hope that it will be useful, but in order
    that the University as a charitable foundation protects its assets for
    the benefit of its educational and research purposes, the University
    makes clear that no condition is made or to be implied, nor is any
    warranty given or to be implied, as to the accuracy of the Software,
    or that it will be suitable for any particular purpose or for use
    under any specific conditions. Furthermore, the University disclaims
    all responsibility for the use which is made of the Software. It
    further disclaims any liability for the outcomes arising from using
    the Software.
    
    The Licensee agrees to indemnify the University and hold the
    University harmless from and against any and all claims, damages and
    liabilities asserted by third parties (including claims for
    negligence) which arise directly or indirectly from the use of the
    Software or the sale of any products based on the Software.
    
    No part of the Software may be reproduced, modified, transmitted or
    transferred in any form or by any means, electronic or mechanical,
    without the express permission of the University. The permission of
    the University is not required if the said reproduction, modification,
    transmission or transference is done without financial return, the
    conditions of this Licence are imposed upon the receiver of the
    product, and all original and amended source code is included in any
    transmitted product. You may be held legally responsible for any
    copyright infringement that is caused or encouraged by your failure to
    abide by these terms and conditions.
    
    You are not permitted under this Licence to use this Software
    commercially. Use for which any financial return is received shall be
    defined as commercial use, and includes (1) integration of all or part
    of the source code or the Software into a product for sale or license
    by or on behalf of Licensee to third parties or (2) use of the
    Software or any derivative of it for research with the final aim of
    developing software products for sale or license to a third party or
    (3) use of the Software or any derivative of it for research with the
    final aim of developing non-software products for sale or license to a
    third party, or (4) use of the Software to provide any service to an
    external organisation for which payment is received. If you are
    interested in using the Software commercially, please contact Isis
    Innovation Limited ("Isis"), the technology transfer company of the
    University, to negotiate a licence. Contact details are:
    innovation@isis.ox.ac.uk quoting reference DE/9564. */

#include "fwdmodel_flex.h"

#include <iostream>
#include <newmatio.h>
#include <stdexcept>
#include "newimage/newimageall.h"
#include "miscmaths/miscprob.h"
using namespace NEWIMAGE;
#include "easylog.h"

string FLEXFwdModel::ModelVersion() const
{
  return "$Id: fwdmodel_flex.cc,v 1.1 2011/03/10 13:55:00 chappell Exp $";
}

void FLEXFwdModel::HardcodedInitialDists(MVNDist& prior, 
    MVNDist& posterior) const
{
    Tracer_Plus tr("FLEXFwdModel::HardcodedInitialDists");
    assert(prior.means.Nrows() == NumParams());

     SymmetricMatrix precisions = IdentityMatrix(NumParams()) * 1e-12;

    // Set priors

     int place=1;
     // baseline
     for (int i=1; i<=npoly; i++) {
       prior.means(place) = 0.0;
       precisions(place,place) = 0.01;
       place++;
     }

     // PTR
     for (int s=1; s<=ncomp; s++) {
       prior.means(place) = 0.0;
       place++;
     }
     //NB dont really need to worry about these are they will be ARD

     // deltw (ppm)
     for (int s=1; s<=ncomp; s++) {
       prior.means(place) = 0; //compspec(s,1);
       precisions(place,place) = 1;
       place++;
     }

     // kevol (log)
     for (int s=1; s<=ncomp; s++) {
       prior.means(place) = compspec(s,2);
       precisions(place,place) = 1;
       place++;
     }

     //phase
     for (int s=1; s<=ncomp; s++) {
       prior.means(place) = 0;
       precisions(place,place) = 1;
       place++;
     }

    // Set precsions on priors
    prior.SetPrecisions(precisions);
    
    // Set initial posterior
    posterior = prior;

    // For parameters with uniformative prior chosoe more sensible inital posterior
      posterior.means(1) = 1;
      precisions(1,1) = 10;
      place=2;

      if (npoly>1) {
	for (int i=1; i<=npoly; i++) {
	  precisions(place,place)=10;
	  place++;
	}
      }

      //      for (int s=1; s<=ncomp; s++) {
      //	posterior.means(place) = 0.1;
      //	precisions(place,place) = 10;
      //	place++;
      //      }

      posterior.SetPrecisions(precisions);
    
}    
    
    

void FLEXFwdModel::Evaluate(const ColumnVector& params, ColumnVector& result) const
{
  Tracer_Plus tr("FLEXFwdModel::Evaluate");

    // ensure that values are reasonable
    // negative check
  ColumnVector paramcpy = params;
   for (int i=1;i<=NumParams();i++) {
      if (params(i)<0) { paramcpy(i) = 0; }
      }

   //model matrices
   ColumnVector baseline(npoly);
   ColumnVector PTR(ncomp);
   ColumnVector deltw(ncomp);
   ColumnVector kevol(ncomp);
   ColumnVector phase(ncomp);

   // extract values from params
   int place=1;
   baseline = params.Rows(place,place+npoly-1);
   place += npoly;

   // PTR
   PTR = params.Rows(place,place+ncomp-1);
   place += ncomp;

   // deltw
   //deltw = params.Rows(place,place+ncomp-1);
   ColumnVector pw;
   pw = params.Rows(place,place+ncomp-1);
   for (int i=1; i<=ncomp; i++) {
     if (pw(i)>M_PI/2-1e-12) pw(i) = M_PI/2-1e-12;
     if (pw(i)<-M_PI/2+1e-12) pw(i) = -M_PI/2+1e-12;
     deltw(i) = compspec(i,1) + tan(pw(i));
     //deltw(i) = compspec(i,1) + pw(i);
   }
   

   place += ncomp;
   deltw /= 1e6; // starts out in ppm
   deltw *= field; //into Hz
   deltw = o1 - deltw; //offset from o1 frequency
   deltw *= 2*M_PI; //into radians/s

   // kevol = (ksw-1/T*_2s)
   kevol = exp( params.Rows(place,place+ncomp-1) ); //infer log(kevol)
   place += ncomp;

   //phase
   phase = params.Rows(place,place+ncomp-1);
   place += ncomp;


    result.ReSize(ntpts);
    result = 0.0;
    
    //baseline
    ColumnVector tpower(tevol);
    tpower=1;
    for (int i=1; i<=npoly; i++) {
      result += baseline(i)*tpower;
      tpower = SP(tpower,tevol);
    }

    for (int s=1; s<=ncomp; s++) {
      for (int i=1; i<=ntpts; i++) {
	result.Row(i) += PTR.Row(s)*exp( -kevol.Row(s)*tevol.Row(i) )*cos( (deltw.Row(s)*tevol.Row(i) + phase.Row(s)).AsScalar());
      }
    }

  return;
}


FLEXFwdModel::FLEXFwdModel(ArgsType& args)
{
  Tracer_Plus tr("FLEXFwdModel");

    string scanParams = args.ReadWithDefault("scan-params","cmdline");

    // compspec
    // 2 Columns: Freq (ppm), kevol (s^-1)
    // Nrows = number of components

    if (scanParams == "cmdline")
    {
      // read timings from file
      tevol = read_ascii_matrix(args.Read("tevol")); //in s

      //read component specificaiton from file
      compspec = read_ascii_matrix(args.Read("comps"));

      //read field (Hz)
      field = convertTo<double>(args.Read("field"));

      //read FLEX offset
      o1 = convertTo<double>(args.Read("o1"));

      npoly = convertTo<int>(args.ReadWithDefault("npoly","1"));

    }

    else
        throw invalid_argument("Only --scan-params=cmdline is accepted at the moment");    

    ncomp = compspec.Nrows();
    ntpts = tevol.Nrows();

    //ARD on baseline parameters (except DC term)
    if (npoly>1) {
    for (int i=1; i<npoly; i++) {
      ard_index.push_back(i+1);
    }
    }
    //ARD on PTR values
    for (int i=0 ; i<ncomp; i++) {
      ard_index.push_back(npoly+i+1);
    }
}

void FLEXFwdModel::ModelUsage()
{ 
  cout << "\nUsage info for --model=FLEX:\n"
       << "Undefined\n"
    ;
}

void FLEXFwdModel::DumpParameters(const ColumnVector& vec,
                                    const string& indent) const
{
  //cout << vec.t() << endl;
}

void FLEXFwdModel::NameParams(vector<string>& names) const
{
  names.clear();

  // name the parameters for the pools using letters
  string lettervec [] = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"};
  
  // baseline
  for (int i=1; i<=npoly; i++) {
    names.push_back("BL_" + lettervec[i-1]);
  }
  
  // components
    for (int i=1; i<=ncomp; i++) {
      names.push_back("PTR_" + lettervec[i-1]);
    }

for (int i=1; i<=ncomp; i++) {
      names.push_back("deltw_" + lettervec[i-1]);
    }
  
    for (int i=1; i<=ncomp; i++) {
      names.push_back("kevol_" + lettervec[i-1]);
    }

    for (int i=1; i<=ncomp; i++) {
      names.push_back("phs_" + lettervec[i-1]);
    }


}

void FLEXFwdModel::SetupARD( const MVNDist& theta, MVNDist& thetaPrior, double& Fard)
{
  Tracer_Plus tr("FLEXFwdModel::SetupARD");

  if (doard)
    {
      //sort out ARD indices

      Fard = 0;

      int ardindex;
      for (unsigned int i=0; i<ard_index.size(); i++) {
	//iterate over all ARD parameters
	ardindex = ard_index[i];

	SymmetricMatrix PriorPrec;
	PriorPrec = thetaPrior.GetPrecisions();
	
	PriorPrec(ardindex,ardindex) = 1e-12; //set prior to be initally non-informative
	
	thetaPrior.SetPrecisions(PriorPrec);
	
	thetaPrior.means(ardindex)=0;

	
	//set the Free energy contribution from ARD term
	SymmetricMatrix PostCov = theta.GetCovariance();
	double b = 2/(theta.means(ardindex)*theta.means(ardindex) + PostCov(ardindex,ardindex));
	Fard += -1.5*(log(b) + digamma(0.5)) - 0.5 - gammaln(0.5) - 0.5*log(b); //taking c as 0.5 - which it will be!
      }
  }
  return;
}

void FLEXFwdModel::UpdateARD(
				const MVNDist& theta,
				MVNDist& thetaPrior, double& Fard) const
{
  Tracer_Plus tr("FLEXFwdModel::UpdateARD");
  
  if (doard)
    Fard=0;
    {
      int ardindex;
      for (unsigned int i=0; i<ard_index.size(); i++) {
	//iterate over all ARD parameters
	ardindex = ard_index[i];

  
      SymmetricMatrix PriorCov;
      SymmetricMatrix PostCov;
      PriorCov = thetaPrior.GetCovariance();
      PostCov = theta.GetCovariance();

      PriorCov(ardindex,ardindex) = theta.means(ardindex)*theta.means(ardindex) + PostCov(ardindex,ardindex);

      
      thetaPrior.SetCovariance(PriorCov);

      //Calculate the extra terms for the free energy
      double b = 2/(theta.means(ardindex)*theta.means(ardindex) + PostCov(ardindex,ardindex));
      //      if (b>1e12) b=1e12;
      Fard += -1.5*(log(b) + digamma(0.5)) - 0.5 - gammaln(0.5) - 0.5*log(b); //taking c as 0.5 - which it will be!
    }
  }

  return;

  }

