#pragma once

// Blueprint 에서도 enum 형태로 사용 가능
UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	ETIP_Left   UMETA(DisplayName = "TurningLeft"), // DisplayName : 블루프린트 상에서 보이는 선택지 이름
	ETIP_Right UMETA(DisplayName = "TurningRight"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),
	ETIP_MAX UMETA(DisplayName = "DefaultMax")
};
