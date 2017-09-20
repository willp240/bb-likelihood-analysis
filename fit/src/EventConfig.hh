#ifndef __BBFIT__EventConfig__
#define __BBFIT__EventConfig__
#include <string>
#include <vector>

namespace bbfit{
class EventConfig{
public:
  EventConfig(): fRate(-1), fNgenerated(0) {}

  double GetRate() const;
  void   SetRate(double);

  int GetNGenerated() const;
  void SetNGenerated(int);

  std::string GetName() const;
  void SetName(const std::string&);

  const std::vector<std::string>& GetNtupFiles() const;
  void SetNtupFiles(const std::vector<std::string>&);

  std::string GetTexLabel() const;
  void SetTexLabel(const std::string&);

  std::string GetNtupBaseDir() const;
  void SetNtupBaseDir(const std::string&);

  std::string GetPrunedPath() const;
  void SetPrunedPath(const std::string&);

  std::string GetMCSplitPath() const;
  void SetMCSplitPath(const std::string&);

  bool GetRandomSplit() const;
  void SetRandomSplit(bool);

private:
  double fRate;
  int    fNgenerated;
  std::vector<std::string> fNtupFiles;
  std::string fNtupBaseDir; // the originals
  std::string fPrunedPath;   // the pruned ouput
  std::string fMCSplitPath;   // after the fake data split
  std::string fTexLabel;
  std::string fName;
  bool fRandomSplit;
};
}

#endif
