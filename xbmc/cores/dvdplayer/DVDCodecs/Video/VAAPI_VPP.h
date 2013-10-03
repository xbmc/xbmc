/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "VAAPI.h"
#include "threads/CriticalSection.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

namespace VAAPI
{
    struct CVPPPicture
    {
      CVPPPicture():valid(false) {}

      bool valid;
      DVDVideoPicture DVDPic;
      CSurfacePtr surface;
    };

    class CVPP
    {
        CVPP();

        public:
        enum ReadyState
        {
            NotReady = 0,
            Ready,
            InitFailed,
            RuntimeFailed
        };
        enum SupportState
        {
            Unknown = 0,
            Supported,
            Unsupported
        };

        CVPP(CDisplayPtr& display, int width, int height);
        ~CVPP();

        static bool VppSupported();
        static bool DeintSupported(DeintMethod method);

        void Deinit();
        void DeinitVpp();
        void DeinitDeint();

        bool InitVpp();
        bool InitDeint(DeintMethod method, int num_surfaces);

        inline bool VppReady() { return m_vppReady == Ready; }
        inline bool VppFailed() { return m_vppReady == InitFailed || m_vppReady == RuntimeFailed; }
        inline bool DeintReady() { return m_deintReady == Ready; }
        inline bool DeintFailed() { return m_deintReady == InitFailed || m_deintReady == RuntimeFailed; }
        inline DeintMethod getUsedMethod() { return m_usedMethod; }

        CVPPPicture DoDeint(const CVPPPicture& input, bool topFieldFirst, bool firstCall);

        void Flush();

        private:
        CSurfacePtr getFreeSurface();


        static SupportState g_supported;
        static SupportState g_deintSupported[VAAPI::Deinterlacing_Count];


        DeintMethod m_usedMethod;
        unsigned int m_forwardReferencesCount;
        unsigned int m_backwardReferencesCount;
        std::list<CVPPPicture> m_forwardReferences;
        std::list<CVPPPicture> m_backwardReferences;

        CDisplayPtr m_display;
        int m_width;
        int m_height;

        VAConfigID m_configId;

        ReadyState m_vppReady;
        ReadyState m_deintReady;

        //VPP Deinterlacing
        CCriticalSection m_deint_lock;
        VAContextID m_deintContext;
        std::vector<CSurfacePtr> m_deintTargets;
        VABufferID m_deintFilter;
    };

}

