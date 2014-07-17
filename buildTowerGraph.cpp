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
#define pi 3.14159265358979323846
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
		TFlt mapping = TFlt(int(100000*(-7*(Ss.GetFlt(1)-13) + 29*(Ss.GetFlt(2)-40))));
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
		TFlt lat1 = locToTower.GetDat(NI.GetId())[0];
		TFlt lon1 = locToTower.GetDat(NI.GetId())[1];		
		for (TUNGraph::TNodeI NI2 = G->BegNI(); NI2 < G->EndNI(); NI2++) 
		{
			if(NI.GetId() != NI2.GetId())
			{
				//Find distances
				TFlt lat2 = locToTower.GetDat(NI2.GetId())[0];
				TFlt lon2 = locToTower.GetDat(NI2.GetId())[1];	
				TFlt theta = lon1 - lon2;
				TFlt dist = sin(lat1*pi/180)*sin(lat2*pi/180) + cos(lat1*pi/180)*cos(lat2*pi/180)*cos(theta*pi/180);
				dist = acos(dist);
				dist = dist*180/pi;
				dist = dist*60*1.1515;
				distances.AddDat(dist, NI2.GetId());
			}
		}

		distances.SortByKey();	
		THash<TFlt, TFlt>::TIter closestNeighs = distances.BegI();
		int numneighs = 0;
		while(numneighs < 5)
		{
			//cout << NI.GetId() << ", " << closestNeighs.GetKey() << ", " << closestNeighs.GetDat() << "\n";
			G->AddEdge(NI.GetId(), closestNeighs.GetDat());
			numneighs++;
			closestNeighs.Next();
		}
		IAssert(G->IsOk());
		//printf("node id %d with degree %d\n", NI.GetId(), NI.GetDeg());
  	}


  	//Now, build matrix of phone calls
	int numtowers = towerCount;
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

	int height = 96;
	double adjA [height][numtowers];
	memset( adjA, 0, height*numtowers*sizeof(double) );
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
			//std::cout << a[i][j] << ' ';				
		}
		//std::cout << std::endl;
	}

	//Set each edge weight to a covariance matrix
	THash< TVec< TInt>, TFlt> edgeWeights;
	for (TUNGraph::TEdgeI EI = G->BegEI(); EI < G->EndEI(); EI++) 
	{
		//Find sample covariance between two vectors
		TInt id1 = towerNumber.GetDat(EI.GetSrcNId());
		TInt id2 = towerNumber.GetDat(EI.GetDstNId());
		TFlt mean1 = 0;
		TFlt mean2 = 0;
		for (int i = 0; i < height; ++i)
		{
			mean1 = mean1 + a[i][id1];
			mean2 = mean2 + a[i][id2];
		}
		mean1 = mean1/height;
		mean2 = mean2/height;

		TFlt sampleCov = 0;
		TFlt var1 = 0;
		TFlt var2 = 0;
		for (int i = 0; i < height; ++i)
		{
			var1 += (a[i][id1] - mean1)*(a[i][id1] - mean1);
			var2 += (a[i][id2] - mean2)*(a[i][id2] - mean2);
			sampleCov += (a[i][id1] - mean1)*(a[i][id2] - mean2);
		}
		var1 = var1/(height - 1);
		var2 = var2/(height - 1);
		sampleCov = sampleCov / (height - 1);
		//cout << var1 << ", " << var2 << ", " << sampleCov << ", " << mean1 << ", " << mean2 << ", " << id1 << ", " << id2 << "\n";
		sampleCov = sampleCov / (var1*var2+0.001);

		TFlt weight = max(sampleCov, TFlt(0));

		printf("edge (%d, %d) with edge weight %f. Cov %f\n", EI.GetSrcNId(), EI.GetDstNId(), weight, sampleCov);
	}
















	// int adjA2 [numtowers];
	// memset( adjA2, 0, numtowers*sizeof(int) );
	// for (int i = 0; i < height; ++i)
	// {
	// 	for (int j = 0; j < numtowers; ++j)
	// 	{
	// 		adjA2[j] += a[i][j];
	// 	}
	// }

	// for (int j = 0; j < numtowers; ++j)
	// {
	// 	std::cout << adjA2[j] << ' ';	
	// }
	// cout << "\n";

	// cout << PhoneLoad.Len() << "\n";
	// for (int i=0; i < PhoneLoad.Len(); i++) 
	// {
	// 		TFlt sourceTow = towerLoc.GetDat(PhoneLoad[i].getLocSrc().CStr()); 
	// 		TFlt destTow = towerLoc.GetDat(PhoneLoad[i].getLocDest().CStr());		
	// 		if (sourceTow > 1 || sourceTow < -1)
	// 		{
	// 		//	cout << PhoneLoad[i].getLocSrc().CStr() << ", " << towerLoc.GetDat(PhoneLoad[i].getLocSrc()) << ", " << towerNumber.GetDat(towerLoc.GetDat(PhoneLoad[i].getLocSrc())) <<  "\n";
	// 		}
	// 		if (destTow > 1 || destTow < -1)
	// 		{
	// 			TFlt temperature = towerLoc.GetDat(PhoneLoad[i].getLocDest());
	// 			cout << PhoneLoad[i].getLocDest().CStr() << ", " << temperature << ", " << towerNumber.GetDat(temperature) <<  "\n";
	// 		}
	// }

	// cout << adjA2[0] << "\n";
	// THash<TFlt, TInt>::TIter NI3 = towerNumber.BegI();
	// while(!NI3.IsEnd())
	// {
	// 	//cout << NI3.GetKey() << ", " << NI3.GetDat() << "\n";
	// 	NI3.Next();
	// }

}




