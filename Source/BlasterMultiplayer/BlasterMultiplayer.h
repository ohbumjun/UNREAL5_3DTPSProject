// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// 맨처음 추가한 Custom Collision Channel 은 ECC_GameTraceChannel1 에 들어간다.
// - 조금 더 descriptive 하게 명시해주기 위해 #define 을 사용할 것이다.
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1