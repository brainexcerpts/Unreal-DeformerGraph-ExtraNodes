/*
	
	Upload to GPU the bone index associated to 'FName BoneName' 	
	(BoneName is a UPROPERTY which value should be set in the UI of the deformer graph by the user)
	
	BoneIndex is found through:
	USkeletalMeshComponent* SkeletalMesh;
	int32 BoneIndex = SkeletalMesh->GetBoneIndex(FName BoneName);

*/
#pragma once

#define TEMP_API

#include "OptimusComputeDataInterface.h"
#include "ComputeFramework/ComputeDataProvider.h"

#include "OptimusDataInterfaceGetBoneId.generated.h"

class FSkeletalMeshObject;
class USkinnedMeshComponent;


/** Compute Framework Data Interface for skeletal data. */
UCLASS(Category = ComputeFramework)
class TEMP_API UOptimusDataInterfaceGetBoneID : public UOptimusComputeDataInterface
{
	GENERATED_BODY()

public:
	//~ Begin UOptimusComputeDataInterface Interface
	FString GetDisplayName() const override;
	TArray<FOptimusCDIPinDefinition> GetPinDefinitions() const override;
	TSubclassOf<UActorComponent> GetRequiredComponentClass() const override;
	//~ End UOptimusComputeDataInterface Interface

	//~ Begin UComputeDataInterface Interface
	TCHAR const* GetClassName() const override { return TEXT("GetBoneId"); }
	void GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const override;
	void GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const override;
	void GetPermutations(FComputeKernelPermutationVector& OutPermutationVector) const override;
	void GetShaderHash(FString& InOutKey) const override;
	void GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const override;
	UComputeDataProvider* CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const override;
	//~ End UComputeDataInterface Interface

	UPROPERTY(EditAnywhere, Category = "Data Interface")
	FName BoneName = "*";
};


// ============================================================================
// ============================================================================


/** Compute Framework Data Provider for reading skeletal mesh. */
UCLASS(BlueprintType, editinlinenew, Category = ComputeFramework)
class UOptimusDataProviderGetBoneID : public UComputeDataProvider
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Binding)
	TObjectPtr<USkinnedMeshComponent> SkinnedMesh = nullptr;

	int32 BoneIndex = -1;

	//~ Begin UComputeDataProvider Interface
	// GameThread Tick/every frame
	bool IsValid() const override;
	// GameThread Tick/every frame
	FComputeDataProviderRenderProxy* GetRenderProxy() override;
	//~ End UComputeDataProvider Interface
};


// ============================================================================
// ============================================================================


class UOptimusDataProviderProxyGetBoneID : public FComputeDataProviderRenderProxy
{
public:
	UOptimusDataProviderProxyGetBoneID(USkinnedMeshComponent* SkinnedMeshComponent,
									   int32 BoneIndex);

	//~ Begin FComputeDataProviderRenderProxy Interface
	void AllocateResources(FRDGBuilder& GraphBuilder) override; 
	void GatherDispatchData(FDispatchSetup const& InDispatchSetup, FCollectedDispatchData& InOutDispatchData) override;
	//~ End FComputeDataProviderRenderProxy Interface

private:
	FSkeletalMeshObject* SkeletalMeshObject;
	uint32 BoneRevisionNumber = 0;
	
private:
	int32 BoneIndex;
};