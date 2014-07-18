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
	THash<TStr, TPair< TFlt, TFlt> > towerLoc;
	THash<TPair< TFlt, TFlt>, TInt> towerNumber;
	THash<TInt, TPair< TFlt, TFlt> > idToLoc;
	TSsParser Ss("LocationTowers.csv", ssfCommaSep);
	while(Ss.Next())
	{
		TPair< TFlt, TFlt> mapping;
		mapping.Val1 = Ss.GetFlt(1);
		// if(mapping.Val1 > 17.7947 && mapping.Val1 < 17.7969)
		// {
		// 	cout << Ss.GetFld(0) << "\n";
		// }
		mapping.Val2 = Ss.GetFlt(2);
		towerLoc.AddDat(Ss.GetFld(0), mapping);
	}

	//Add all nodes (towers) to graph
	PUNGraph G = TUNGraph::New();
	//towerLoc.SortByDat(); //Sorts towers
	THash<TStr, TPair< TFlt, TFlt> >::TIter NI = towerLoc.BegI();
	TInt towerCount = 0;
	while(!NI.IsEnd())
	{
		TPair< TFlt, TFlt> tow = NI.GetDat();
		if(!towerNumber.IsKey(tow))
		{
			G->AddNode(towerCount);
			towerNumber.AddDat(tow, towerCount);
			idToLoc.AddDat(towerCount, tow);
			towerCount++;
		}

		//if(!G->IsNode(tow))
		// {
		// 	G->AddNode(tow);
		// 	towerNumber.AddDat(tow, towerCount);
		// 	towerCount++;
		// }
		NI.Next();
	}

	TInt temperature = towerNumber.GetDat(towerLoc.GetDat("421022B16C795"));
	TFlt temperature2 = towerLoc.GetDat("421022B16C795").GetVal1();
	//cout << temperature << ", " << temperature2 << ", " << idToLoc(1118).GetVal1() << "\n";

	int totaledges = 0;
	//For each node, find closest towers and add an edge:
	for (TUNGraph::TNodeI NI = G->BegNI(); NI < G->EndNI(); NI++) 
	{
		THash<TFlt, TFlt> distances;
		TFlt lat1 = idToLoc.GetDat(NI.GetId()).GetVal1();
		TFlt lon1 = idToLoc.GetDat(NI.GetId()).GetVal2();		
		for (TUNGraph::TNodeI NI2 = G->BegNI(); NI2 < G->EndNI(); NI2++) 
		{
			if(NI.GetId() != NI2.GetId())
			{
				//Find distances
				TFlt lat2 = idToLoc.GetDat(NI2.GetId()).GetVal1();
				TFlt lon2 = idToLoc.GetDat(NI2.GetId()).GetVal2();	
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
			//G->AddEdge(NI.GetId(), closestNeighs.GetDat());
			if(!G->IsEdge(NI.GetId(), closestNeighs.GetDat()))
			{
				G->AddEdge(NI.GetId(), closestNeighs.GetDat());
			}
				totaledges++;
			numneighs++;
			closestNeighs.Next();
		}
		IAssert(G->IsOk());
		//printf("node id %d with degree %d\n", NI.GetId(), NI.GetDeg());

		// if(NI.GetId() == 1118)
		// //if(lat1 > 17.7957 && lat1 < 17.7959)
		// //if(lat1 > 17.7947 && lat1 < 17.7969)
		// {
		// 	cout << "FOUND IT: " << NI.GetId() << ": " << lat1 <<  "\n";
		// 	THash<TFlt, TFlt>::TIter closestNeighs2 = distances.BegI();
		// 	int numneighs = 0;
		// 	while(numneighs < 10)
		// 	{
		// 		cout << NI.GetId() << ", " << closestNeighs2.GetKey() << ", " << closestNeighs2.GetDat() << "\n";
		// 		numneighs++;
		// 		closestNeighs2.Next();
		// 	}
		// }


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
			
			TPair< TFlt, TFlt> sourceTow = towerLoc.GetDat(PhoneLoad[i].getLocSrc().CStr()); 
			TPair< TFlt, TFlt> destTow = towerLoc.GetDat(PhoneLoad[i].getLocDest().CStr());
			
			if (sourceTow.GetVal1() > 1 || sourceTow.GetVal1() < -1)
			{
				a[PhoneLoad[i].getTime()/10000*4 + (PhoneLoad[i].getTime()%10000)/1500][towerNumber.GetDat(sourceTow)]++;

			}
			if (destTow.GetVal1() > 1 || destTow.GetVal1() < -1)
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
	THash< TPair< TInt, TInt>,  TFlt> edgeWeights;
	for (TUNGraph::TEdgeI EI = G->BegEI(); EI < G->EndEI(); EI++) 
	{
		//Find sample covariance between two vectors
		TInt id1 = EI.GetSrcNId();//towerNumber.GetDat(EI.GetSrcNId());
		TInt id2 = EI.GetDstNId();//towerNumber.GetDat(EI.GetDstNId());
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
		sampleCov = sampleCov / (var1*var2+0.001);
		TFlt weight = max(sampleCov, TFlt(0));
		TPair< TInt, TInt> temp;
		temp.Val1 = EI.GetSrcNId();
		temp.Val2 = EI.GetDstNId();
		edgeWeights.AddDat(temp, weight);
		//cout << temp << ", " << weight << "\n";
		//printf("edge (%d, %d) with edge weight %f. Cov %f\n", EI.GetSrcNId(), EI.GetDstNId(), weight, sampleCov);
	}


	 //Remove edges with 0 weight
	for (TUNGraph::TNodeI NI = G->BegNI(); NI < G->EndNI(); NI++) 
	{
		int temp = NI.GetDeg();
		int tempCounter = 0;
		for(int j = 0; j < temp; j++)
		{
			TPair< TInt, TInt> temp;
			temp.Val1 = min(NI.GetId(),NI.GetOutNId(tempCounter));
			temp.Val2 = max(NI.GetId(), NI.GetOutNId(tempCounter));
			TFlt weight = edgeWeights.GetDat(temp); //TODO: SOLVE THIS
			//cout << temp << ", " << weight << "\n";
			if(double(weight) <= 0.00001)
			{
				G->DelEdge(NI.GetId(), NI.GetOutNId(tempCounter) );
			}
			else
			{
				tempCounter++;
			}
		}
	}

	//Remove nodes with no edges
	int numCounter [10];
	memset( numCounter, 0, 10*sizeof(int) );
	for (TUNGraph::TNodeI NI = G->BegNI(); NI < G->EndNI(); NI++) 
	{
		if(NI.GetDeg() == 9)
		{
			//cout << NI.GetId() << "= NODE ID" << "\n";
			int countcount = 0;
			for(int j = 0; j < NI.GetDeg(); j++)
			{
				if(G->GetNI(NI.GetOutNId(j)).GetDeg() == 9)
					//cout << NI.GetOutNId(j) << "\n";
					countcount++;
			}
			if(countcount == 0)
			{
				countcount = 0;
				for(int j = 0; j < NI.GetDeg(); j++)
				{
					if(G->GetNI(NI.GetOutNId(j)).GetDeg() == 8)
						countcount++;
				}
				numCounter[countcount]++;
				//if (countcount == 1)
				//	cout << NI.GetId() << "\n";
			}
		}
		//numCounter[NI.GetDeg()]++;
	}
	for(int j = 0; j<10; j++)
	{
		//cout << numCounter[j] << "\n";
	}



	int countt = 0;
	for (TUNGraph::TEdgeI EI = G->BegEI(); EI < G->EndEI(); EI++) 
	{
		countt++;
		//printf("edge (%d, %d) \n", EI.GetSrcNId(), EI.GetDstNId());
	}
	cout << countt << "\n";
	cout << TSnap::GetTriads(G) << "\n";
	cout << TSnap::GetClustCf(G) << "\n";

	//cout << idToLoc(584).GetVal1() << ", " << idToLoc(1115).GetVal1() << "\n";

}




