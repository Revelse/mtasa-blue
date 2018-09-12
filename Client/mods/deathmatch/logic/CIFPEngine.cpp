/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/mods/deathmatch/logic/CIFPEngine.cpp
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>

std::shared_ptr<CClientIFP> CIFPEngine::EngineLoadIFP(CResource* pResource, CClientManager* pManager, const SString& strFile, bool bIsRawData, const SString& strBlockName)
{
    // Grab the resource root entity
    CClientEntity*     pRoot = pResource->GetResourceIFPRoot();
    const unsigned int u32BlockNameHash = HashString(strBlockName.ToLower());

    // Check whether the IFP blockname exists or not
    if (g_pClientGame->GetIFPPointerFromMap(u32BlockNameHash) == nullptr)
    {
        // Create a IFP element
        std::shared_ptr<CClientIFP> pIFP(new CClientIFP(pManager, INVALID_ELEMENT_ID));

        // Try to load the IFP file
        if (pIFP->LoadIFP(strFile, bIsRawData, strBlockName))
        {
            // We can use the map to retrieve correct IFP by block name later
            g_pClientGame->InsertIFPPointerToMap(u32BlockNameHash, pIFP);

            // Success loading the file. Set parent to IFP root
            pIFP->SetParent(pRoot);
            return pIFP;
        }
    }
    return nullptr;
}

bool CIFPEngine::EngineReplaceAnimation(CClientEntity* pEntity, const SString& strInternalBlockName, const SString& strInternalAnimName,
                                        const SString& strCustomBlockName, const SString& strCustomAnimName)
{
    if (IS_PED(pEntity))
    {
        CClientPed& Ped = static_cast<CClientPed&>(*pEntity);

        const unsigned int          u32BlockNameHash = HashString(strCustomBlockName.ToLower());
        CAnimBlock*                 pInternalBlock = g_pGame->GetAnimManager()->GetAnimationBlock(strInternalBlockName);
        std::shared_ptr<CClientIFP> pCustomIFP = g_pClientGame->GetIFPPointerFromMap(u32BlockNameHash);
        if (pInternalBlock && pCustomIFP)
        {
            // Try to load the block, if it's not loaded already
            pInternalBlock->Request(BLOCKING, true);

            auto                            pInternalAnimHierarchy = g_pGame->GetAnimManager()->GetAnimation(strInternalAnimName, pInternalBlock);
            CAnimBlendHierarchySAInterface* pCustomAnimHierarchyInterface = pCustomIFP->GetAnimationHierarchy(strCustomAnimName);
            if (pInternalAnimHierarchy && pCustomAnimHierarchyInterface)
            {
                Ped.ReplaceAnimation(pInternalAnimHierarchy, pCustomIFP, pCustomAnimHierarchyInterface);

                CAnimManager* pAnimationManager = g_pGame->GetAnimManager();
                RpClump* pClump = Ped.GetClump();
                if (pClump)
                {
                    auto pCurrentAnimAssociation = pAnimationManager->RpAnimBlendClumpGetAssociation(pClump, strInternalAnimName);
                    if (pCurrentAnimAssociation)
                    {
                        auto pAnimHierarchy = pAnimationManager->GetCustomAnimBlendHierarchy(pCustomAnimHierarchyInterface);
                        pAnimationManager->UncompressAnimation(pAnimHierarchy.get());
                        pCurrentAnimAssociation->FreeAnimBlendNodeArray();
                        pCurrentAnimAssociation->InitializeWithHierarchy(pClump, pCustomAnimHierarchyInterface);
                        pCurrentAnimAssociation->SetCurrentProgress(0.0);
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool CIFPEngine::EngineRestoreAnimation(CClientEntity* pEntity, const SString& strInternalBlockName, const SString& strInternalAnimName,
                                        const eRestoreAnimation& eRestoreType)
{
    if (IS_PED(pEntity))
    {
        CClientPed& Ped = static_cast<CClientPed&>(*pEntity);

        if (eRestoreType == eRestoreAnimation::ALL)
        {
            Ped.RestoreAllAnimations();
            return true;
        }
        else
        {
            CAnimBlock* pInternalBlock = g_pGame->GetAnimManager()->GetAnimationBlock(strInternalBlockName);
            if (pInternalBlock)
            {
                if (eRestoreType == eRestoreAnimation::BLOCK)
                {
                    Ped.RestoreAnimations(*pInternalBlock);
                    return true;
                }
                else
                {
                    auto pInternalAnimHierarchy = g_pGame->GetAnimManager()->GetAnimation(strInternalAnimName, pInternalBlock);
                    if (pInternalAnimHierarchy)
                    {
                        Ped.RestoreAnimation(pInternalAnimHierarchy);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Return true if data looks like IFP file contents
bool CIFPEngine::IsIFPData(const SString& strData)
{
    return strData.length() > 32 && memcmp(strData, "\x41\x4E\x50", 3) == 0;
}
