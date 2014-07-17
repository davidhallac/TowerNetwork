//
//  towerAnalyze.cpp
//  YemenData
//
//  Created by David Hallac on 7/12/14.
//


#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include "../../snap-core/Snap.h"
#include "process.h"
#include <math.h>
using namespace std;

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		cout << "Not enough arguments!" << "\n";
		return 0;
	}
	//Load Phone Calls
	TVec<TPhoneCall> PhoneLoad;
	TFIn fin(argv[1]);
	PhoneLoad.Load(fin);

	//Get Tower Information from CSV File
	THash<TStr, TFlt> towerLoc; //Map ID --> Lat + 33*Long (unique hash)
	THash<TFlt, TInt> towerNumber; //Helper that maps towers to an in-order ID (0 to ~1100)
	THash<TFlt, TVec<TFlt> > locToTower;
	TSsParser Ss("LocationTowers.csv", ssfCommaSep);
	while(Ss.Next())
	{
		TFlt mapping = 100000*(-7*(Ss.GetFlt(1)-13) + 29*(Ss.GetFlt(2)-40));
		towerLoc.AddDat(Ss.GetFld(0), mapping);
		TVec<TFlt> temp;
		temp.Add(Ss.GetFlt(1));
		temp.Add(Ss.GetFlt(2));
		locToTower.AddDat(mapping, temp);
	}

	//Add all nodes (towers) to graph
	PUNGraph G = TUNGraph::New();
	towerLoc.SortByDat(); //Sorts towers
	THash<TStr, TFlt>::TIter NI = towerLoc.BegI();
	TInt towerCount = 0;
	while(!NI.IsEnd())
	{
		TFlt tow = NI.GetDat();
		if(!G->IsNode(tow))
		{
			G->AddNode(tow);
			towerNumber.AddDat(tow, towerCount);
			towerCount++;
		}
		NI.Next();
	}

	//For each node, find closest towers and add an edge:
	for (TUNGraph::TNodeI NI = G->BegNI(); NI < G->EndNI(); NI++) 
	{
		THash<TFlt, TFlt> distances;
		for (TUNGraph::TNodeI NI2 = G->BegNI(); NI2 < G->EndNI(); NI2++) 
		{
			if(NI.GetId() != NI2.GetId())
			{
				//TODO: Find distances
				TFlt dist = 0;

				distances.AddDat(dist, NI2.GetId());
			}
			distances.SortByKey();
			//TODO: Add 5 closest neighbors
			THash<TFlt, TFlt>::TIter fiveClosest = distances.BegI();
			
		}

		//printf("node id %d with degree %d\n", NI.GetId(), NI.GetDeg());
  	}

	int numtowers = towerCount;
	for (int e = 0; e < 10000; e++) {
    const int NId1 = G->GetRndNId();
    const int NId2 = G->GetRndNId();
    G->AddEdge(NId1, NId2);
	}
	IAssert(G->IsOk());

	int a [96][numtowers];
	memset( a, 0, 96*numtowers*sizeof(int) );
	for (int i=0; i < PhoneLoad.Len(); i++) 
	{

		//Look at calls only
		if(PhoneLoad[i].getDuration() > 1)
		{
			
			TFlt sourceTow = towerLoc.GetDat(PhoneLoad[i].getLocSrc().CStr()); 
			TFlt destTow = towerLoc.GetDat(PhoneLoad[i].getLocDest().CStr());
			
			if (sourceTow > 1 || sourceTow < -1)
			{
				a[PhoneLoad[i].getTime()/10000*4 + (PhoneLoad[i].getTime()%10000)/1500][towerNumber.GetDat(sourceTow)]++;

			}
			if (destTow > 1 || destTow < -1)
			{
				a[PhoneLoad[i].getTime()/10000*4 + (PhoneLoad[i].getTime()%10000)/1500][towerNumber.GetDat(destTow)]++;
			}

		}
	}

	double adjA [96][numtowers];
	memset( adjA, 0, 96*numtowers*sizeof(int) );
	int height = 96;
	for (int i = 0; i < height; ++i)
	{
		double sum = 0.01;
		for (int j = 0; j < numtowers; ++j)
		{
			sum = sum + a[i][j];
		}
		for (int j = 0; j < numtowers; j++)
		{
			adjA[i][j] = (a[i][j]*100 * numtowers)/sum;
			//std::cout << adjA[i][j] << ' ';				
		}
		//std::cout << std::endl;
	}

}




