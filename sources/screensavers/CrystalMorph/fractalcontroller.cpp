//////////////////////////////////////////////////////////////////
// FRACTALCONTROLLER.CPP
//
//////////////////////////////////////////////////////////////////
#include "fractalcontroller.h"       // also includes glu and gl correctly

//For debug.. prints a fractaldata object info.
/*void PrintData(FractalData * data)
{
	printf("NumTransforms: %d\n",data->numTransforms);
	for(int i = 0 ; i < data->numTransforms; i++)
	{
		printf("Transform %d\n",i);
		printf("sx: %1.1f\tsy: %1.1f\tsz: %1.1f\n",
		data->transforms[i].scaling.x,
		data->transforms[i].scaling.y,
		data->transforms[i].scaling.z);

		printf("tx: %1.1f\tty: %1.1f\ttz: %1.1f\n",
		data->transforms[i].translation.x,
		data->transforms[i].translation.y,
		data->transforms[i].translation.z);

		printf("rx: %1.1f\try: %1.1f\trz: %1.1f\n",
		data->transforms[i].rotation.x,
		data->transforms[i].rotation.y,
		data->transforms[i].rotation.z);
	}
}*/

//Inits everything and loads preset 1 into the fractal
FractalController::FractalController(Fractal* fractal, FractalSettings * fracsettings)
{
	myFractal = fractal;
	activeFractalData = fractal->GetDataHandle();
  settings = fracsettings;

	myAnimating = false;
	myRatio = 1.0;
	mySpeed = 0.001;
	myCounter = 0;
	
	selectionOn = false;
	mySelectedTransform = 0;
	mySelectedXYZ = 0;

	GeneratePresets();
	SetDesiredToPreset(rand()%6);
	CopyFractalData(&myDesiredFractalData,&myStartingFractalData);
	UpdateFractalData();
}

//Toggles animation
void FractalController::SetAnimation(bool on)
{
	if(on)
	{
		myAnimating = true;
		CopyFractalData(&myDesiredFractalData,&myAnimationBase);
		StartMorph();
		myCounter = 0;
	}
	else
	{
		myAnimating = false;
	}
}

void FractalController::SetMorphSpeed(float time)
{
	mySpeed = 1.0/ (time * 60);
}

//Sets whether selection is on and what is being selected
void FractalController::SetSelection(bool on, int selection)
{
	if(on)
	{
		selectionOn = true;
		if(selection >= myDesiredFractalData.numTransforms)
			mySelectedTransform = 0;
		else if(selection < 0)
			mySelectedTransform = myDesiredFractalData.numTransforms -1;
		else
			mySelectedTransform = selection;
	}
	else
	{
		selectionOn = false;
	}

	myFractal->SetSelection(selectionOn, mySelectedTransform);
}

//Increments the appropriate XYZ of the current selected transform
void FractalController::IncrementSelectedXYZ(float dx, float dy, float dz)
{
	if(!selectionOn)
		return;
	SetAnimation(false);
	switch(mySelectedXYZ)
	{
		case 0:
			myDesiredFractalData.transforms[mySelectedTransform].translation.x += 0.05 * dx;
			myDesiredFractalData.transforms[mySelectedTransform].translation.y += 0.05 * dy;
			myDesiredFractalData.transforms[mySelectedTransform].translation.z += 0.05 * dz;
			break;
		case 1:
			myDesiredFractalData.transforms[mySelectedTransform].rotation.x += 1.5 * dx;
			myDesiredFractalData.transforms[mySelectedTransform].rotation.y += 1.5 * dy;
			myDesiredFractalData.transforms[mySelectedTransform].rotation.z += 1.5 * dz;
			break;
		default:
			myDesiredFractalData.transforms[mySelectedTransform].scaling.x *= (1.0+ 0.05 * dx);
			myDesiredFractalData.transforms[mySelectedTransform].scaling.y *= (1.0+ 0.05 * dy);
			myDesiredFractalData.transforms[mySelectedTransform].scaling.z *= (1.0+ 0.05 * dz);
			break;
	}
}

//Handles other keyboard shortcuts not handled in glFramework.cpp
void FractalController::HandleKeyBoardHit(char key)
{
	switch(key)
	{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0': SetAnimation(false);
			SetDesiredToPreset(key - '0');
			StartMorph();
			break;

		case 'l': SetAnimation(!myAnimating);
			break;
		case ';': mySpeed *= 1.1;
			break;
		case '\'': mySpeed /= 1.1;
			break;

		case ',': SetSelection(!selectionOn, mySelectedTransform);
			break;
		case '.': SetSelection(selectionOn, mySelectedTransform -1);
			break;
		case '/': SetSelection(selectionOn, mySelectedTransform+1);
			break;
		case 'q': mySelectedXYZ = 0;
			break;
		case 'a': mySelectedXYZ = 1;
			break;
		case 'z': mySelectedXYZ = 2;
			break;

		case 'w': IncrementSelectedXYZ(-1,0,0);
			break;
		case 'e': IncrementSelectedXYZ( 1,0,0);
			break;
		case 's': IncrementSelectedXYZ( 0,-1,0);
			break;
		case 'd': IncrementSelectedXYZ( 0,1,0);
			break;
		case 'x': IncrementSelectedXYZ( 0,0,-1);
			break;
		case 'c': IncrementSelectedXYZ( 0,0, 1);
			break;
		case 'r': IncrementSelectedXYZ(-1,-1,-1);
			break;
		case 't': IncrementSelectedXYZ(1,1,1);
			break;

	}

}
void MakeSierpinski(FractalData * data);
void MakeCube(FractalData * data, int type);
void MakeCantor(FractalData * data);
void MakeNiceRandomFractalData(FractalData * data, int transforms=-1);

//Generates the preset FractalData objects and loads them into the array.
void FractalController::GeneratePresets()
{
	myNumPresets = 7;
	
	for (int i = 0; i < MAX_PRESETS; i++)
	{
		presets[i].Init();	
	}

	MakeSierpinski(&(presets[0]));
	MakeCube(&(presets[1]), 0);
	MakeCube(&(presets[2]), 1);
	MakeCube(&(presets[3]), 2);
	MakeCube(&(presets[4]), 3);
	MakeCantor(&(presets[5]));
	MakeNiceRandomFractalData(&(presets[6]));
}

//Sets the desired fractal data to a certain preset for the morphings
void FractalController::SetDesiredToPreset(int i)
{

	if(i <0 || i >= myNumPresets)
		i = 0;

	if (i == 6)
	{
		MakeNiceRandomFractalData(&(presets[6]));		
	}
	CopyFractalData(&(presets[i]),&myDesiredFractalData);
}

void FractalController::SetToRandomFractal(int numTransforms)
{
  MakeNiceRandomFractalData(&(presets[6]),numTransforms);
  CopyFractalData(&(presets[6]),&myDesiredFractalData);
}

void FractalController::FindCutoff(int maxtransforms = 0)
{
	int transforms = maxtransforms > 0 ? maxtransforms : activeFractalData->numTransforms;
	int cutoff = 0;
	int numPrims = 1;
  while (numPrims*transforms < settings->iMaxObjects)
	{
		numPrims *= transforms;
		cutoff++;
	}
  if (cutoff > settings->iMaxDepth) cutoff = settings->iMaxDepth;
	myFractal->SetCutoff(cutoff);
}

//Inits the morphing process
void FractalController::StartMorph()
{
	myRatio = 0;
	int minTransforms, maxTransforms;
	FractalData * smallerData;

	CopyFractalData(activeFractalData,&myStartingFractalData);


	if(myStartingFractalData.numTransforms > myDesiredFractalData.numTransforms)
	{
		minTransforms = myDesiredFractalData.numTransforms;
		maxTransforms = myStartingFractalData.numTransforms;
		smallerData = &myDesiredFractalData;
	}
	else
	{
		minTransforms = myStartingFractalData.numTransforms;
		maxTransforms = myDesiredFractalData.numTransforms;
		smallerData = &myStartingFractalData;
	}

	for(int i = minTransforms; i < maxTransforms; i++)
	{
		smallerData->transforms[i].translation.x =
		smallerData->transforms[i].translation.y =
		smallerData->transforms[i].translation.z = 0.0;
		smallerData->transforms[i].rotation.x =
		smallerData->transforms[i].rotation.y =
		smallerData->transforms[i].rotation.z = 0.0;
		smallerData->transforms[i].scaling.x =
		smallerData->transforms[i].scaling.y =
		smallerData->transforms[i].scaling.z = 0.0;
	}
	FindCutoff(maxTransforms);
}

//Slightly modifies the desired fractal object in ways to create an aesthetically pleasing animation.
void FractalController::AnimateDesired()
{
	myCounter += mySpeed;
	int ind = 0;
	int numTrans = myDesiredFractalData.numTransforms;
	for(float i = 0; i < numTrans; i++)
	{
		ind = (int) i;
		float r = i / ((float)numTrans);
/*			myDesiredFractalData.transforms[ind].rotation.x = myAnimationBase.transforms[ind].rotation.x + 1000*myCounter*(0.1 + 0.4*i/numTrans);
			myDesiredFractalData.transforms[ind].rotation.y = myAnimationBase.transforms[ind].rotation.y + (float)(2*i-numTrans) / (float)numTrans* 1000*myCounter*(0.2 + 0.4*i/numTrans);
			myDesiredFractalData.transforms[ind].rotation.z = myAnimationBase.transforms[ind].rotation.z + 1000*myCounter*(0.16 + 0.4*i/numTrans);

			myDesiredFractalData.transforms[ind].translation.x
			= myAnimationBase.transforms[ind].translation.x *(0.1 + 1.0 * cos((2.1 + 0.5*i/numTrans)*myCounter));
			myDesiredFractalData.transforms[ind].translation.y
			= myAnimationBase.transforms[ind].translation.y *(0.2 + 0.9 * cos((4.2 - 1.4*i/numTrans)*myCounter));
			myDesiredFractalData.transforms[ind].translation.z
			= myAnimationBase.transforms[ind].translation.z *(0.9 - 0.2 * sin((6.3 - 2.4*i/numTrans)*myCounter));

			myDesiredFractalData.transforms[ind].scaling.x
			= myAnimationBase.transforms[ind].scaling.x * (1.0 - 0.2 * sin((1.1 - 0.5*i/numTrans)*myCounter));
			myDesiredFractalData.transforms[ind].scaling.y
			= myAnimationBase.transforms[ind].scaling.y * (1.0 + 0.2 * sin((2.2 + 1.4*i/numTrans)*myCounter));
			myDesiredFractalData.transforms[ind].scaling.z
			= myAnimationBase.transforms[ind].scaling.z * (1.0 - 0.2 * sin((4.3 - 2.4*i/numTrans)*myCounter));
*/
			myDesiredFractalData.transforms[ind].rotation.x = myAnimationBase.transforms[ind].rotation.x + myCounter*(0.1 + 0.2*i/numTrans - 0.3*((3*ind)%7)-2);
			myDesiredFractalData.transforms[ind].rotation.y = myAnimationBase.transforms[ind].rotation.y + (float)(2*i-numTrans) / (float)numTrans* myCounter*(0.2 + 0.4*i/numTrans);
			myDesiredFractalData.transforms[ind].rotation.z = myAnimationBase.transforms[ind].rotation.z + myCounter*(0.16 + 0.4*i/numTrans - 0.4*((5*ind)%9)-4);
      myDesiredFractalData.transforms[ind].rotation.x -= (int)(myDesiredFractalData.transforms[ind].rotation.x/(2.0f*M_PI)) * 2.0f*M_PI;
      myDesiredFractalData.transforms[ind].rotation.y -= (int)(myDesiredFractalData.transforms[ind].rotation.y/(2.0f*M_PI)) * 2.0f*M_PI;
      myDesiredFractalData.transforms[ind].rotation.z -= (int)(myDesiredFractalData.transforms[ind].rotation.z/(2.0f*M_PI)) * 2.0f*M_PI;


			myDesiredFractalData.transforms[ind].translation.x
			= myAnimationBase.transforms[ind].translation.x *0.5 + 0.7*(2*(ind%2)-1) * sin((0 + 0.5*r)*myCounter/10.0);
			myDesiredFractalData.transforms[ind].translation.y
			= myAnimationBase.transforms[ind].translation.y *0.6 + 0.7*(2*(ind%2)-1) * sin((0 - 1.4*r)*myCounter/10.0);
			myDesiredFractalData.transforms[ind].translation.z
			= myAnimationBase.transforms[ind].translation.z *0.9 - 0.2*(2*(ind%2)-1) * sin((0 - 2.4*r)*myCounter/10.0);

			myDesiredFractalData.transforms[ind].scaling.x
			= myAnimationBase.transforms[ind].scaling.x * (1.0 + 0.3*(2*(ind%2)-1)* sin((r - 0.49*(0.2 + r)*myCounter)));
			myDesiredFractalData.transforms[ind].scaling.y
			= myAnimationBase.transforms[ind].scaling.y * (1.0 + 0.3*(2*(ind%2)-1)* sin((r - 0.5*(0.2 + r)*myCounter)));
			myDesiredFractalData.transforms[ind].scaling.z
			= myAnimationBase.transforms[ind].scaling.z * (1.0 + 0.3*(2*(ind%2)-1)* sin((r - 0.51*(0.2 + r)*myCounter)));

	}


}


//Increments the fractal if necessary between its initial state and desired state for morphing.
void FractalController::UpdateFractalData()
{
	if(myAnimating)
	{
		AnimateDesired();
	}
	if(myRatio >= 1.0)
	{
    for (int i=0; i < myDesiredFractalData.numTransforms; i++)
      myDesiredFractalData.transforms[i].color = activeFractalData->transforms[i].color;

    myRatio = 1.0;
		CopyFractalData(&myDesiredFractalData, activeFractalData);
		FindCutoff();
	}
	else
	{
		LerpToDesired(myRatio);
		myRatio += mySpeed;
	}

}

//Lerps between initial and desired and puts the result in the fractals data
void FractalController::LerpToDesired(float ratio)
{
	int maxTransforms = myStartingFractalData.numTransforms;
	int i;
	if(myDesiredFractalData.numTransforms > myStartingFractalData.numTransforms)
	{
		maxTransforms = myDesiredFractalData.numTransforms;
	}

	activeFractalData->numTransforms = maxTransforms;
	activeFractalData->base = myDesiredFractalData.base;
	for(i=0; i<maxTransforms; i++)
	{
		LerpXYZ(ratio,
				&(myStartingFractalData.transforms[i].translation),
				&(myDesiredFractalData.transforms[i].translation),
				&(activeFractalData->transforms[i].translation));
		LerpXYZ(ratio,
				&(myStartingFractalData.transforms[i].scaling),
				&(myDesiredFractalData.transforms[i].scaling),
				&(activeFractalData->transforms[i].scaling));
		LerpXYZ(ratio,
				&(myStartingFractalData.transforms[i].rotation),
				&(myDesiredFractalData.transforms[i].rotation),
				&(activeFractalData->transforms[i].rotation));
	}

}

//Lerps betweeen two XYZ values
void FractalController::LerpXYZ(float ratio, XYZ *src1, XYZ *src2, XYZ *dst)
{
	dst->x = src1->x * (1-ratio) + src2->x * ratio;
	dst->y = src1->y * (1-ratio) + src2->y * ratio;
	dst->z = src1->z * (1-ratio) + src2->z * ratio;
}

//Copies one fractal data to another
void FractalController::CopyFractalData(FractalData *src, FractalData *dst)
{
	dst->numTransforms = src->numTransforms;
	dst->base = src->base;
	for(int i=0; i< src->numTransforms; i++)
	{
		dst->transforms[i] = src->transforms[i];
	}
}

//Makes the sierpinkski preset
void MakeSierpinski(FractalData * data)
{
	data->numTransforms = 4;
	int i;
	for(i = 0; i < 4; i++)
	{
		data->transforms[i].scaling.x = data->transforms[i].scaling.y = data->transforms[i].scaling.z = 0.5;
	}
	data->transforms[0].translation.x  = data->transforms[0].translation.y = data->transforms[0].translation.z =
	data->transforms[1].translation.x  = data->transforms[1].translation.y = data->transforms[1].translation.z =
	data->transforms[2].translation.x  = data->transforms[2].translation.y = data->transforms[2].translation.z =
	data->transforms[3].translation.x  = data->transforms[3].translation.y = data->transforms[3].translation.z = 0.5;

	data->transforms[1].translation.y  = data->transforms[1].translation.z =
	data->transforms[2].translation.x  = data->transforms[2].translation.z =
	data->transforms[3].translation.x  = data->transforms[3].translation.y = -0.5;

	data->base = FRACTAL_BASE_PYRAMID;
}

//Makes the cantor preset
void MakeCantor(FractalData * data)
{
	data->numTransforms = 2;

	data->transforms[0].scaling.y = 1/3.0;
	data->transforms[1].scaling.y = 1/3.0;
	
	data->transforms[0].translation.y = 1.0; 
	data->transforms[1].translation.y = -1.0; 

	data->base = FRACTAL_BASE_CUBE;
}

//Makes the various cubic presets.
void MakeCube(FractalData * data, int type)
{
	switch(type)
	{
		default:
		case 0:	data->numTransforms = 20; break;
		case 1:	data->numTransforms = 13; break;
		case 2:	data->numTransforms = 9; break;
		case 3:	data->numTransforms = 8; break;
	}
	int i,j,k;
	for(i = 0; i < data->numTransforms; i++)
	{
		data->transforms[i].scaling.x = data->transforms[i].scaling.y = data->transforms[i].scaling.z = 1/3.0;
		data->transforms[i].rotation.x = data->transforms[i].rotation.y = data->transforms[i].rotation.z = 0;
	}
	int index = 0;
	bool addTransform;
	for(i=-1;i<2;i++)
	for(j=-1;j<2;j++)
	for(k=-1;k<2;k++)
	{
		switch(type)
		{
			default:
			case 0:	addTransform = (i != 0 && j != 0) || (i != 0 && k != 0) || (j != 0 && k != 0); break;
			case 1:	addTransform = ((i+j+k+8) % 2) == 0; break;
			case 2:	addTransform = i*i==j*j && j*j==k*k; break;
			case 3:	addTransform = i*i==j*j && j*j==k*k && i!=0; break;

		}

		if( addTransform)
		{
			data->transforms[index].translation.x = ((float) i);
			data->transforms[index].translation.y = ((float) j);
			data->transforms[index].translation.z = ((float) k);
			index++;

		}

	}
	data->base = FRACTAL_BASE_CUBE;
}

//float Rand(){ return (rand() /(RAND_MAX+1.0) );}

void MakeNiceRandomFractalData(FractalData * data, int transforms)
{
  if (transforms == -1)
	  data->numTransforms = 3+rand()%3;
  else 
    data->numTransforms = transforms;
	int index;
	for(index = 0; index < data->numTransforms; index++)
	{
		data->transforms[index].scaling.x = data->transforms[index].scaling.y = data->transforms[index].scaling.z = 0.5;
	}
	
	int current, place = 0;
	int check, checkflags = 0;
	for(index = 0; index < data->numTransforms; index++)
	{
		place = rand()%(8-index);
		current = 0;
		check = 0;
		for(float i=-0.5;i<1;i++)
		for(float j=-0.5;j<1;j++)
		for(float k=-0.5;k<1;k++)
		{
			if (!(checkflags & (1 << check)))
			if (current++ == place)
			{
				checkflags += 1 << check;
				
				data->transforms[index].translation.x = i;
				data->transforms[index].translation.y = j;
				data->transforms[index].translation.z = k;			
				
				data->transforms[index].rotation.x = (rand()%4) * M_PI/2;
				data->transforms[index].rotation.y = (rand()%4) * M_PI/2;
				data->transforms[index].rotation.z = (rand()%4) * M_PI/2;
			}	
			check++;
		}
	
	}

	data->base = rand()%2 ? FRACTAL_BASE_PYRAMID : rand()%2 ? FRACTAL_BASE_CUBE: FRACTAL_BASE_SPHERE;
}
