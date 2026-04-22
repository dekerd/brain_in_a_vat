// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVCaptureAnnouncementWidget.generated.h"

class UTextBlock;

/**
 * 화면 중앙에 거점 점령 결과 메시지를 표시하는 위젯.
 * 예: "아군이 거점 XXX을 점령하였습니다."
 *
 * WBP BindWidget 요구:
 *   - MessageText (UTextBlock)
 *
 * 표시 → 3초 자동 숨김 흐름은 PlayerController가 관리.
 * WBP에서 페이드 인/아웃 애니메이션을 추가하고 싶으면 해당 UMG Animation을
 * "FadeIn" / "FadeOut" 이름으로 만들면 이 클래스에서 자동 재생.
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVCaptureAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 메시지 텍스트 블록 (필수).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	// 메시지 세팅 + 페이드인 애니메이션(있으면) 재생.
	UFUNCTION(BlueprintCallable, Category = "Capture Announcement")
	void SetMessage(const FText& InMessage);

	// 페이드 아웃 애니메이션(있으면) 재생. 애니메이션 종료 시 자동 숨김.
	// 애니메이션 없으면 즉시 숨김.
	UFUNCTION(BlueprintCallable, Category = "Capture Announcement")
	void PlayFadeOut();

protected:
	// FadeOut 애니메이션이 끝나면 위젯을 숨기기 위해 UUserWidget의 콜백을 오버라이드.
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	// (선택) WBP에서 같은 이름의 Animation을 만들면 자동 바인딩.
	UPROPERTY(Transient, meta = (BindWidgetAnim, OptionalWidget = true))
	TObjectPtr<class UWidgetAnimation> FadeIn;

	UPROPERTY(Transient, meta = (BindWidgetAnim, OptionalWidget = true))
	TObjectPtr<class UWidgetAnimation> FadeOut;
};
