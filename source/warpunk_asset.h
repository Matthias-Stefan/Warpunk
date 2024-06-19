/* ========================================================================
   $File: warpunk_asset.h $
   $Date: 12.02.2024 $
   $Creator: Matthias Stefan $
   ======================================================================== */

#ifndef WARPUNK_ASSET_H
#define WARPUNK_ASSET_H

global char* Files[] = {
    
};

enum asset_type
{
    AssetType_Audio,
    AssetType_Geo,
    AssetType_Texture,
    
    AssetType_Count
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
};

struct asset_source_file
{
    u32 ID;
    asset_type Type;
    asset_state State;
    char *Path;
    platform_file_handle Handle;
}

struct game_assets
{
    asset_source_file* AudioFiles[256];
    asset_source_file* GeoFiles[256];
    asset_source_file* TextureFiles[256];
};

internal void LoadAudio(game_assets *Assets, char *Path); 
internal void LoadGeo(game_assets *Assets, char *Path);
internal void LoadTexture(game_assets *Assets, char *Path);

internal void UnloadAudio(game_assets *Assets, u32 ID);
internal void UnloadGeo(game_assets *Assets, u32 ID);
internal void UnloadTexture(game_assets *Assets, u32 ID);


#endif //WARPUNK_ASSET_H
