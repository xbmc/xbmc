//--------------------------------------------------------------------------------------
// File: SharedResourcePool.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include <map>
#include <memory>

namespace DirectX
{
    // Pool manager ensures that only a single TData instance is created for each unique TKey.
    // This is used to avoid duplicate resource creation, so that for instance a caller can
    // create any number of SpriteBatch instances, but these can internally share shaders and
    // vertex buffer if more than one SpriteBatch uses the same underlying D3D device.
    template<typename TKey, typename TData>
    class SharedResourcePool
    {
    public:
        SharedResourcePool()
          : mResourceMap(std::make_shared<ResourceMap>())
        { }


        // Allocates or looks up the shared TData instance for the specified key.
        std::shared_ptr<TData> DemandCreate(TKey key)
        {
            std::lock_guard<std::mutex> lock(mResourceMap->mutex);

            // Return an existing instance?
            auto pos = mResourceMap->find(key);

            if (pos != mResourceMap->end())
            {
                auto existingValue = pos->second.lock();

                if (existingValue)
                    return existingValue;
                else
                    mResourceMap->erase(pos);
            }
            
            // Allocate a new instance.
            auto newValue = std::make_shared<WrappedData>(key, mResourceMap);

            mResourceMap->insert(std::make_pair(key, newValue));

            return newValue;
        }


    private:
        // Keep track of all allocated TData instances.
        struct ResourceMap : public std::map<TKey, std::weak_ptr<TData>>
        {
            std::mutex mutex;
        };
        
        std::shared_ptr<ResourceMap> mResourceMap;


        // Wrap TData with our own subclass, so we can hook the destructor
        // to remove instances from our pool before they are freed.
        struct WrappedData : public TData
        {
            WrappedData(TKey key, std::shared_ptr<ResourceMap> const& resourceMap)
              : mKey(key),
                mResourceMap(resourceMap),
                TData(key)
            { }

            ~WrappedData()
            {
                std::lock_guard<std::mutex> lock(mResourceMap->mutex);

                auto pos = mResourceMap->find(mKey);

                // Check for weak reference expiry before erasing, in case DemandCreate runs on
                // a different thread at the same time as a previous instance is being destroyed.
                // We mustn't erase replacement objects that have just been added!
                if (pos != mResourceMap->end() && pos->second.expired())
                {
                    mResourceMap->erase(pos);
                }
            }

            TKey mKey;
            std::shared_ptr<ResourceMap> mResourceMap;
        };


        // Prevent copying.
        SharedResourcePool(SharedResourcePool const&);
        SharedResourcePool& operator= (SharedResourcePool const&);
    };
}
