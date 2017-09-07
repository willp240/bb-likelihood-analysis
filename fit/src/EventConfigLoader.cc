#include <EventConfigLoader.hh>
#include <EventConfig.hh>
#include <ConfigLoader.hh>
#include <map>
#include <algorithm>

namespace bbfit{

EventConfigLoader::EventConfigLoader(const std::string& filePath_){
    fPath = filePath_;    
}

EventConfig
EventConfigLoader::LoadOne(const std::string& name_) const{
  ConfigLoader::Open(fPath);
  std::string baseDir;
  std::string prunedDir;
  ConfigLoader::Load("summary", "orig_base_dir", baseDir);
  ConfigLoader::Load("summary", "pruned_ntup_dir", prunedDir);


  double rate;
  int    nGenerated;
  std::string texLabel;
  std::vector<std::string> ntupFiles;

  ConfigLoader::Load(name_, "rate", rate);
  ConfigLoader::Load(name_, "n_generated", nGenerated);
  ConfigLoader::Load(name_, "texLabel", texLabel);
  ConfigLoader::Load(name_, "ntup_files", ntupFiles);

  EventConfig retVal;
  retVal.SetRate(rate);
  retVal.SetNGenerated(nGenerated);
  retVal.SetNtupFiles(ntupFiles);
  retVal.SetTexLabel(texLabel);
  retVal.SetName(name_);
  retVal.SetNtupBaseDir(baseDir);
  retVal.SetPrunedPath(prunedDir + name_ + ".root");
  return retVal;
}

std::map<std::string, EventConfig>
EventConfigLoader::LoadActive() const{
  typedef std::vector<std::string> StringVec;
  typedef std::map<std::string, EventConfig> EventConfigMap;
  ConfigLoader::Open(fPath);

  StringVec toLoad;
  ConfigLoader::Load("summary", "active", toLoad);

  //  if all is in the list, just do all of them
  if(std::find(toLoad.begin(), toLoad.end(), "all") != toLoad.end())
    return LoadAll();

  EventConfigMap evMap;
  for(size_t i = 0; i < toLoad.size(); i++)
    evMap[toLoad.at(i)] = LoadOne(toLoad.at(i));

  return evMap;
}

EventConfigLoader::~EventConfigLoader(){ 
  ConfigLoader::Close();
}

std::map<std::string, EventConfig>
EventConfigLoader::LoadAll() const{
  typedef std::set<std::string> StringSet;
  StringSet toLoad = ConfigLoader::ListSections();
  toLoad.erase("summary");

  std::map<std::string, EventConfig> evMap;
  for(StringSet::iterator it = toLoad.begin(); it != toLoad.end();
      ++it){
    evMap[*it] = LoadOne(*it);
  }
  return evMap;
}

}
