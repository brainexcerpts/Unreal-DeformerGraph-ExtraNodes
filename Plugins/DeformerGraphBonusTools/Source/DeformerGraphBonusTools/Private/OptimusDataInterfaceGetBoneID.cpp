#include "OptimusDataInterfaceGetBoneId.h"

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

FString UOptimusDataInterfaceGetBoneID::GetDisplayName() const
{
	return TEXT("GetBoneId_BonusNode");
}

TArray<FOptimusCDIPinDefinition> UOptimusDataInterfaceGetBoneID::GetPinDefinitions() const
{
	TArray<FOptimusCDIPinDefinition> Defs;
	Defs.Add({ "BoneId", "ReadBoneId" });
	return Defs;
}


TSubclassOf<UActorComponent> UOptimusDataInterfaceGetBoneID::GetRequiredComponentClass() const
{
	return USkinnedMeshComponent::StaticClass();
}


void UOptimusDataInterfaceGetBoneID::GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const
{	
	OutFunctions.AddDefaulted_GetRef()
		.SetName(TEXT("ReadBoneId"))
		.AddReturnType(EShaderFundamentalType::Uint);
}

BEGIN_SHADER_PARAMETER_STRUCT(FGetBoneIdDataInterfaceParameters, )
SHADER_PARAMETER(uint32, BoneId) 
END_SHADER_PARAMETER_STRUCT()

void UOptimusDataInterfaceGetBoneID::GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const
{
	InOutBuilder.AddNestedStruct<FGetBoneIdDataInterfaceParameters>(UID);
}

void UOptimusDataInterfaceGetBoneID::GetPermutations(FComputeKernelPermutationVector& OutPermutationVector) const
{

}

void UOptimusDataInterfaceGetBoneID::GetShaderHash(FString& InOutKey) const
{
	GetShaderFileHash(TEXT("/Plugin/DeformerGraphBonusTools/Private/DataInterfaceGetBoneId.ush"), EShaderPlatform::SP_PCD3D_SM5).AppendString(InOutKey);
}

void UOptimusDataInterfaceGetBoneID::GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const
{
	TMap<FString, FStringFormatArg> TemplateArgs =
	{
		{ TEXT("DataInterfaceName"), InDataInterfaceName },
	};

	FString TemplateFile;
	LoadShaderSourceFile(TEXT("/Plugin/DeformerGraphBonusTools/Private/DataInterfaceGetBoneId.ush"), EShaderPlatform::SP_PCD3D_SM5, &TemplateFile, nullptr);
	OutHLSL += FString::Format(*TemplateFile, TemplateArgs);
}

UComputeDataProvider* UOptimusDataInterfaceGetBoneID::CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const
{
	UOptimusDataProviderGetBoneID* Provider = NewObject<UOptimusDataProviderGetBoneID>();
	
	Provider->SkinnedMesh = Cast<USkinnedMeshComponent>(InBinding);


	// GameThread, called at initialization.
	// fetching transformations here won't get them animated since
	// this function is not called as part of the rendering loop
	// Don't base precomputation on the current LOD level it will be wrong.

	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(InBinding);
	if (SkeletalMesh == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("RODO Can't find skeletal mesh"));
		return Provider;
	}
	
	Provider->BoneIndex = SkeletalMesh->GetBoneIndex(BoneName); 

	return Provider;
}


bool UOptimusDataProviderGetBoneID::IsValid() const
{
	return
		SkinnedMesh != nullptr &&
		SkinnedMesh->MeshObject != nullptr;
}

FComputeDataProviderRenderProxy* UOptimusDataProviderGetBoneID::GetRenderProxy()
{	
	// GameThread, every tick.

	return new UOptimusDataProviderProxyGetBoneID(
		SkinnedMesh,		
		BoneIndex); 
}


UOptimusDataProviderProxyGetBoneID::UOptimusDataProviderProxyGetBoneID(
	USkinnedMeshComponent* SkinnedMeshComponent, int32 BoneIndex_)
	: BoneIndex(BoneIndex_)
{
	SkeletalMeshObject = SkinnedMeshComponent->MeshObject;
	BoneRevisionNumber = SkinnedMeshComponent->GetBoneTransformRevisionNumber();
}


void UOptimusDataProviderProxyGetBoneID::AllocateResources(FRDGBuilder& GraphBuilder)
{


}

void UOptimusDataProviderProxyGetBoneID::GatherDispatchData(
		FDispatchSetup const& InDispatchSetup, 
		FCollectedDispatchData& InOutDispatchData)
{
	if (!ensure(InDispatchSetup.ParameterStructSizeForValidation == sizeof(FGetBoneIdDataInterfaceParameters)))
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
		FGetBoneIdDataInterfaceParameters* Parameters = (FGetBoneIdDataInterfaceParameters*)(InOutDispatchData.ParameterBuffer + InDispatchSetup.ParameterBufferOffset + InDispatchSetup.ParameterBufferStride * InvocationIndex);
				
		Parameters->BoneId = BoneIndex;		
	}
}
