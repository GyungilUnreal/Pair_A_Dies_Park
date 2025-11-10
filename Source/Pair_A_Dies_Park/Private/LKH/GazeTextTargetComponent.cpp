#include "GazeTextTargetComponent.h"

UGazeTextTargetComponent::UGazeTextTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 기본 문구를 하나 넣어둘 수도 있음
	DisplayText = FText::FromString(TEXT("Interact"));
	TextWidgetName = NAME_None;
}
