#include "OptimusDataInterfaceGetBoneMatrix.h"

#include "ComputeFramework/ComputeKernelPermutationVector.h"
#include "ComputeFramework/ShaderParamTypeDefinition.h"
#include "OptimusDataDomain.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "ShaderParameterMetadataBuilder.h"
#include "SkeletalMeshDeformerHelpers.h"
#include "SkeletalRenderPublic.h"
#include "RenderGraphBuilder.h" 
#include "Engine/SkeletalMesh.h" 
#include "Rendering/SkeletalMeshModel.h" 
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Map.h"

#include "Templates/Invoke.h"

// ---------------------------------------------------------------------------

FString UOptimusDataInterfaceGetBoneMatrix::GetDisplayName() const
{
	return TEXT("GetBoneMatrix_BonusNode");
}

TArray<FOptimusCDIPinDefinition> UOptimusDataInterfaceGetBoneMatrix::GetPinDefinitions() const
{
	TArray<FOptimusCDIPinDefinition> Defs;	
	Defs.Add({ "BoneMatrix", "ReadBoneMatrix" });	
	return Defs;
}


TSubclassOf<UActorComponent> UOptimusDataInterfaceGetBoneMatrix::GetRequiredComponentClass() const
{
	return USkinnedMeshComponent::StaticClass();
}


void UOptimusDataInterfaceGetBoneMatrix::GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const
{
	OutFunctions.AddDefaulted_GetRef()
		.SetName(TEXT("ReadBoneMatrix"))
		.AddReturnType(EShaderFundamentalType::Float, 4, 4);
}

BEGIN_SHADER_PARAMETER_STRUCT(FGetBoneMatrixDataInterfaceParameters, )
SHADER_PARAMETER(FMatrix44f, BoneMatrix)
END_SHADER_PARAMETER_STRUCT()

void UOptimusDataInterfaceGetBoneMatrix::GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const
{
	InOutBuilder.AddNestedStruct<FGetBoneMatrixDataInterfaceParameters>(UID);	
}

void UOptimusDataInterfaceGetBoneMatrix::GetPermutations(FComputeKernelPermutationVector& OutPermutationVector) const
{

}

void UOptimusDataInterfaceGetBoneMatrix::GetShaderHash(FString& InOutKey) const
{
	GetShaderFileHash(TEXT("/Plugin/DeformerGraphBonusTools/Private/DataInterfaceGetBoneMatrix.ush"), EShaderPlatform::SP_PCD3D_SM5).AppendString(InOutKey);
}

void UOptimusDataInterfaceGetBoneMatrix::GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const
{
	TMap<FString, FStringFormatArg> TemplateArgs =
	{
		{ TEXT("DataInterfaceName"), InDataInterfaceName },
	};

	FString TemplateFile;
	LoadShaderSourceFile(TEXT("/Plugin/DeformerGraphBonusTools/Private/DataInterfaceGetBoneMatrix.ush"), EShaderPlatform::SP_PCD3D_SM5, &TemplateFile, nullptr);
	OutHLSL += FString::Format(*TemplateFile, TemplateArgs);
}

UComputeDataProvider* UOptimusDataInterfaceGetBoneMatrix::CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const
{
	UOptimusDataProviderGetBoneMatrix* Provider = NewObject<UOptimusDataProviderGetBoneMatrix>();
	
	Provider->SkinnedMesh = Cast<USkinnedMeshComponent>(InBinding);


	// GameThread, called at initialization.
	// fetching transformations here won't get them animated since
	// this function is not called as part of the rendering loop
	// Don't base precomputation on the current LOD level it will be wrong.

	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(InBinding);
	if (SkeletalMesh == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("Can't find skeletal mesh"));
		return Provider;
	}
	
	Provider->SkeletalMesh = SkeletalMesh;
	Provider->BoneIndex = SkeletalMesh->GetBoneIndex(BoneName); 

	if (Provider->BoneIndex < 0) {
		UE_LOG(LogTemp, Warning, TEXT("GetBoneMatrix node: can't find joint: %s"), *BoneName.ToString());
	}
	
	Provider->bInverse = bInverse; 	
	
	return Provider;
}


bool UOptimusDataProviderGetBoneMatrix::IsValid() const
{
	return
		SkinnedMesh != nullptr &&
		SkinnedMesh->MeshObject != nullptr;
}

FComputeDataProviderRenderProxy* UOptimusDataProviderGetBoneMatrix::GetRenderProxy()
{	
	// GameThread, every tick.

	//FMatrix44f BoneMatrix  = FMatrix44f(SkeletalMesh->GetBoneMatrix(BoneIndex));
	// Should be equivalent to the above:
	
    // Get Joint global (world) transformation:
	FMatrix44f BoneMatrix = FMatrix44f(SkeletalMesh->GetBoneTransform(BoneIndex).ToMatrixWithScale());
	if (bInverse)
		BoneMatrix = BoneMatrix.Inverse();

	return new UOptimusDataProviderProxyGetBoneMatrix(
		SkinnedMesh,		
		BoneMatrix);
}


UOptimusDataProviderProxyGetBoneMatrix::UOptimusDataProviderProxyGetBoneMatrix(
	USkinnedMeshComponent* SkinnedMeshComponent, FMatrix44f BoneMatrix_)
	: BoneMatrix(BoneMatrix_)
{
	SkeletalMeshObject = SkinnedMeshComponent->MeshObject;
	BoneRevisionNumber = SkinnedMeshComponent->GetBoneTransformRevisionNumber();
}


void UOptimusDataProviderProxyGetBoneMatrix::AllocateResources(FRDGBuilder& GraphBuilder)
{


}

void UOptimusDataProviderProxyGetBoneMatrix::GatherDispatchData(
		FDispatchSetup const& InDispatchSetup, 
		FCollectedDispatchData& InOutDispatchData)
{
	if (!ensure(InDispatchSetup.ParameterStructSizeForValidation == sizeof(FGetBoneMatrixDataInterfaceParameters)))
	{
		return;
	}

	const int32 LodIndex = SkeletalMeshObject->GetLOD();
	FSkeletalMeshRenderData const& SkeletalMeshRenderData = SkeletalMeshObject->GetSkeletalMeshRenderData();
	FSkeletalMeshLODRenderData const* LodRenderData = &SkeletalMeshRenderData.LODRenderData[LodIndex];
	if (!ensure(LodRenderData->RenderSections.Num() == InDispatchSetup.NumInvocations))
	{
		return;
	}	

	FRHIShaderResourceView* NullSRVBinding = GWhiteVertexBufferWithSRV->ShaderResourceViewRHI.GetReference();

	for (int32 InvocationIndex = 0; InvocationIndex < InDispatchSetup.NumInvocations; ++InvocationIndex)
	{
		FGetBoneMatrixDataInterfaceParameters* Parameters = (FGetBoneMatrixDataInterfaceParameters*)(InOutDispatchData.ParameterBuffer + InDispatchSetup.ParameterBufferOffset + InDispatchSetup.ParameterBufferStride * InvocationIndex);
		
		Parameters->BoneMatrix = BoneMatrix.GetTransposed();
	}
}