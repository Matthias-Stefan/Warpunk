/* ========================================================================
   $File: win32_warpunk.cpp $
   $Date: 21.10.2023 $
   $Creator: Matthias Stefan $
   ======================================================================== */

// <<Function Definitions>>+=

#ifndef WARPUNK_NOCODE_H
#define WARPUNK_NOCODE_H


for (;;)
{
    if (CurrentPt->IsTarget == 1)
    {
        break;
    }
    
    for (u16 AdjacentPtIndex = 0;
         AdjacentPtIndex < CurrentPt->AdjacentSize; 
         ++AdjacentPtIndex)
    {
        way_point *AdjacentPt = CurrentPt->AdjacentPts[AdjacentPtIndex];
        AdjacentPt->G_Dist = DistanceSq(CurrentPt->Pos, AdjacentPt->Pos);
        AdjacentPt->F_Dist = AdjacentPt->G_Dist + AdjacentPt->H_Dist;
        
        if (WayPts.Size > 1)
        {
            for (u16 a = 0; a < WayPts.Size-1; ++a)
            {
                for (u16 b = a+1; b < WayPts.Size; ++b)
                {
                    way_point **A = GetQueueItemAt(&WayPts, a);
                    way_point **B = GetQueueItemAt(&WayPts, b);
                    
                    if ((*A)->F_Dist > (*B)->F_Dist)
                    {
                        way_point *Temp = *A;
                        *A = *B;
                        (*B) = Temp;
                    }
                }
            }
        }
    }
    
    way_point **ShortestWayPt = GetQueueItemAt(&WayPts, 0);
    (*ShortestWayPt)->G_Dist = CurrentPt->G_Dist + (*ShortestWayPt)->G_Dist; 
    (*ShortestWayPt)->Prev = CurrentPt;
    CurrentPt = (*ShortestWayPt);
    
}

// renderer.h
#if 0

#define #define WIN32_LOAD_RENDERER(name) platform_renderer* name(HDC WindowDC)
typedef WIN32_LOAD_RENDERER(win32_load_renderer);
#define WIN32_LOAD_RENDERER_ENTRY() WIN32_LOAD_RENDERER(Win32LoadRenderer)

struct win32_renderer_function_table
{
    win32_load_renderer *LoadRenderer;
    renderer_begin_frame *BeginFrame;
    renderer_end_frame *EndFrame;
};

global char *Win32RendererFunctionTableNames[] =
{
    "Win32LoadRenderer",
    "Win32BeginFrame",
    "Win32EndFrame",
    
};
#endif

#endif //WARPUNK_NOCODE_H
