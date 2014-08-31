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

#include "system.h"
#ifdef HAVE_LIBVA

#include "VAAPI_VPP.h"
#if VA_CHECK_VERSION(0,34,0)
# define VPP_AVAIL 1
#endif

#ifdef VPP_AVAIL
#include <va/va_vpp.h>
#endif

#include "utils/log.h"
#include "threads/SingleLock.h"

#include <algorithm>
#include <boost/scoped_array.hpp>

#ifndef VA_SURFACE_ATTRIB_SETTABLE
#define vaCreateSurfaces(d, f, w, h, s, ns, a, na) \
    vaCreateSurfaces(d, w, h, f, ns, s)
#endif

using namespace VAAPI;


CVPP::SupportState CVPP::g_supported = CVPP::Unknown;
CVPP::SupportState CVPP::g_deintSupported[VAAPI::Deinterlacing_Count] = {CVPP::Unknown};

CVPP::CVPP()
    :m_width(0)
    ,m_height(0)
    ,m_configId(VA_INVALID_ID)
    ,m_vppReady(CVPP::InitFailed)
    ,m_deintReady(CVPP::InitFailed)
    ,m_deintContext(VA_INVALID_ID)
    ,m_deintFilter(VA_INVALID_ID)
{}

CVPP::CVPP(CDisplayPtr& display, int width, int height)
    :m_usedMethod(VAAPI::DeinterlacingNone)
    ,m_display(display)
    ,m_width(width)
    ,m_height(height)
    ,m_configId(VA_INVALID_ID)
    ,m_vppReady(CVPP::NotReady)
    ,m_deintReady(CVPP::NotReady)
    ,m_deintContext(VA_INVALID_ID)
    ,m_deintFilter(VA_INVALID_ID)
{
    assert(display.get() && display->get());
    assert(width > 0 && height > 0);

    CLog::Log(LOGDEBUG, "VAAPI_VPP - Probing available methods");
    for(int i = 1; i < VAAPI::Deinterlacing_Count; ++i)
    {
        if(VppSupported() && g_deintSupported[i] == Unknown && InitVpp())
        {
            if(InitDeint((DeintMethod)i, 1))
                CLog::Log(LOGDEBUG, "VAAPI_VPP - Method %d is supported!", i);
            else
                CLog::Log(LOGDEBUG, "VAAPI_VPP - Method %d is NOT supported!", i);
            Deinit();
        }
    }
    CLog::Log(LOGDEBUG, "VAAPI_VPP - Done probing available methods");
}

CVPP::~CVPP()
{
    Deinit();
}

bool CVPP::VppSupported()
{
#ifdef VPP_AVAIL
    return g_supported != Unsupported;
#else
    return false;
#endif
}

bool CVPP::DeintSupported(DeintMethod method)
{
#ifdef VPP_AVAIL
    if(method == DeinterlacingNone)
        return false;

    return VppSupported() && g_deintSupported[method] != Unsupported;
#else
    return false;
#endif
}

void CVPP::Deinit()
{
#ifdef VPP_AVAIL
    CLog::Log(LOGDEBUG, "VAAPI_VPP - Deinitializing");

    DeinitDeint();
    DeinitVpp();
#endif
}

void CVPP::DeinitVpp()
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    if(VppReady())
    {
        vaDestroyConfig(m_display->get(), m_configId);
        m_vppReady = NotReady;

        CLog::Log(LOGDEBUG, "VAAPI_VPP - Deinitialized vpp");
    }
#endif
}

void CVPP::DeinitDeint()
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    if(DeintReady() || m_deintReady == RuntimeFailed)
    {
        vaDestroyBuffer(m_display->get(), m_deintFilter);
        vaDestroyContext(m_display->get(), m_deintContext);
        m_deintTargets.clear();
        m_deintReady = NotReady;
        m_usedMethod = DeinterlacingNone;

        m_forwardReferences.clear();
        m_backwardReferences.clear();

        CLog::Log(LOGDEBUG, "VAAPI_VPP - Deinitialized deint");
    }
#endif
}

bool CVPP::InitVpp()
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    if(VppFailed() || VppReady())
        return false;

    int numEntrypoints = vaMaxNumEntrypoints(m_display->get());
    boost::scoped_array<VAEntrypoint> entrypoints(new VAEntrypoint[numEntrypoints]);

    if(vaQueryConfigEntrypoints(m_display->get(), VAProfileNone, entrypoints.get(), &numEntrypoints) != VA_STATUS_SUCCESS)
    {
        CLog::Log(LOGERROR, "VAAPI_VPP - failed querying entrypoints");

        m_vppReady = InitFailed;
        return false;
    }

    int i;
    for(i = 0; i < numEntrypoints; ++i)
        if(entrypoints[i] == VAEntrypointVideoProc)
            break;

    if(i >= numEntrypoints)
    {
        CLog::Log(LOGDEBUG, "VAAPI_VPP - Entrypoint VideoProc not supported");
        m_vppReady = InitFailed;
        g_supported = Unsupported;
        return false;
    }

    if(vaCreateConfig(m_display->get(), VAProfileNone, VAEntrypointVideoProc, NULL, 0, &m_configId) != VA_STATUS_SUCCESS)
    {
        CLog::Log(LOGERROR, "VAAPI_VPP - failed creating va config");
        m_vppReady = InitFailed;
        return false;
    }

    CLog::Log(LOGDEBUG, "VAAPI_VPP - Successfully initialized VPP");

    m_vppReady = Ready;
    g_supported = Supported;
    return true;
#else
    g_supported = Unsupported;
    m_vppReady = InitFailed;
    return false;
#endif
}

bool CVPP::InitDeint(DeintMethod method, int num_surfaces)
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    assert(VppReady());

    if(DeintReady())
        return false;

    VAProcDeinterlacingType selectedMethod = VAProcDeinterlacingNone;
    switch(method)
    {
        case DeinterlacingBob:
            selectedMethod = VAProcDeinterlacingBob;
            break;
        case DeinterlacingWeave:
            selectedMethod = VAProcDeinterlacingWeave;
            break;
        case DeinterlacingMotionAdaptive:
            selectedMethod = VAProcDeinterlacingMotionAdaptive;
            break;
        case DeinterlacingMotionCompensated:
            selectedMethod = VAProcDeinterlacingMotionCompensated;
            break;
        default:
            selectedMethod = VAProcDeinterlacingNone;
            break;
    }

    if(selectedMethod == VAProcDeinterlacingNone)
    {
        CLog::Log(LOGERROR, "VAAPI_VPP - invalid deinterlacing method requested!");
        m_deintReady = InitFailed;
        return false;
    }

    boost::scoped_array<VASurfaceID> deintTargetsInternal(new VASurfaceID[num_surfaces]);
    if(vaCreateSurfaces(m_display->get(), VA_RT_FORMAT_YUV420, m_width, m_height, deintTargetsInternal.get(), num_surfaces, NULL, 0) != VA_STATUS_SUCCESS)
    {
        CLog::Log(LOGERROR, "VAAPI_VPP - failed creating deint target surfaces");
        m_deintReady = InitFailed;
        return false;
    }

    if(vaCreateContext(m_display->get(), m_configId, m_width, m_height, VA_PROGRESSIVE, deintTargetsInternal.get(), num_surfaces, &m_deintContext) != VA_STATUS_SUCCESS)
    {
        CLog::Log(LOGERROR, "VAAPI_VPP - failed creating deint context");
        vaDestroySurfaces(m_display->get(), deintTargetsInternal.get(), num_surfaces);
        m_deintReady = InitFailed;
        return false;
    }

    VAProcFilterType filters[VAProcFilterCount];
    unsigned int numFilters = VAProcFilterCount;
    VAProcFilterCapDeinterlacing deinterlacingCaps[VAProcDeinterlacingCount];
    unsigned int numDeinterlacingCaps = VAProcDeinterlacingCount;

    if(
        vaQueryVideoProcFilters(m_display->get(), m_deintContext, filters, &numFilters) != VA_STATUS_SUCCESS
     || vaQueryVideoProcFilterCaps(m_display->get(), m_deintContext, VAProcFilterDeinterlacing, deinterlacingCaps, &numDeinterlacingCaps) != VA_STATUS_SUCCESS)
    {
        vaDestroyContext(m_display->get(), m_deintContext);
        vaDestroySurfaces(m_display->get(), deintTargetsInternal.get(), num_surfaces);
        CLog::Log(LOGERROR, "VAAPI_VPP - failed querying filter caps");
        m_deintReady = InitFailed;
        g_deintSupported[method] = Unsupported;
        return false;
    }

    bool methodSupported = false;
    for(unsigned int fi = 0; fi < numFilters; ++fi)
        if(filters[fi] == VAProcFilterDeinterlacing)
            for(unsigned int ci = 0; ci < numDeinterlacingCaps; ++ci)
            {
                if(deinterlacingCaps[ci].type != selectedMethod)
                    continue;
                methodSupported = true;
                break;
            }

    if(!methodSupported)
    {
        vaDestroyContext(m_display->get(), m_deintContext);
        vaDestroySurfaces(m_display->get(), deintTargetsInternal.get(), num_surfaces);
        CLog::Log(LOGDEBUG, "VAAPI_VPP - deinterlacing filter is not supported");
        m_deintReady = InitFailed;
        g_deintSupported[method] = Unsupported;
        return false;
    }

    VAProcFilterParameterBufferDeinterlacing deint;
    deint.type = VAProcFilterDeinterlacing;
    deint.algorithm = selectedMethod;
    deint.flags = 0;

    if(vaCreateBuffer(m_display->get(), m_deintContext, VAProcFilterParameterBufferType, sizeof(deint), 1, &deint, &m_deintFilter) != VA_STATUS_SUCCESS)
    {
        vaDestroyContext(m_display->get(), m_deintContext);
        vaDestroySurfaces(m_display->get(), deintTargetsInternal.get(), num_surfaces);
        CLog::Log(LOGERROR, "VAAPI_VPP - creating ProcFilterParameterBuffer failed");
        m_deintReady = InitFailed;
        return false;
    }

    VAProcPipelineCaps pplCaps;
    if(vaQueryVideoProcPipelineCaps(m_display->get(), m_deintContext, &m_deintFilter, 1, &pplCaps) != VA_STATUS_SUCCESS)
    {
        vaDestroyBuffer(m_display->get(), m_deintFilter);
        vaDestroyContext(m_display->get(), m_deintContext);
        vaDestroySurfaces(m_display->get(), deintTargetsInternal.get(), num_surfaces);
        CLog::Log(LOGERROR, "VAAPI_VPP - querying ProcPipelineCaps failed");
        m_deintReady = InitFailed;
        return false;
    }

    m_forwardReferencesCount = pplCaps.num_forward_references;
    m_backwardReferencesCount = pplCaps.num_backward_references;

    CLog::Log(LOGDEBUG, "VAAPI_VPP - Successfully initialized deinterlacer %d. Need %d forward and %d backward refs.", (int)method, m_forwardReferencesCount, m_backwardReferencesCount);

    m_deintTargets.resize(num_surfaces);
    for(int i = 0; i < num_surfaces; ++i)
        m_deintTargets[i] = CSurfacePtr(new CSurface(deintTargetsInternal[i], m_display));

    m_deintReady = Ready;
    m_usedMethod = method;
    g_deintSupported[method] = Supported;
    m_forwardReferences.clear();
    return true;
#else
    m_deintReady = InitFailed;
    return false;
#endif
}

#define CHECK_VA(s, b) \
{\
    VAStatus res = s;\
    if(res != VA_STATUS_SUCCESS)\
    {\
        CLog::Log(LOGERROR, "VAAPI_VPP - failed executing "#s" at line %d with error 0x%x:%s", __LINE__, res, vaErrorStr(res));\
        VABufferID buf = b;\
        if(buf != 0)\
            vaDestroyBuffer(m_display->get(), buf);\
        if(forwRefs != 0)\
            delete[] forwRefs;\
        m_forwardReferences.clear();\
        return CVPPPicture();\
    }\
}

CVPPPicture CVPP::DoDeint(const CVPPPicture& input, bool topFieldFirst, bool firstCall)
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    assert(input.valid);

    if(!DeintReady())
        return CVPPPicture();

    VAStatus syncStatus = vaSyncSurface(m_display->get(), input.surface->m_id);
    if(syncStatus != VA_STATUS_SUCCESS)
    {
        CLog::Log(LOGDEBUG, "VAAPI_VPP - failed syncing surface with error 0x%x:%s", syncStatus, vaErrorStr(syncStatus));

        if(syncStatus != VA_STATUS_ERROR_DECODING_ERROR)
            return CVPPPicture();

        VASurfaceDecodeMBErrors *errors;
        if(vaQuerySurfaceError(m_display->get(), input.surface->m_id, syncStatus, (void**)&errors) != VA_STATUS_SUCCESS)
            return CVPPPicture();

        while(errors != 0 && errors->status == 1)
        {
            CLog::Log(LOGDEBUG, "VAAPI_VPP - detected decode error: start_mb: %d, end_mb: %d, error_type: %d", errors->start_mb, errors->end_mb, (int)errors->decode_error_type);
            ++errors;
        }

        return CVPPPicture();
    }

    CVPPPicture procPic = input;

    if(m_forwardReferences.size() < m_forwardReferencesCount)
    {
        if(!firstCall)
            m_forwardReferences.push_back(input);
        return CVPPPicture();
    }

    CSurfacePtr deintTarget = getFreeSurface();
    if(!deintTarget.get())
        return CVPPPicture();

    VAProcFilterParameterBufferDeinterlacing *deint;
    VABufferID pipelineBuf;
    VAProcPipelineParameterBuffer *pipelineParam;
    VARectangle inputRegion;
    VARectangle outputRegion;
    unsigned int deint_flags = 0;

    VASurfaceID *forwRefs = 0;

    CHECK_VA(vaBeginPicture(m_display->get(), m_deintContext, deintTarget->m_id), 0);

    CHECK_VA(vaMapBuffer(m_display->get(), m_deintFilter, (void**)&deint), 0);

    if(firstCall || m_forwardReferences.size() < m_forwardReferencesCount)
    {
        if(!topFieldFirst)
            deint_flags = deint->flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;
        else
            deint_flags = deint->flags = 0;
    }
    else
    {
        if(topFieldFirst)
            deint_flags = deint->flags = VA_DEINTERLACING_BOTTOM_FIELD;
        else
            deint_flags = deint->flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST;
    }

    CHECK_VA(vaUnmapBuffer(m_display->get(), m_deintFilter), 0);

    CHECK_VA(vaCreateBuffer(m_display->get(), m_deintContext, VAProcPipelineParameterBufferType, sizeof(VAProcPipelineParameterBuffer), 1, NULL, &pipelineBuf), 0);
    CHECK_VA(vaMapBuffer(m_display->get(), pipelineBuf, (void**)&pipelineParam), pipelineBuf);

    memset(pipelineParam, 0, sizeof(VAProcPipelineParameterBuffer));

    // This input/output regions crop two the top and bottom pixel rows
    // It removes the flickering pixels caused by bob
    const int vcrop = 1;
    const int hcrop = 0;
    inputRegion.x = outputRegion.x = hcrop;
    inputRegion.y = outputRegion.y = vcrop;
    inputRegion.width = outputRegion.width = m_width - (2 * hcrop);
    inputRegion.height = outputRegion.height = m_height - (2 * vcrop);

    pipelineParam->surface = procPic.surface->m_id;
    pipelineParam->output_region = &outputRegion;
    pipelineParam->surface_region = &inputRegion;
    pipelineParam->output_background_color = 0xff000000;

    pipelineParam->filter_flags = (deint_flags & VA_DEINTERLACING_BOTTOM_FIELD) ? VA_BOTTOM_FIELD : VA_TOP_FIELD;

    pipelineParam->filters = &m_deintFilter;
    pipelineParam->num_filters = 1;

    if(m_forwardReferencesCount > 0)
    {
        forwRefs = new VASurfaceID[m_forwardReferencesCount];

        unsigned int i;

        if(m_forwardReferences.size() < m_forwardReferencesCount)
        {
            for(i = 0; i < m_forwardReferencesCount; ++i)
                forwRefs[i] = VA_INVALID_SURFACE;
        }
        else
        {
            i = 0;
            for(std::list<CVPPPicture>::iterator it = m_forwardReferences.begin(); it != m_forwardReferences.end(); ++it, ++i)
                forwRefs[i] = it->surface->m_id;
        }
    }

    pipelineParam->num_forward_references = m_forwardReferences.size();
    pipelineParam->forward_references = forwRefs;
    pipelineParam->num_backward_references = 0;
    pipelineParam->backward_references = NULL;

    CHECK_VA(vaUnmapBuffer(m_display->get(), pipelineBuf), pipelineBuf);

    CHECK_VA(vaRenderPicture(m_display->get(), m_deintContext, &pipelineBuf, 1), pipelineBuf);
    CHECK_VA(vaEndPicture(m_display->get(), m_deintContext), pipelineBuf);

    CHECK_VA(vaDestroyBuffer(m_display->get(), pipelineBuf), 0);

    CHECK_VA(vaSyncSurface(m_display->get(), deintTarget->m_id), 0);

    if(!firstCall && m_forwardReferencesCount > 0)
    {
        if(m_forwardReferences.size() >= m_forwardReferencesCount)
            m_forwardReferences.pop_front();

        if(m_forwardReferences.size() < m_forwardReferencesCount)
            m_forwardReferences.push_back(input);
    }

    if(forwRefs)
        delete[] forwRefs;

    CVPPPicture res;
    res.valid = true;
    res.DVDPic = procPic.DVDPic;
    res.surface = deintTarget;

    return res;
#else
    return CVPPPicture();
#endif
}

void CVPP::Flush()
{
#ifdef VPP_AVAIL
    CSingleLock lock(m_deint_lock);

    m_forwardReferences.clear();
    m_backwardReferences.clear();
#endif
}

CSurfacePtr CVPP::getFreeSurface()
{
#ifdef VPP_AVAIL
    for(unsigned int i = 0; i < m_deintTargets.size(); ++i)
    {
        if(m_deintTargets[i].unique())
        {
            return m_deintTargets[i];
        }
    }

    CLog::Log(LOGWARNING, "VAAPI_VPP - Running out of surfaces");
#endif

    return CSurfacePtr();
}

#endif
