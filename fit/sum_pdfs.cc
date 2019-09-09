#include <DistConfigLoader.hh>
#include <DistConfig.hh>
#include <DistBuilder.hh>
#include <EventConfig.hh>
#include <EventConfigLoader.hh>
#include <iostream>
#include <sys/stat.h>
#include <BinnedED.h>
#include <DistFiller.h>
#include <ROOTNtuple.h>
#include <IO.h>
#include <DistTools.h>
#include <TH1D.h>
#include <TH2D.h>
#include <AxisCollection.h>
#include <BinAxis.h>

#include <HistTools.h>
#include <iostream>
using namespace bbfit;

int main(int argc, char *argv[]){
  if (argc != 3){
    std::cout << "\nUsage: .\sum_pdfs <event_config_file> <pdf_config_file>" << std::endl;
    return 1;
  }
    
  std::string evConfigFile(argv[1]);
  std::string pdfConfigFile(argv[2]);


  std::cout << "\nReading from config files: "   << std::endl
	    << "\t" << evConfigFile << ",\n "  
	    << "\t" << pdfConfigFile
	    << std::endl;

    
  // load up the pdfs
  DistConfigLoader pLoader(pdfConfigFile);
  DistConfig pConfig = pLoader.Load();
  std::string pdfDir = pConfig.GetPDFDir();
	std::string sumDir = pdfDir + "/summed_pdfs";
  struct stat st = {0};
  if (stat(sumDir.c_str(), &st) == -1) {
    mkdir(sumDir.c_str(), 0700);
  }

  std::cout << "\nSaving pdfs to " << sumDir << std::endl;

  // and another one for the projections - there will be loads
  std::string projDir = sumDir + "/projections";
  if (stat(projDir.c_str(), &st) == -1) {
    mkdir(projDir.c_str(), 0700);
  }
  
  std::cout << "\nSaving projections logs to " << projDir << std::endl;

  // load up all the event types we want pdfs for
	std::vector<BinnedED> dists;
	
	
  typedef std::map<std::string, EventConfig> EvMap;
  EventConfigLoader loader(evConfigFile);
  EvMap toGet = loader.LoadActive();

 
  // now make and fill the pdfs
  for(EvMap::iterator it = toGet.begin(); it != toGet.end(); ++it){    
    std::cout << "Retrieving distribution for " << it->first << std::endl;
    std::string distPath = pdfDir + "/" + it->first + ".h5";
		
		BinnedED dist = BinnedED(it->first, IO::LoadHistogram(distPath));
		dists.push_back(dist);

		double energyLowPSD = 2.3;
		double energyHighPSD = 2.8;
		
		
		
		//sum across PSD in energy outside of ROI
		//get the boundary energy bins
		AxisCollection axCol = dist.GetAxes();
		BinAxis axE = axCol.GetAxis(0);
		size_t lowEBin = axE.FindBin(energyLowPSD);
		size_t highEBin = axE.FindBin(energyHighPSD);

		for(size_t iEBin =0; iEBin< 48; iEBin++){
			if ((iEBin>lowEBin) && (iEBin<highEBin)) continue;
			
			for(size_t iRBin=0; iRBin<6; iRBin++){

				
				//get contents and summ
				double summedBinContent=0;
				for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
					for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						summedBinContent += dist.GetBinContent(index);
					}
				}
				summedBinContent /=25;

				//replace
				for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
					for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						dist.SetBinContent(index, summedBinContent);
					}
				}

			}
		}

		//in ROI, ignore running of PSD with energy in radial slices 
		//get the boundary energy bins
		AxisCollection axCol = dist.GetAxes();
		BinAxis axE = axCol.GetAxis(0);
		size_t lowEBin = axE.FindBin(energyLowPSD);
		size_t highEBin = axE.FindBin(energyHighPSD);

		for(size_t iRBin=0; iRBin<6; iRBin++){
			for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
				for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){

					//get contents and summ
					double summedBinContent=0;
					double summedBins =0;
					//only in roi
					for(size_t iEBin =0; iEBin< 48; iEBin++){
						if ((iEBin<=lowEBin) && (iEBin>=highEBin)) continue;
						
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						summedBinContent += dist.GetBinContent(index);
						summedBins++;
					}
				}
				summedBinContent/=summedBins;

				//replace
				for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
					for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						dist.SetBinContent(index, summedBinContent);
					}
				}

			}
		}


/*}



	
	std::map<size_t, double> sumBinMap;	
	//sum PSD across background groups
	for(size_t iEBin =0; iEBin< 48; iEBin++){
		for(size_t iRBin=0; iRBin<6; iRBin++){
			for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
				for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
					double summedBinContent = 0; 
					for(std::vector<BinnedED>::iterator it = dists.begin(); it != dists.end(); ++it){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						summedBinContent += dist.GetBinContent(index);
					}
					
					sumBinMap.insert(std::pair<size_t, double>(index, summedBinContent));
				}
			}
		}
	}

	//get integrals to normalise back
	for(std::vector<BinnedED>::iterator it = dists.begin(); it != dists.end(); ++it){
		BinnedED dist = *it;
		for(size_t iEBin =0; iEBin< 48; iEBin++){
			for(size_t iRBin=0; iRBin<6; iRBin++){
				for(size_t iPSD1Bin=0; iPSD1Bin<5; iPSD1Bin++){
					double integralBefore=0;
					double integralAfter=0;
					for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						integralBefore += dist.GetBinContent(index);
						integralAfter += sumBinMap[index];
					}
					std::cout << "integral before: " << integralBefore << std::endl;
					std::cout << "integral after: " << integralAfter << std::endl;

					double correctedIntegral =0;
					for(size_t iPSD2Bin=0; iPSD2Bin<5; iPSD2Bin++){
						std::vector<size_t> indexVec;
						indexVec.push_back(iEBin);
						indexVec.push_back(iRBin);
						indexVec.push_back(iPSD1Bin);
						indexVec.push_back(iPSD2Bin);
						size_t index = dist.FlattenIndices(indexVec);
						newContent = summedBinMap[index]*(integralBefore/integralAfter);
						dist.SetBinContent(index, newContent);
						correctedIntegral+=newContent;
					}
					std::cout << "integral correctd: " << correctedIntegral << std::endl;
				}
			}
		}
*/	

	// normalise 
		std::cout<< "Integral" << dist.Integral() << std::endl;	
		if(dist.Integral()){
			std::cout<< "Normalising" << std::endl;	
			dist.Normalise();
		}
	

		// detect zero bins
		int zeroBins = 0;
		for(int i = 0; i<dist.GetNBins(); i++){
			if (!dist.GetBinContent(i)){
				//std::cout << "content "<< dist.GetBinContent(i) << " for bin " << i << std::endl;
				zeroBins++;
			}
		}
		std::cout<< "Zero bins in dist " << it->first << " is " << zeroBins << std::endl;
		
    // save as h5
    IO::SaveHistogram(dist.GetHistogram(), sumDir + "/" + it->first + ".h5");

    // save as a root histogram if possible
    if(dist.GetNDims() <= 2)
        IO::SaveHistogram(dist.GetHistogram(), sumDir + "/" + it->first + ".root");

    // HigherD save the projections
    if(dist.GetNDims() > 1){
      std::vector<BinnedED> projs = HistTools::GetVisualisableProjections(dist);

      // save them as apropriate
      for(size_t i = 0; i < projs.size(); i++){
          const BinnedED& proj = projs.at(i);

          if(projs.at(i).GetNDims() == 1)
              DistTools::ToTH1D(proj).SaveAs((projDir + "/" + proj.GetName() + ".root").c_str());
          else
              DistTools::ToTH2D(proj).SaveAs((projDir + "/" + proj.GetName() + ".root").c_str());
      }
    }
  }

  return 0;
}
