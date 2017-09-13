#include "FitConfig.hh"
#include <ContainerTools.hpp> 

namespace bbfit{

ParameterDict 
FitConfig::GetMinima() const{
  return fMinima;
}

ParameterDict
FitConfig::GetMaxima() const{
  return fMaxima;
}

ParameterDict
FitConfig::GetSigmas() const{
  return fSigmas;
}

ParameterDict
FitConfig::GetNBins() const{
  return fNbins;
}
  
int
FitConfig::GetIterations() const{
  return fIterations;
}

void
FitConfig::SetIterations(int it_){
  fIterations = it_;
}
  
int
FitConfig::GetBurnIn() const{
    return fBurnIn;
}

void
FitConfig::SetBurnIn(int b_){
  fBurnIn = b_;
}

void
FitConfig::AddParameter(const std::string& name_, double min_, double max_, double sigma_, int nbins_){
  fMinima[name_] = min_;
  fMaxima[name_] = max_;
  fSigmas[name_] = sigma_;
  fNbins[name_] = nbins_;
}

std::set<std::string>
FitConfig::GetParamNames() const{
  return ContainerTools::GetKeys(fMinima);
}

const std::string&
FitConfig::GetOutDir() const{
    return fOutDir;
}

void
FitConfig::SetOutDir(const std::string& s_){
  fOutDir = s_;
}

}
