// Copyright Inside Out Games Limited 2017

#include "VertWorldSettings.h"
#include "VertGlobals.h"

AVertWorldSettings::AVertWorldSettings()
{
	PlayerColours.Add(FPlayerColours(FLinearColor::Blue, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Green, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Red, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Yellow, FLinearColor::Gray, FLinearColor::Gray));
}