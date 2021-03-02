//--------------------------------------------------------------------------------------
// File: nvStereo.cpp
// Authors: Samuel Gateau
// Email: devsupport@nvidia.com
//
// Util functions for stereo
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "nvStereo.h"

using namespace nv;

//--------------------------------------------------------------------------------------
// D3D10
//--------------------------------------------------------------------------------------

#define D3D10MAP_WIDTH 8
#define D3D10MAP_FORMAT DXGI_FORMAT_R32_FLOAT
#define D3D10MAP_PIXELSIZE 4
/*
#define D3D10MAP_WIDTH 2
#define D3D10MAP_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT
#define D3D10MAP_PIXELSIZE 16
*/
/*#define D3D10MAP_WIDTH 4
#define D3D10MAP_FORMAT DXGI_FORMAT_R32G32_FLOAT
#define D3D10MAP_PIXELSIZE 8
*/

// Create / Destroy
StereoParametersD3D10::StereoParametersD3D10() :
    m_StereoParamsMap(0),
    m_StereoParamsMapSRV(0)
{}

StereoParametersD3D10::~StereoParametersD3D10()
{}

// Create the graphics objects
void StereoParametersD3D10::createGraphics( ID3D10Device* pd3dDevice )
{
    D3D10_TEXTURE2D_DESC desc;
    memset( &desc, 0, sizeof(D3D10_TEXTURE2D_DESC) );
    desc.Width = D3D10MAP_WIDTH;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = D3D10MAP_FORMAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    pd3dDevice->CreateTexture2D( &desc, 0, &m_StereoParamsMap);
    pd3dDevice->CreateShaderResourceView( m_StereoParamsMap, 0, &m_StereoParamsMapSRV );
}

void StereoParametersD3D10::destroyGraphics()
{
    if (m_StereoParamsMap)
        m_StereoParamsMap->Release();
    m_StereoParamsMap = 0;

    if (m_StereoParamsMapSRV)
        m_StereoParamsMapSRV->Release();
    m_StereoParamsMapSRV = 0;
}

// Update the stereo parameters data
// Call this function everytime the stereo Separation or the stereo Convergence changes
void StereoParametersD3D10::updateStereoParamsMap( ID3D10Device* pd3dDevice, float eyeSeparation, float separation, float convergence )
{
    // allocate the Stereo Map if not done yet
    if ( !m_StereoParamsMap )
        createGraphics( pd3dDevice );

    // create the sys mem version of the data
    unsigned int width = D3D10MAP_WIDTH;
    unsigned int height = 1;
    unsigned int pixelSize = D3D10MAP_PIXELSIZE;
    ID3D10Texture2D* sysTexture;

    // Allocate the buffer sys mem data to write the stereo tag and stereo params
    D3D10_SUBRESOURCE_DATA sysData;
    sysData.pSysMem = new unsigned char[ pixelSize*width*2 * (height+1)] ;
    sysData.SysMemPitch = 2*width*pixelSize;

    // Stereo parameters
    float* rightParams = ((float *) sysData.pSysMem) + width*pixelSize / 4;
    float* leftParams = ((float *) sysData.pSysMem);
    rightParams[0] = -eyeSeparation * separation;
    rightParams[1] = eyeSeparation * separation * convergence;
    rightParams[2] = 1.0;

    leftParams[0] = eyeSeparation * separation;
    leftParams[1] = -eyeSeparation * separation * convergence;
    leftParams[2] = -1.0;

    // Stereo Blit defines.
    #define NVSTEREO_IMAGE_SIGNATURE 0x4433564e //NV3D

    typedef struct  _Nv_Stereo_Image_Header
    {
        unsigned int    dwSignature;
        unsigned int    dwWidth;
        unsigned int    dwHeight;
        unsigned int    dwBPP;
        unsigned int    dwFlags;
    } NVSTEREOIMAGEHEADER, *LPNVSTEREOIMAGEHEADER;

    // ORed flags in the dwFlags fiels of the _Nv_Stereo_Image_Header structure above
    #define     SIH_SWAP_EYES               0x00000001

    LPNVSTEREOIMAGEHEADER pStereoImageHeader = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) sysData.pSysMem) + (sysData.SysMemPitch * height));
    pStereoImageHeader->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
    pStereoImageHeader->dwBPP = pixelSize * 8;

    // We'll put left image on the left and right image on the right, so no need for swap
    pStereoImageHeader->dwFlags = 0; ///**/SIH_SWAP_EYES;
    pStereoImageHeader->dwWidth = width*2;
    pStereoImageHeader->dwHeight = height;


    D3D10_TEXTURE2D_DESC desc;
    memset( &desc, 0, sizeof(D3D10_TEXTURE2D_DESC) );
    desc.Width = width*2;
    desc.Height = height+1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = D3D10MAP_FORMAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    pd3dDevice->CreateTexture2D( &desc, &sysData, &sysTexture);

    // free the initial data
    delete sysData.pSysMem;

    // Copy the sysTex to the StereoParamsMap
    D3D10_BOX stereoSrcBox;
    stereoSrcBox.front = 0;
    stereoSrcBox.back = 1;
    stereoSrcBox.top = 0;
    stereoSrcBox.bottom = height;
    stereoSrcBox.left = 0;
    stereoSrcBox.right = width;

    pd3dDevice->CopySubresourceRegion( m_StereoParamsMap, 0, 0, 0, 0, sysTexture, 0, &stereoSrcBox );

    // free the sys texture
    sysTexture->Release();
}

