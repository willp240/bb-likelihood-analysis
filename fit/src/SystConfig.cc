#include <SystConfig.hh>
#include <ContainerTools.hpp>

namespace bbfit{

ParameterDict
SystConfig::GetNominal() const{
  return fNominal;
}

ParameterDict
SystConfig::GetMinima() const{
  return fMinima;
}

ParameterDict
SystConfig::GetMaxima() const{
  return fMaxima;
}

ParameterDict
SystConfig::GetMass() const{
  return fMass;
}

ParameterDict
SystConfig::GetNBins() const{
  return fNBins;
}

ParameterDict
SystConfig::GetConstrMean() const{
  return fConstrMean;
}

ParameterDict
SystConfig::GetConstrSigma() const{
  return fConstrSigma;
}

std::map<std::string, std::string>
SystConfig::GetObs() const{
  return fObs;
}

std::map<std::string, std::string>
SystConfig::GetType() const{
  return fType;
}

const std::string&
SystConfig::GetName() const {
  return fName;
}

void
SystConfig::AddParameter(const std::string& name_, double nom_, double min_, double max_, double mass_, int nbins_, const std::string& obs_, const std::string& type_){
  fNominal[name_] = nom_;
  fMinima[name_] = min_;
  fMaxima[name_] = max_;
  fMass[name_] = mass_;
  fNBins[name_] = nbins_;
  fObs[name_] = obs_;
  fType[name_] = type_;
}

void
SystConfig::AddParameter(const std::string& name_, double nom_, double min_, double max_, double mass_, int nbins_, double constr_mean_, double constr_sigma_, const std::string& obs_, const std::string& type_){
  fNominal[name_] = nom_;
  fMinima[name_] = min_;
  fMaxima[name_] = max_;
  fMass[name_] = mass_;
  fNBins[name_] = nbins_;
  fConstrMean[name_] = constr_mean_;
  fConstrSigma[name_] = constr_sigma_;
  fObs[name_] =obs_;
  fType[name_] = type_;
}
  
}
