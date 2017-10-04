#include <string>
#include <FitConfigLoader.hh>
#include <DistConfigLoader.hh>
#include <DistConfig.hh>
#include <DistBuilder.hh>
#include <CutConfigLoader.hh>
#include <FitConfigLoader.hh>
#include <FitConfig.hh>
#include <CutFactory.hh>
#include <CutCollection.h>
#include <fstream>
#include <ROOTNtuple.h>
#include <BinnedNLLH.h>
#include <sys/stat.h>
#include <Rand.h>
#include <AxisCollection.h>
#include <IO.h>
#include <MCMC.h>
#include <HamiltonianSampler.h>
#include <SigmoidBoundary.h>
#include <MetropolisSampler.h>

using namespace bbfit;

void
Fit(const std::string& mcmcConfigFile_, 
    const std::string& distConfigFile_,
    const std::string& cutConfigFile_, 
    const std::string& dataPath_,
    const std::string& outDirOverride_){
    Rand::SetSeed(0);


    // Load up the configuration data
    FitConfig mcConfig;
  
    typedef std::vector<CutConfig> CutVec;
    CutVec cutConfs;
    
    {
        FitConfigLoader mcLoader(mcmcConfigFile_);
        mcConfig = mcLoader.LoadActive();
    
        CutConfigLoader cutConfLoader(cutConfigFile_);
        cutConfs = cutConfLoader.LoadActive();
    }
    


    // create the output directories
    std::string outDir = mcConfig.GetOutDir();
    if(outDirOverride_ != "")
        outDir = outDirOverride_;
    
    std::string projDir1D = outDir + "/1dlhproj";
    std::string projDir2D = outDir + "/2dlhproj";
    std::string scaledDistDir = outDir + "/scaled_dists";
    
    struct stat st = {0};
    if (stat(outDir.c_str(), &st) == -1) {
        mkdir(outDir.c_str(), 0700);
    }
    
    if (stat(projDir1D.c_str(), &st) == -1) {
        mkdir(projDir1D.c_str(), 0700);
    }
    
    if (stat(projDir2D.c_str(), &st) == -1) {
        mkdir(projDir2D.c_str(), 0700);
    }
    
    if (stat(scaledDistDir.c_str(), &st) == -1) {
        mkdir(scaledDistDir.c_str(), 0700);
    }
        

  // Make the cuts
  CutCollection cutCol;
  for(CutVec::iterator it = cutConfs.begin(); it != cutConfs.end();
      ++it){
    std::string name = it->GetName();
    std::string type = it->GetType();
    std::string obs = it->GetObs();
    double val = it->GetValue();
    double val2 = it->GetValue2();
    Cut *cut = CutFactory::New(name, type, obs, val, val2);
    cutCol.AddCut(*cut);
    delete cut; // cut col takes its own copy
  }
  

  // Load up the dists
  DistConfigLoader dLoader(distConfigFile_);
  DistConfig pConfig = dLoader.Load();
  std::string distDir = pConfig.GetPDFDir();

  std::vector<BinnedED> dists;
  
  
  // the ones you actually want to fit are those listed in mcmcconfig
  typedef std::set<std::string> StringSet;
  StringSet distsToFit = mcConfig.GetParamNames();
  

  for(StringSet::iterator it = distsToFit.begin(); it != distsToFit.end();
      ++it){
    std::string distPath = distDir + "/" + *it + ".h5";
    dists.push_back(BinnedED(*it, IO::LoadHistogram(distPath)));
  }


  // if its a root tree then load it up
  BinnedED dataDist;
  
  if(dataPath_.substr(dataPath_.find_last_of(".") + 1) == "h5"){
      Histogram loaded = IO::LoadHistogram(dataPath_);
      dataDist = BinnedED("data", loaded);
      dataDist.SetObservables(pConfig.GetBranchNames());
  }
  else{
      // Load up the data set
      ROOTNtuple dataToFit(dataPath_, "oxsx_saved");
      
      // Log the effects of the cuts
      CutLog log(cutCol.GetCutNames());
      
      // and bin the data inside
      dataDist = DistBuilder::Build("data", pConfig, (DataSet*)&dataToFit, cutCol, log);
      
  // 
      std::ofstream ofs((outDir + "/data_cut_log.txt").c_str());
      ofs << "Cut log for data set " << dataPath_ << std::endl;
      ofs << log.AsString() << std::endl;
      ofs.close();
  }

  // now build the likelihood
  BinnedNLLH lh;
  lh.AddPdfs(dists);
  lh.SetCuts(cutCol);
  lh.SetDataDist(dataDist);

  ParameterDict constrMeans  = mcConfig.GetConstrMeans();
  ParameterDict constrSigmas = mcConfig.GetConstrSigmas();
  for(ParameterDict::iterator it = constrMeans.begin(); it != constrMeans.end();
      ++it)
      lh.SetConstraint(it->first, it->second, constrSigmas.at(it->first));

  // and now the optimiser
  // Create something to do hamiltonian sampling
  ParameterDict sigmas = mcConfig.GetSigmas();

  typedef SigmoidBoundary<BinnedNLLH> Post_t;
  Post_t posterior(lh, mcConfig.GetMinima(), mcConfig.GetMaxima(), 1000);
  
  HamiltonianSampler<Post_t> sampler(posterior, mcConfig.GetEpsilon(), 
                                     mcConfig.GetNSteps());
  // sampler.SetMinima(mcConfig.GetMinima());
  // sampler.SetMaxima(mcConfig.GetMaxima());
  ParameterDict masses;

  for(ParameterDict::iterator it = sigmas.begin(); it != sigmas.end(); ++it)
      masses[it->first] = 1/sigmas[it->first]/1/sigmas[it->first];

  sampler.SetMasses(masses);
  
  MCMC mh(sampler);

  mh.SetMaxIter(mcConfig.GetIterations());
  mh.SetBurnIn(mcConfig.GetBurnIn());
  mh.SetMinima(mcConfig.GetMinima());
  mh.SetMaxima(mcConfig.GetMaxima());
  
  // we're going to minmise not maximise -log(lh) and the 
  // mc chain needs to know that the test stat is logged 
  // otherwise it will give us the distribution of the log(lh) not the lh

  mh.SetTestStatLogged(true);
  mh.SetFlipSign(true);
  
  // create some axes for the mc to fill
  AxisCollection lhAxes;
  for(StringSet::iterator it = distsToFit.begin(); it != distsToFit.end();
      ++it){
    lhAxes.AddAxis(BinAxis(*it, mcConfig.GetMinima()[*it], 
			   mcConfig.GetMaxima()[*it],
			   mcConfig.GetNBins()[*it]		   
			   )
		   );
  }

  mh.SetHistogramAxes(lhAxes);
  
  // go
  const FitResult& res = mh.Optimise(&lh);

  // Now save the results
  res.SaveAs(outDir + "/fit_result.txt");
  
  std::cout << "Saved fit result to " << outDir + "/fit_result.txt"
            << std::endl;

  MCMCSamples samples = mh.GetSamples();

  // save the histograms
  typedef std::map<std::string, Histogram> HistMap;
  const HistMap& proj1D = samples.Get1DProjections();
  const HistMap& proj2D = samples.Get2DProjections();

  std::cout << "Saving LH projections to \n\t" 
            << projDir1D
            << "\n\t"
            << projDir2D
            << std::endl;

  for(HistMap::const_iterator it = proj1D.begin(); it != proj1D.end();
      ++it){
    IO::SaveHistogram(it->second, projDir1D + "/" + it->first + ".root");
  }

  for(HistMap::const_iterator it = proj2D.begin(); it != proj2D.end();
      ++it){
    IO::SaveHistogram(it->second, projDir2D + "/" + it->first + ".root");
  }

  // scale the distributions to the correct heights
  // they are named the same as their fit parameters
  std::cout << "Saving scaled histograms and data to \n\t"
            << scaledDistDir << std::endl;

  ParameterDict bestFit = res.GetBestFit();
  for(size_t i = 0; i < dists.size(); i++){
    std::string name = dists.at(i).GetName();
    dists[i].Normalise();
    dists[i].Scale(bestFit[name]);
    IO::SaveHistogram(dists[i].GetHistogram(), 
		      scaledDistDir + "/" + name + ".root");
  }

  // and also save the data
  IO::SaveHistogram(dataDist.GetHistogram(), scaledDistDir + "/" + "data.root");
  // avoid binning again if not nessecary
  IO::SaveHistogram(dataDist.GetHistogram(),  outDir + "/" + "data.h5");

  // save autocorrelations
  std::ofstream cofs((outDir + "/auto_correlations.txt").c_str());
  std::vector<double> autocors = samples.GetAutoCorrelations();
  for(size_t i = 0; i < autocors.size(); i++)
      cofs << i << "\t" << autocors.at(i) << "\n";
  cofs.close();

  // and a copy of all of the configurations used
  std::ifstream if_a(mcmcConfigFile_.c_str(), std::ios_base::binary);
  std::ifstream if_b(cutConfigFile_.c_str(),  std::ios_base::binary);
  
  std::ofstream of((outDir + "/config_log.txt").c_str(), std::ios_base::binary);
  
  of << "dists from : " << distDir
     << "\n\n\n" << "data set fit : " << dataPath_
     << "\n\n\n" << if_a.rdbuf()
     << "\n\n\n" << if_b.rdbuf();
  
}

int main(int argc, char *argv[]){
  if (argc != 5 && argc != 6){
    std::cout << "\nUsage: fit_dataset <fit_config_file> <dist_config_file> <cut_config_file> <data_to_fit> <(opt) outdir_override>" << std::endl;
      return 1;
  }

  std::string fitConfigFile(argv[1]);
  std::string pdfPath(argv[2]);
  std::string cutConfigFile(argv[3]);
  std::string dataPath(argv[4]);
  std::string outDirOverride;
  if(argc == 6)
    outDirOverride = std::string(argv[5]);

  Fit(fitConfigFile, pdfPath, cutConfigFile, dataPath, outDirOverride);

  return 0;
}
