#include <string>
#include <FitConfigLoader.hh>
#include <DistConfigLoader.hh>
#include <DistConfig.hh>
#include <DistBuilder.hh>
#include <CutConfigLoader.hh>
#include <FitConfigLoader.hh>
#include <FitConfig.hh>
#include <SystConfigLoader.hh>
#include <SystConfig.hh>
#include <SystFactory.hh>
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
#include <MetropolisSampler.h>
#include <Minuit.h>

using namespace bbfit;


void
Fit(const std::string& mcmcConfigFile_, 
    const std::string& distConfigFile_,
    const std::string& cutConfigFile_, 
    const std::string& systConfigFile_,
    const std::string& dataPath_,
    const std::string& dims_,
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

  // Load up the systematics
  SystConfigLoader systLoader(systConfigFile_);
  SystConfig systConfig = systLoader.LoadActive();

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
      ROOTNtuple dataToFit(dataPath_, "pruned");
      
      // Log the effects of the cuts
      CutLog log(cutCol.GetCutNames());
      
      // and bin the data inside
      dataDist = DistBuilder::Build("data", pConfig, (DataSet*)&dataToFit, cutCol, log);
       
      std::ofstream ofs((outDir + "/data_cut_log.txt").c_str());
      ofs << "Cut log for data set " << dataPath_ << std::endl;
      ofs << log.AsString() << std::endl;
      ofs.close();
  }

  //marginalise over PSD for 3D fitting
  if(dims_=="3d"){
       std::cout<< "Marginilising for 3d" << std::endl;
      std::vector<std::string> keepObs;
      keepObs.push_back("energy");
      keepObs.push_back("r");
      keepObs.push_back("timePSD");
      dataDist = dataDist.Marginalise(keepObs);
  }

	
  //marginalise over PSD for 2D fitting
  if(dims_=="2d"){
      std::cout<< "Marginilising for 2d" << std::endl;
      std::vector<std::string> keepObs;
      keepObs.push_back("energy");
      keepObs.push_back("r");
      dataDist = dataDist.Marginalise(keepObs);
  }

  AxisCollection systAxes = DistBuilder::BuildAxes(pConfig);
  std::vector<std::string> dataObs = pConfig.GetBranchNames();
  ObsSet dataObsSet(dataObs);

  ParameterDict syst_nom =    systConfig.GetNominal();
  ParameterDict syst_maxima = systConfig.GetMaxima();
  ParameterDict syst_minima = systConfig.GetMinima();
  ParameterDict syst_mass =   systConfig.GetMass();
  ParameterDict syst_nbins =  systConfig.GetNBins();
  std::map<std::string, std::string> syst_type = systConfig.GetType();
  std::map<std::string, std::string> syst_obs =  systConfig.GetObs();

  std::map<std::string, Systematic*> syst_map;
 
  //Loop over systematics and declare each type. Must be a better way to do this but
  // it will do for now
  for(std::map<std::string, std::string>::iterator it = syst_type.begin(); it != syst_type.end();
      ++it) {
    std::vector<std::string> Obs;
    Obs.push_back(syst_obs[it->first]);
    ObsSet obsSet(Obs);

    double stddev_nom = 0;
    if(syst_type[it->first] == "conv")
      stddev_nom = syst_nom[it->first+"_stddevs"];
    Systematic *syst = SystFactory::New(it->first, syst_type[it->first], syst_nom[it->first], stddev_nom);
    syst->SetAxes(systAxes);
    syst->SetTransformationObs(obsSet);
    syst->SetDistributionObs(dataObsSet);
    syst->Construct();
    syst_map[it->first] = syst;
  }

  ParameterDict minima = mcConfig.GetMinima();
  ParameterDict maxima = mcConfig.GetMaxima();

  // now build the likelihood
  BinnedNLLH lh;
  lh.SetBufferAsOverflow(true);
  lh.SetBuffer("energy",12,12);
  lh.AddPdfs(dists);
  lh.SetCuts(cutCol);
  lh.SetDataDist(dataDist);
  for(std::map<std::string, Systematic*>::iterator it = syst_map.begin(); it != syst_map.end(); ++it) {
    lh.AddSystematic(syst_map[it->first]);
  }

  ParameterDict constrMeans  = mcConfig.GetConstrMeans();
  ParameterDict constrSigmas = mcConfig.GetConstrSigmas();

  //to do: add systematic constraints

  for(ParameterDict::iterator it = constrMeans.begin(); it != constrMeans.end();
      ++it)
    lh.SetConstraint(it->first, it->second, constrSigmas.at(it->first));

  // and now the optimiser
  // Create something to do hamiltonian sampling
  ParameterDict sigmas = mcConfig.GetSigmas();

  HamiltonianSampler<BinnedNLLH> sampler(lh, mcConfig.GetEpsilon(), 
                                       mcConfig.GetNSteps());
  
  ParameterDict masses;
  
  for(ParameterDict::iterator it = sigmas.begin(); it != sigmas.end(); ++it)
      masses[it->first] = 10/sigmas[it->first]/1/sigmas[it->first];

  for(ParameterDict::iterator it = syst_mass.begin(); it != syst_mass.end(); ++it){
    masses[it->first] = syst_mass[it->first];
    minima[it->first] = syst_minima[it->first];
    maxima[it->first] = syst_maxima[it->first];
  }
  
  sampler.SetMinima(minima);
  sampler.SetMaxima(maxima);
  sampler.SetMasses(masses);

  MCMC mh(sampler);

  bool saveChain = true;
  mh.SetSaveChain(saveChain);
  mh.SetMaxIter(mcConfig.GetIterations());
  mh.SetBurnIn(mcConfig.GetBurnIn());
  mh.SetMinima(minima);
  mh.SetMaxima(maxima);
  
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
  for(ParameterDict::iterator it = syst_nom.begin(); it != syst_nom.end(); ++it){
    std::cout << minima[it->first] << " " << maxima[it->first] << " " << syst_nbins[it->first] <<  " " << it->first << std::endl;
    lhAxes.AddAxis(BinAxis(it->first, minima[it->first], maxima[it->first], syst_nbins[it->first]));
  }

  mh.SetHistogramAxes(lhAxes);

  // go
  const FitResult& res = mh.Optimise(&lh);
  lh.SetParameters(res.GetBestFit());
  
  // Now save the results
  res.SaveAs(outDir + "/fit_result.txt");

  std::cout << "Saved fit result to " << outDir + "/fit_result.txt"
            << std::endl;

  MCMCSamples samples = mh.GetSamples();

  if(saveChain){
    TTree* outchain = samples.GetChain();
    //Messy way to make output filename. OutDir is full path to output result directory. Using find_last_of / and stripping everything before it to get directory name (tempString2). Similarly find name of directory above (where pdfs and fake data is saved). Output filename is then aboveDirectory_resultDirectory.root, inside OutDir. Could surely do this in fewer lines, or just call it outputTree.root or something, but nice to have a more unique name 
    std::string chainFileName = outDir;
    std::string tempString1 = outDir;
    std::string tempString2 = outDir;
    size_t last_slash_idx = tempString1.find_last_of("/");
    tempString2.erase(0, last_slash_idx+1 );
    if (std::string::npos != last_slash_idx){
      tempString1.erase(last_slash_idx,std::string::npos);
      last_slash_idx = tempString1.find_last_of("/");
      if (std::string::npos != last_slash_idx)
	tempString1.erase(0, last_slash_idx );
    }
    
    chainFileName = outDir+tempString1+"_"+tempString2+".root";
    TFile *f = new TFile(chainFileName.c_str(),"recreate");
    outchain->Write();  
  }

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

  //Initialise postfit distributions to same axis as data
  BinnedED postfitDist;
  postfitDist = dataDist;
  postfitDist.Empty();

  // scale the distributions to the correct heights
  // they are named the same as their fit parameters
  std::cout << "Saving scaled histograms and data to \n\t"
            << scaledDistDir << std::endl;

  if(dataDist.GetHistogram().GetNDims() < 3){
      ParameterDict bestFit = res.GetBestFit();
      for(size_t i = 0; i < dists.size(); i++){
	  std::string name = dists.at(i).GetName();
	  dists[i].Normalise();
	  dists[i].Scale(bestFit[name]);
	  IO::SaveHistogram(dists[i].GetHistogram(), 
                            scaledDistDir + "/" + name + ".root");
	  //sum all scaled distributions to get full postfit "dataset"
	  postfitDist.Add(dists[i]);
      }
      //      for(int i_syst = 0; i_syst < syst_vec.size(); i_syst ++) {
      for(std::map<std::string, Systematic*>::iterator it = syst_map.begin(); it != syst_map.end(); ++it) {
	double distInt = postfitDist.Integral();
	syst_map[it->first]->SetParameter(it->first,bestFit[it->first]);
	postfitDist = syst_map[it->first]->operator()(postfitDist);
	postfitDist.Scale(distInt);
      }
      IO::SaveHistogram(postfitDist.GetHistogram(),
			scaledDistDir + "/postfitdist.root");
  }else{
      ParameterDict bestFit = res.GetBestFit();
      for(size_t i = 0; i < dists.size(); i++){
	  std::string name = dists.at(i).GetName();
	  dists[i].Normalise();
	  dists[i].Scale(bestFit[name]);
	  
	  std::vector<std::string> keepObs;
	  keepObs.push_back("r");
	  keepObs.push_back("energy");
	  dists[i] = dists[i].Marginalise(keepObs);
	  IO::SaveHistogram(dists[i].GetHistogram(), 
			    scaledDistDir + "/" + name + ".root");
	  //sum all scaled distributions to get full postfit "dataset"
	  postfitDist.Add(dists[i]);
      }
      //for(int i_syst = 0; i_syst < syst_vec.size(); i_syst ++) {
      for(std::map<std::string, Systematic*>::iterator it = syst_map.begin(); it != syst_map.end(); ++it) {
	double distInt = postfitDist.Integral();
	syst_map[it->first]->SetParameter(it->first,bestFit[it->first]);
	postfitDist = syst_map[it->first]->operator()(postfitDist);
	postfitDist.Scale(distInt);
      }
      IO::SaveHistogram(postfitDist.GetHistogram(),
                        scaledDistDir + "/postfitdist.root");
  }

  // and also save the data
  if(dataDist.GetHistogram().GetNDims() < 3){
      IO::SaveHistogram(dataDist.GetHistogram(), scaledDistDir + "/" + "data.root");
  }else{
      std::vector<std::string> keepObs;
      keepObs.push_back("r");
      keepObs.push_back("energy");
      dataDist = dataDist.Marginalise(keepObs);
      IO::SaveHistogram(dataDist.GetHistogram(), scaledDistDir + "/" + "data.root");
  }
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
  if (argc != 7 && argc != 8){
    std::cout << "\nUsage: fit_dataset <fit_config_file> <dist_config_file> <cut_config_file> <syst_config_file> <data_to_fit> <4d,3d or 2d> <(opt) outdir_override>" << std::endl;
    return 1;
  }

  std::string fitConfigFile(argv[1]);
  std::string pdfPath(argv[2]);
  std::string cutConfigFile(argv[3]);
  std::string systConfigFile(argv[4]);
  std::string dataPath(argv[5]);
  std::string dims(argv[6]);
  std::string outDirOverride;
  if(argc == 8)
    outDirOverride = std::string(argv[7]);

  Fit(fitConfigFile, pdfPath, cutConfigFile, systConfigFile, dataPath, dims, outDirOverride);
  
  return 0;
}
 
