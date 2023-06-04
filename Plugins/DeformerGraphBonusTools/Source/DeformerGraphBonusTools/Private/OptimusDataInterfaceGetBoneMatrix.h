/*
	Upload to GPU the bone index associated to 'FName BoneName' 	
	(BoneName is a UPROPERTY which value should be set in the UI of the deformer graph by the user)
	
	BoneIndex is found through:
	USkeletalMeshComponent* SkeletalMesh;
	int32 BoneIndex = SkeletalMesh->GetBoneIndex(FName BoneName);
*/
#pragma once

#include "OptimusComputeDataInterface.h"
#include "ComputeFramework/ComputeDataProvider.h"

#include "OptimusDataInterfaceGetBoneMatrix.generated.h"

class FSkeletalMeshObject;
class USkinnedMeshComponent;


/** 
    Compute Framework Data Interface for skeletal data. 
    
    Thanks to Macro magic (UCLASS etc) which allows introspection, 
    this class will be automatically detected and added to the UI of the 
    Deformer graph editor.
    
    UPROPERTY will also show un in the details pannel on the right side.

*/
UCLASS(Category = ComputeFramework)
class DEFORMERGRAPHBONUSTOOLS_API UOptimusDataInterfaceGetBoneMatrix : public UOptimusComputeDataInterface
{
	GENERATED_BODY()

public:
	//~ Begin UOptimusComputeDataInterface Interface
	FString GetDisplayName() const override;
	TArray<FOptimusCDIPinDefinition> GetPinDefinitions() const override;
	TSubclassOf<UActorComponent> GetRequiredComponentClass() const override;
	//~ End UOptimusComputeDataInterface Interface

	//~ Begin UComputeDataInterface Interface
	TCHAR const* GetClassName() const override { return TEXT("GetBoneMatrix"); }
	void GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const override;
	void GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const override;
	void GetPermutations(FComputeKernelPermutationVector& OutPermutationVector) const override;
	void GetShaderHash(FString& InOutKey) const override;
	void GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const override;
	UComputeDataProvider* CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const override;
	//~ End UComputeDataInterface Interface

	UPROPERTY(EditAnywhere, Category = "Data Interface")
	FName BoneName = "*";
	UPROPERTY(EditAnywhere, Category = "Data Interface")
	bool bInverse = false;
};


// ============================================================================
// ============================================================================


/** Compute Framework Data Provider for reading skeletal mesh. */
UCLASS(BlueprintType, editinlinenew, Category = ComputeFramework)
class UOptimusDataProviderGetBoneMatrix : public UComputeDataProvider
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Binding)
	TObjectPtr<USkinnedMeshComponent> SkinnedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Binding)
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh = nullptr;
	
	uint32 BoneIndex;
	bool bInverse;

	//~ Begin UComputeDataProvider Interface
	// GameThread Tick/every frame
	bool IsValid() const override;
	// GameThread Tick/every frame
	FComputeDataProviderRenderProxy* GetRenderProxy() override;
	//~ End UComputeDataProvider Interface
};


// ============================================================================
// ============================================================================


class UOptimusDataProviderProxyGetBoneMatrix : public FComputeDataProviderRenderProxy
{
public:
	UOptimusDataProviderProxyGetBoneMatrix(USkinnedMeshComponent* SkinnedMeshComponent,
		FMatrix44f BoneMatrix);

	//~ Begin FComputeDataProviderRenderProxy Interface
	void AllocateResources(FRDGBuilder& GraphBuilder) override; 
	void GatherDispatchData(FDispatchSetup const& InDispatchSetup, FCollectedDispatchData& InOutDispatchData) override;
	//~ End FComputeDataProviderRenderProxy Interface

private:
	FSkeletalMeshObject* SkeletalMeshObject;
	uint32 BoneRevisionNumber = 0;
	
private:
	FMatrix44f BoneMatrix;	
};
