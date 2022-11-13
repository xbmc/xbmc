//--------------------------------------------------------------------------------------
// File: EffectRuntime.cpp
//
// Direct3D 11 Effect runtime routines (performance critical)
// These functions are expected to be called at high frequency
// (when applying a pass).
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#include "pchfx.h"

namespace D3DX11Effects
{
    // D3D11_KEEP_UNORDERED_ACCESS_VIEWS == (uint32_t)-1
    uint32_t g_pNegativeOnes[8] = { D3D11_KEEP_UNORDERED_ACCESS_VIEWS, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, D3D11_KEEP_UNORDERED_ACCESS_VIEWS,
                                D3D11_KEEP_UNORDERED_ACCESS_VIEWS, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, D3D11_KEEP_UNORDERED_ACCESS_VIEWS,
                                D3D11_KEEP_UNORDERED_ACCESS_VIEWS, D3D11_KEEP_UNORDERED_ACCESS_VIEWS };

bool SBaseBlock::ApplyAssignments(CEffect *pEffect)
{
    SAssignment *pAssignment = pAssignments;
    SAssignment *pLastAssn = pAssignments + AssignmentCount;
    bool bRecreate = false;

    for(; pAssignment < pLastAssn; pAssignment++)
    {
        bRecreate |= pEffect->EvaluateAssignment(pAssignment);
    }

    return bRecreate;
}

void SPassBlock::ApplyPassAssignments()
{
    SAssignment *pAssignment = pAssignments;
    SAssignment *pLastAssn = pAssignments + AssignmentCount;

    pEffect->IncrementTimer();

    for(; pAssignment < pLastAssn; pAssignment++)
    {
        pEffect->EvaluateAssignment(pAssignment);
    }
}

// Returns true if the shader uses global interfaces (since these interfaces can be updated through SetClassInstance)
bool SPassBlock::CheckShaderDependencies( _In_ const SShaderBlock* pBlock )
{
    if( pBlock->InterfaceDepCount > 0 )
    {
        assert( pBlock->InterfaceDepCount == 1 );
        for( size_t i=0; i < pBlock->pInterfaceDeps[0].Count; i++ )
        {
            SInterface* pInterfaceDep = pBlock->pInterfaceDeps[0].ppFXPointers[i];
            if( pInterfaceDep > pEffect->m_pInterfaces && pInterfaceDep < (pEffect->m_pInterfaces + pEffect->m_InterfaceCount) )
            {
                // This is a global interface pointer (as opposed to an SInterface created in a BindInterface call
                return true;
            }
        }
    }
    return false;
}

// Returns true if the pass (and sets HasDependencies) if the pass sets objects whose backing stores can be updated
#pragma warning(push)
#pragma warning(disable: 4616 6282)
bool SPassBlock::CheckDependencies()
{
    if( HasDependencies )
        return true;

    for( size_t i=0; i < AssignmentCount; i++ )
    {
        if( pAssignments[i].DependencyCount > 0 )
            return HasDependencies = true;
    }
    if( BackingStore.pBlendBlock && BackingStore.pBlendBlock->AssignmentCount > 0 )
    {
        for( size_t i=0; i < BackingStore.pBlendBlock->AssignmentCount; i++ )
        {
            if( BackingStore.pBlendBlock->pAssignments[i].DependencyCount > 0 )
                return HasDependencies = true;
        }
    }
    if( BackingStore.pDepthStencilBlock && BackingStore.pDepthStencilBlock->AssignmentCount > 0 )
    {
        for( size_t i=0; i < BackingStore.pDepthStencilBlock->AssignmentCount; i++ )
        {
            if( BackingStore.pDepthStencilBlock->pAssignments[i].DependencyCount > 0 )
                return HasDependencies = true;
        }
    }
    if( BackingStore.pRasterizerBlock && BackingStore.pRasterizerBlock->AssignmentCount > 0 )
    {
        for( size_t i=0; i < BackingStore.pRasterizerBlock->AssignmentCount; i++ )
        {
            if( BackingStore.pRasterizerBlock->pAssignments[i].DependencyCount > 0 )
                return HasDependencies = true;
        }
    }
    if( BackingStore.pVertexShaderBlock && CheckShaderDependencies( BackingStore.pVertexShaderBlock ) )
    {
        return HasDependencies = true;
    }
    if( BackingStore.pGeometryShaderBlock && CheckShaderDependencies( BackingStore.pGeometryShaderBlock ) )
    {
        return HasDependencies = true;
    }
    if( BackingStore.pPixelShaderBlock && CheckShaderDependencies( BackingStore.pPixelShaderBlock ) )
    {
        return HasDependencies = true;
    }
    if( BackingStore.pHullShaderBlock && CheckShaderDependencies( BackingStore.pHullShaderBlock ) )
    {
        return HasDependencies = true;
    }
    if( BackingStore.pDomainShaderBlock && CheckShaderDependencies( BackingStore.pDomainShaderBlock ) )
    {
        return HasDependencies = true;
    }
    if( BackingStore.pComputeShaderBlock && CheckShaderDependencies( BackingStore.pComputeShaderBlock ) )
    {
        return HasDependencies = true;
    }

    return HasDependencies;
}
#pragma warning(pop)

// Update constant buffer contents if necessary
inline void CheckAndUpdateCB_FX(ID3D11DeviceContext *pContext, SConstantBuffer *pCB)
{
    if (pCB->IsDirty && !pCB->IsNonUpdatable)
    {
        // CB out of date; rebuild it
        pContext->UpdateSubresource(pCB->pD3DObject, 0, nullptr, pCB->pBackingStore, pCB->Size, pCB->Size);
        pCB->IsDirty = false;
    }
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

// Set the shader and dependent state (SRVs, samplers, UAVs, interfaces)
void CEffect::ApplyShaderBlock(_In_ SShaderBlock *pBlock)
{
    SD3DShaderVTable *pVT = pBlock->pVT;

    // Apply constant buffers first (tbuffers are done later)
    SShaderCBDependency *pCBDep = pBlock->pCBDeps;
    SShaderCBDependency *pLastCBDep = pBlock->pCBDeps + pBlock->CBDepCount;

    for (; pCBDep<pLastCBDep; pCBDep++)
    {
        assert(pCBDep->ppFXPointers);

        for (size_t i = 0; i < pCBDep->Count; ++ i)
        {
            CheckAndUpdateCB_FX(m_pContext, (SConstantBuffer*)pCBDep->ppFXPointers[i]);
        }

        (m_pContext->*(pVT->pSetConstantBuffers))(pCBDep->StartIndex, pCBDep->Count, pCBDep->ppD3DObjects);
    }

    // Next, apply samplers
    SShaderSamplerDependency *pSampDep = pBlock->pSampDeps;
    SShaderSamplerDependency *pLastSampDep = pBlock->pSampDeps + pBlock->SampDepCount;

    for (; pSampDep<pLastSampDep; pSampDep++)
    {
        assert(pSampDep->ppFXPointers);

        for (size_t i=0; i<pSampDep->Count; i++)
        {
            if ( ApplyRenderStateBlock(pSampDep->ppFXPointers[i]) )
            {
                // If the sampler was updated, its pointer will have changed
                pSampDep->ppD3DObjects[i] = pSampDep->ppFXPointers[i]->pD3DObject;
            }
        }
        (m_pContext->*(pVT->pSetSamplers))(pSampDep->StartIndex, pSampDep->Count, pSampDep->ppD3DObjects);
    }
 
    // Set the UAVs
    // UAV ranges were combined in EffectLoad.  This code remains unchanged, however, so that ranges can be easily split
    assert( pBlock->UAVDepCount < 2 );
    if( pBlock->UAVDepCount > 0 )
    {
        SUnorderedAccessViewDependency *pUAVDep = pBlock->pUAVDeps;
        assert(pUAVDep->ppFXPointers != 0);
        _Analysis_assume_(pUAVDep->ppFXPointers != 0);

        for (size_t i=0; i<pUAVDep->Count; i++)
        {
            pUAVDep->ppD3DObjects[i] = pUAVDep->ppFXPointers[i]->pUnorderedAccessView;
        }

        if( EOT_ComputeShader5 == pBlock->GetShaderType() )
        {
            m_pContext->CSSetUnorderedAccessViews( pUAVDep->StartIndex, pUAVDep->Count, pUAVDep->ppD3DObjects, g_pNegativeOnes );
        }
        else
        {
            // This call could be combined with the call to set render targets if both exist in the pass
            m_pContext->OMSetRenderTargetsAndUnorderedAccessViews( D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, pUAVDep->StartIndex, pUAVDep->Count, pUAVDep->ppD3DObjects, g_pNegativeOnes );
        }
    }

    // TBuffers are funny:
    // We keep two references to them. One is in as a standard texture dep, and that gets used for all sets
    // The other is as a part of the TBufferDeps array, which tells us to rebuild the matching CBs.
    // These two refs could be rolled into one, but then we would have to predicate on each CB or each texture.
    SConstantBuffer **ppTB = pBlock->ppTbufDeps;
    SConstantBuffer **ppLastTB = ppTB + pBlock->TBufferDepCount;

    for (; ppTB<ppLastTB; ppTB++)
    {
        CheckAndUpdateCB_FX(m_pContext, (SConstantBuffer*)*ppTB);
    }

    // Set the textures
    SShaderResourceDependency *pResourceDep = pBlock->pResourceDeps;
    SShaderResourceDependency *pLastResourceDep = pBlock->pResourceDeps + pBlock->ResourceDepCount;

    for (; pResourceDep<pLastResourceDep; pResourceDep++)
    {
        assert(pResourceDep->ppFXPointers != 0);
        _Analysis_assume_(pResourceDep->ppFXPointers != 0);

        for (size_t i=0; i<pResourceDep->Count; i++)
        {
            pResourceDep->ppD3DObjects[i] = pResourceDep->ppFXPointers[i]->pShaderResource;
        }

        (m_pContext->*(pVT->pSetShaderResources))(pResourceDep->StartIndex, pResourceDep->Count, pResourceDep->ppD3DObjects);
    }

    // Update Interface dependencies
    uint32_t Interfaces = 0;
    ID3D11ClassInstance** ppClassInstances = nullptr;
    assert( pBlock->InterfaceDepCount < 2 );
    if( pBlock->InterfaceDepCount > 0 )
    {
        SInterfaceDependency *pInterfaceDep = pBlock->pInterfaceDeps;
        assert(pInterfaceDep->ppFXPointers);

        ppClassInstances = pInterfaceDep->ppD3DObjects;
        Interfaces = pInterfaceDep->Count;
        for (size_t i=0; i<pInterfaceDep->Count; i++)
        {
            assert(pInterfaceDep->ppFXPointers != 0);
            _Analysis_assume_(pInterfaceDep->ppFXPointers != 0);
            SClassInstanceGlobalVariable* pCI = pInterfaceDep->ppFXPointers[i]->pClassInstance;
            if( pCI )
            {
                assert( pCI->pMemberData != 0 );
                _Analysis_assume_( pCI->pMemberData != 0 );
                pInterfaceDep->ppD3DObjects[i] = pCI->pMemberData->Data.pD3DClassInstance;
            }
            else
            {
                pInterfaceDep->ppD3DObjects[i] = nullptr;
            }
        }
    }

    // Now set the shader
    (m_pContext->*(pVT->pSetShader))(pBlock->pD3DObject, ppClassInstances, Interfaces);
}

// Returns true if the block D3D data was recreated
bool CEffect::ApplyRenderStateBlock(_In_ SBaseBlock *pBlock)
{
    if( pBlock->IsUserManaged )
    {
        return false;
    }

    bool bRecreate = pBlock->ApplyAssignments(this);

    if (bRecreate)
    {
        switch (pBlock->BlockType)
        {
        case EBT_Sampler:
            {
                SSamplerBlock *pSBlock = pBlock->AsSampler();

                assert(pSBlock->pD3DObject != 0);
                _Analysis_assume_(pSBlock->pD3DObject != 0);
                pSBlock->pD3DObject->Release();

                HRESULT hr = m_pDevice->CreateSamplerState( &pSBlock->BackingStore.SamplerDesc, &pSBlock->pD3DObject );
                if ( SUCCEEDED(hr) )
                {
                    SetDebugObjectName(pSBlock->pD3DObject, "D3DX11Effect");
                }
            }
            break;

        case EBT_DepthStencil:
            {
                SDepthStencilBlock *pDSBlock = pBlock->AsDepthStencil();

                assert(nullptr != pDSBlock->pDSObject);
                SAFE_RELEASE( pDSBlock->pDSObject );
                if( SUCCEEDED( m_pDevice->CreateDepthStencilState( &pDSBlock->BackingStore, &pDSBlock->pDSObject ) ) )
                {
                    pDSBlock->IsValid = true;
                    SetDebugObjectName( pDSBlock->pDSObject, "D3DX11Effect" );
                }
                else
                    pDSBlock->IsValid = false;
            }
            break;
        
        case EBT_Blend:
            {
                SBlendBlock *pBBlock = pBlock->AsBlend();

                assert(nullptr != pBBlock->pBlendObject);
                SAFE_RELEASE( pBBlock->pBlendObject );
                if( SUCCEEDED( m_pDevice->CreateBlendState( &pBBlock->BackingStore, &pBBlock->pBlendObject ) ) )
                {
                    pBBlock->IsValid = true;
                    SetDebugObjectName( pBBlock->pBlendObject, "D3DX11Effect" );
                }
                else
                    pBBlock->IsValid = false;
            }
            break;

        case EBT_Rasterizer:
            {
                SRasterizerBlock *pRBlock = pBlock->AsRasterizer();

                assert(nullptr != pRBlock->pRasterizerObject);

                SAFE_RELEASE( pRBlock->pRasterizerObject );
                if( SUCCEEDED( m_pDevice->CreateRasterizerState( &pRBlock->BackingStore, &pRBlock->pRasterizerObject ) ) )
                {
                    pRBlock->IsValid = true;
                    SetDebugObjectName( pRBlock->pRasterizerObject, "D3DX11Effect" );
                }
                else
                    pRBlock->IsValid = false;
            }
            break;
        
        default:
            assert(0);
        }
    }

    return bRecreate;
}

void CEffect::ValidateIndex(_In_ uint32_t Elements)
{
    if (m_FXLIndex >= Elements)
    {
        DPF(0, "ID3DX11Effect: Overindexing variable array (size: %u, index: %u), using index = 0 instead", Elements, m_FXLIndex);
        m_FXLIndex = 0;
    }
}

// Returns true if the assignment was changed
bool CEffect::EvaluateAssignment(_Inout_ SAssignment *pAssignment)
{
    bool bNeedUpdate = false;
    SGlobalVariable *pVarDep0, *pVarDep1;
    
    switch (pAssignment->AssignmentType)
    {
    case ERAT_NumericVariable:
        assert(pAssignment->DependencyCount == 1);
        if (pAssignment->pDependencies[0].pVariable->LastModifiedTime >= pAssignment->LastRecomputedTime)
        {
            memcpy(pAssignment->Destination.pNumeric, pAssignment->Source.pNumeric, pAssignment->DataSize);
            bNeedUpdate = true;
        }
        break;

    case ERAT_NumericVariableIndex:
        assert(pAssignment->DependencyCount == 2);
        pVarDep0 = pAssignment->pDependencies[0].pVariable;
        pVarDep1 = pAssignment->pDependencies[1].pVariable;

        if (pVarDep0->LastModifiedTime >= pAssignment->LastRecomputedTime)
        {
            m_FXLIndex = *pVarDep0->Data.pNumericDword;

            ValidateIndex(pVarDep1->pType->Elements);

            // Array index variable is dirty, update the pointer
            pAssignment->Source.pNumeric = pVarDep1->Data.pNumeric + pVarDep1->pType->Stride * m_FXLIndex;
            
            // Copy the new data
            memcpy(pAssignment->Destination.pNumeric, pAssignment->Source.pNumeric, pAssignment->DataSize);
            bNeedUpdate = true;
        }
        else if (pVarDep1->LastModifiedTime >= pAssignment->LastRecomputedTime)
        {
            // Only the array variable is dirty, copy the new data
            memcpy(pAssignment->Destination.pNumeric, pAssignment->Source.pNumeric, pAssignment->DataSize);
            bNeedUpdate = true;
        }
        break;

    case ERAT_ObjectVariableIndex:
        assert(pAssignment->DependencyCount == 1);
        pVarDep0 = pAssignment->pDependencies[0].pVariable;
        if (pVarDep0->LastModifiedTime >= pAssignment->LastRecomputedTime)
        {
            m_FXLIndex = *pVarDep0->Data.pNumericDword;
            ValidateIndex(pAssignment->MaxElements);

            // Array index variable is dirty, update the destination pointer
            *((void **)pAssignment->Destination.pGeneric) = pAssignment->Source.pNumeric +
                pAssignment->DataSize * m_FXLIndex;
            bNeedUpdate = true;
        }
        break;

    default:
    //case ERAT_Constant:           -- These are consumed and discarded
    //case ERAT_ObjectVariable:     -- These are consumed and discarded
    //case ERAT_ObjectConstIndex:   -- These are consumed and discarded
    //case ERAT_ObjectInlineShader: -- These are consumed and discarded
    //case ERAT_NumericConstIndex:  -- ERAT_NumericVariable should be generated instead
        assert(0);
        break;
    }
    
    // Mark the assignment as not dirty
    pAssignment->LastRecomputedTime = m_LocalTimer;

    return bNeedUpdate;
}

// Returns false if this shader has interface dependencies which are nullptr (SetShader will fail).
bool CEffect::ValidateShaderBlock( _Inout_ SShaderBlock* pBlock )
{
    if( !pBlock->IsValid )
        return false;
    if( pBlock->InterfaceDepCount > 0 )
    {
        assert( pBlock->InterfaceDepCount == 1 );
        for( size_t  i=0; i < pBlock->pInterfaceDeps[0].Count; i++ )
        {
            SInterface* pInterfaceDep = pBlock->pInterfaceDeps[0].ppFXPointers[i];
            assert( pInterfaceDep != 0 );
            _Analysis_assume_( pInterfaceDep != 0 );
            if( pInterfaceDep->pClassInstance == nullptr )
            {
                return false;
            }
        }
    }
    return true;
}

// Returns false if any state in the pass is invalid
bool CEffect::ValidatePassBlock( _Inout_ SPassBlock* pBlock )
{
    pBlock->ApplyPassAssignments();

    if (nullptr != pBlock->BackingStore.pBlendBlock)
    {
        ApplyRenderStateBlock(pBlock->BackingStore.pBlendBlock);
        pBlock->BackingStore.pBlendState = pBlock->BackingStore.pBlendBlock->pBlendObject;
        if( !pBlock->BackingStore.pBlendBlock->IsValid )
            return false;
    }

    if( nullptr != pBlock->BackingStore.pDepthStencilBlock )
    {
        ApplyRenderStateBlock( pBlock->BackingStore.pDepthStencilBlock );
        pBlock->BackingStore.pDepthStencilState = pBlock->BackingStore.pDepthStencilBlock->pDSObject;
        if( !pBlock->BackingStore.pDepthStencilBlock->IsValid )
            return false;
    }

    if( nullptr != pBlock->BackingStore.pRasterizerBlock )
    {
        ApplyRenderStateBlock( pBlock->BackingStore.pRasterizerBlock );
        if( !pBlock->BackingStore.pRasterizerBlock->IsValid )
            return false;
    }

    if( nullptr != pBlock->BackingStore.pVertexShaderBlock && !ValidateShaderBlock(pBlock->BackingStore.pVertexShaderBlock) )
        return false;

    if( nullptr != pBlock->BackingStore.pGeometryShaderBlock && !ValidateShaderBlock(pBlock->BackingStore.pGeometryShaderBlock) )
        return false;

    if( nullptr != pBlock->BackingStore.pPixelShaderBlock )
    {
        if( !ValidateShaderBlock(pBlock->BackingStore.pPixelShaderBlock) )
            return false;
        else if( pBlock->BackingStore.pPixelShaderBlock->UAVDepCount > 0 && 
                 pBlock->BackingStore.RenderTargetViewCount > pBlock->BackingStore.pPixelShaderBlock->pUAVDeps[0].StartIndex )
        {
            return false;
        }
    }

    if( nullptr != pBlock->BackingStore.pHullShaderBlock && !ValidateShaderBlock(pBlock->BackingStore.pHullShaderBlock) )
        return false;

    if( nullptr != pBlock->BackingStore.pDomainShaderBlock && !ValidateShaderBlock(pBlock->BackingStore.pDomainShaderBlock) )
        return false;

    if( nullptr != pBlock->BackingStore.pComputeShaderBlock && !ValidateShaderBlock(pBlock->BackingStore.pComputeShaderBlock) )
        return false;

    return true;
}

// Set all state defined in the pass
void CEffect::ApplyPassBlock(_Inout_ SPassBlock *pBlock)
{
    pBlock->ApplyPassAssignments();

    if (nullptr != pBlock->BackingStore.pBlendBlock)
    {
        ApplyRenderStateBlock(pBlock->BackingStore.pBlendBlock);
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pBlendBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid BlendState." );
#endif
        pBlock->BackingStore.pBlendState = pBlock->BackingStore.pBlendBlock->pBlendObject;
        m_pContext->OMSetBlendState(pBlock->BackingStore.pBlendState,
            pBlock->BackingStore.BlendFactor,
            pBlock->BackingStore.SampleMask);
    }

    if (nullptr != pBlock->BackingStore.pDepthStencilBlock)
    {
        ApplyRenderStateBlock(pBlock->BackingStore.pDepthStencilBlock);
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pDepthStencilBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid DepthStencilState." );
#endif
        pBlock->BackingStore.pDepthStencilState = pBlock->BackingStore.pDepthStencilBlock->pDSObject;
        m_pContext->OMSetDepthStencilState(pBlock->BackingStore.pDepthStencilState,
            pBlock->BackingStore.StencilRef);
    }

    if (nullptr != pBlock->BackingStore.pRasterizerBlock)
    {
        ApplyRenderStateBlock(pBlock->BackingStore.pRasterizerBlock);
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pRasterizerBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid RasterizerState." );
#endif
        m_pContext->RSSetState(pBlock->BackingStore.pRasterizerBlock->pRasterizerObject);
    }

    if (nullptr != pBlock->BackingStore.pRenderTargetViews[0])
    {
        // Grab all render targets
        ID3D11RenderTargetView *pRTV[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];

        assert(pBlock->BackingStore.RenderTargetViewCount <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
        _Analysis_assume_(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT >= pBlock->BackingStore.RenderTargetViewCount);

        for (uint32_t i=0; i<pBlock->BackingStore.RenderTargetViewCount; i++)
        {
            pRTV[i] = pBlock->BackingStore.pRenderTargetViews[i]->pRenderTargetView;
        }

        // This call could be combined with the call to set PS UAVs if both exist in the pass
        m_pContext->OMSetRenderTargetsAndUnorderedAccessViews( pBlock->BackingStore.RenderTargetViewCount, pRTV, pBlock->BackingStore.pDepthStencilView->pDepthStencilView, 7, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr );
    }

    if (nullptr != pBlock->BackingStore.pVertexShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pVertexShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid vertex shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pVertexShaderBlock);
    }

    if (nullptr != pBlock->BackingStore.pPixelShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pPixelShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid pixel shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pPixelShaderBlock);
    }

    if (nullptr != pBlock->BackingStore.pGeometryShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pGeometryShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid geometry shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pGeometryShaderBlock);
    }

    if (nullptr != pBlock->BackingStore.pHullShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pHullShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid hull shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pHullShaderBlock);
    }

    if (nullptr != pBlock->BackingStore.pDomainShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pDomainShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid domain shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pDomainShaderBlock);
    }

    if (nullptr != pBlock->BackingStore.pComputeShaderBlock)
    {
#ifdef FXDEBUG
        if( !pBlock->BackingStore.pComputeShaderBlock->IsValid )
            DPF( 0, "Pass::Apply - warning: applying invalid compute shader." );
#endif
        ApplyShaderBlock(pBlock->BackingStore.pComputeShaderBlock);
    }
}

void CEffect::IncrementTimer()
{
    m_LocalTimer++;

#if !defined(_M_X64) && !defined(_M_ARM64)
#ifdef _DEBUG
    if (m_LocalTimer > g_TimerRolloverCount)
    {
        DPF(0, "Rolling over timer (current time: %zu, rollover cap: %u).", m_LocalTimer, g_TimerRolloverCount);
#else
    if (m_LocalTimer >= 0x80000000) // check to see if we've exceeded ~2 billion
    {
#endif
        HandleLocalTimerRollover();

        m_LocalTimer = 1;
    }
#endif // _M_X64
}

// This function resets all timers, rendering all assignments dirty
// This is clearly bad for performance, but should only happen every few billion ticks
void CEffect::HandleLocalTimerRollover()
{
    uint32_t  i, j, k;
    
    // step 1: update variables
    for (i = 0; i < m_VariableCount; ++ i)
    {
        m_pVariables[i].LastModifiedTime = 0;
    }

    // step 2: update assignments on all blocks (pass, depth stencil, rasterizer, blend, sampler)
    for (uint32_t iGroup = 0; iGroup < m_GroupCount; ++ iGroup)
    {
        for (i = 0; i < m_pGroups[iGroup].TechniqueCount; ++ i)
        {
            for (j = 0; j < m_pGroups[iGroup].pTechniques[i].PassCount; ++ j)
            {
                for (k = 0; k < m_pGroups[iGroup].pTechniques[i].pPasses[j].AssignmentCount; ++ k)
                {
                    m_pGroups[iGroup].pTechniques[i].pPasses[j].pAssignments[k].LastRecomputedTime = 0;
                }
            }
        }
    }

    for (i = 0; i < m_DepthStencilBlockCount; ++ i)
    {
        for (j = 0; j < m_pDepthStencilBlocks[i].AssignmentCount; ++ j)
        {
            m_pDepthStencilBlocks[i].pAssignments[j].LastRecomputedTime = 0;
        }
    }

    for (i = 0; i < m_RasterizerBlockCount; ++ i)
    {
        for (j = 0; j < m_pRasterizerBlocks[i].AssignmentCount; ++ j)
        {
            m_pRasterizerBlocks[i].pAssignments[j].LastRecomputedTime = 0;
        }
    }

    for (i = 0; i < m_BlendBlockCount; ++ i)
    {
        for (j = 0; j < m_pBlendBlocks[i].AssignmentCount; ++ j)
        {
            m_pBlendBlocks[i].pAssignments[j].LastRecomputedTime = 0;
        }
    }

    for (i = 0; i < m_SamplerBlockCount; ++ i)
    {
        for (j = 0; j < m_pSamplerBlocks[i].AssignmentCount; ++ j)
        {
            m_pSamplerBlocks[i].pAssignments[j].LastRecomputedTime = 0;
        }
    }
}

}
