// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#ifdef INT32
#undef INT32
#endif
#ifdef UINT32
#undef UINT32
#endif
#include <wincodec.h>

#include "lib/pvw32dll.h"
#include "PixelAccess.h"
#include "pictview.h"

#pragma comment(lib, "windowscodecs.lib")

struct COpenImageBackendHandle
{
    PVImageInfo ImageInfo;
    PVImageHandles ImageHandles;
    PVFormatSpecificInfo FormatInfo;
    char FileName[MAX_PATH];
    BYTE* Pixels;
    BYTE** Lines;
    DWORD BufferSize;
    LONG StretchWidth;
    LONG StretchHeight;
    DWORD StretchMode;
    COLORREF BkColor;
    DWORD DecodedFrameIndex;
    BOOL Decoded;
    BOOL BitmapSource;
};

static PVCODE WINAPI PVReadImage2Open(LPPVHandle Img, HDC PaintDC, RECT* pDRect, TProgressProc Progress, void* AppSpecific, int ImageIndex);
static PVCODE WINAPI PVCloseImageOpen(LPPVHandle Img);
static PVCODE WINAPI PVDrawImageOpen(LPPVHandle Img, HDC PaintDC, int X, int Y, LPRECT rect);
static const char* WINAPI PVGetErrorTextOpen(DWORD ErrorCode);
static PVCODE WINAPI PVOpenImageExOpen(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int Size);
static PVCODE WINAPI PVSetBkHandleOpen(LPPVHandle Img, COLORREF BkColor);
static DWORD WINAPI PVGetDLLVersionOpen(void);
static PVCODE WINAPI PVSetStretchParametersOpen(LPPVHandle Img, DWORD Width, DWORD Height, DWORD Mode);
static PVCODE WINAPI PVLoadFromClipboardOpen(LPPVHandle* Img, LPPVImageInfo pImgInfo, int Size);
static PVCODE WINAPI PVGetImageInfoOpen(LPPVHandle Img, LPPVImageInfo pImgInfo, int Size, int ImageIndex);
static PVCODE WINAPI PVSetParamOpen(LPPVHandle Img);
static PVCODE WINAPI PVGetHandles2Open(LPPVHandle Img, LPPVImageHandles* pHandles);
static PVCODE WINAPI PVSaveImageOpen(LPPVHandle Img, const char* OutFName, LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific, int ImageIndex);
static PVCODE WINAPI PVChangeImageOpen(LPPVHandle Img, DWORD Flags);
static DWORD WINAPI PVIsOutCombSupportedOpen(int Fmt, int Compr, int Colors, int ColorModel);
static PVCODE WINAPI PVReadImageSequenceOpen(LPPVHandle Img, LPPVImageSequence* ppSeq);
static PVCODE WINAPI PVCropImageOpen(LPPVHandle Img, int Left, int Top, int Width, int Height);

static void ResetHandleInfo(COpenImageBackendHandle* pHandle);
static void ReleaseDecodedSurface(COpenImageBackendHandle* pHandle);
static void FreeOpenImageBackendHandle(COpenImageBackendHandle* pHandle);
static PVCODE CopyImageInfoOut(const PVImageInfo& srcInfo, LPPVImageInfo pImgInfo, int Size);
static DWORD GetAlignedStride(DWORD width);
static PVCODE HrToPVCode(HRESULT hr);
static DWORD MapContainerFormat(const GUID& formatGuid, const char** pFormatName);
static DWORD MapCompression(DWORD format);
static BOOL HasExifMetadata(IWICBitmapFrameDecode* pFrame, DWORD format);
static PVCODE LoadFrameInfoFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE DecodeFrameFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE DecodeBitmapHandle(COpenImageBackendHandle* pHandle, HBITMAP hBitmap);
static PVCODE AllocateDecodedSurface(COpenImageBackendHandle* pHandle, DWORD width, DWORD height);
static COpenImageBackendHandle* GetOpenImageBackendHandle(LPPVHandle Img);
static PVCODE EnsureDecodedFrame(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE RotateDecodedImage(COpenImageBackendHandle* pHandle, BOOL clockwise);

template <class T>
static void SafeRelease(T*& pObject)
{
    if (pObject != NULL)
    {
        pObject->Release();
        pObject = NULL;
    }
}

class CCoInitScope
{
public:
    HRESULT Hr;
    BOOL NeedUninitialize;

public:
    CCoInitScope()
    {
        Hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        NeedUninitialize = SUCCEEDED(Hr);
        if (Hr == RPC_E_CHANGED_MODE)
            Hr = S_OK;
    }

    ~CCoInitScope()
    {
        if (NeedUninitialize)
            CoUninitialize();
    }
};

static CPVW32DLL OpenImageBackend = {
    PVReadImage2Open,
    PVCloseImageOpen,
    PVDrawImageOpen,
    PVGetErrorTextOpen,
    PVOpenImageExOpen,
    PVSetBkHandleOpen,
    PVGetDLLVersionOpen,
    PVSetStretchParametersOpen,
    PVLoadFromClipboardOpen,
    PVGetImageInfoOpen,
    PVSetParamOpen,
    PVGetHandles2Open,
    PVSaveImageOpen,
    PVChangeImageOpen,
    PVIsOutCombSupportedOpen,
    PVReadImageSequenceOpen,
    PVCropImageOpen,
    GetRGBAtCursor,
    CalculateHistogram,
    CreateThumbnail,
    SimplifyImageSequence,
};

BOOL InitOpenImageBackend()
{
    PVW32DLL = OpenImageBackend;
    PVW32DLL.Handle = NULL;
    lstrcpynA(PVW32DLL.Version, "Open backend / WIC", SizeOf(PVW32DLL.Version));
    return TRUE;
}

static void ResetHandleInfo(COpenImageBackendHandle* pHandle)
{
    memset(&pHandle->ImageInfo, 0, sizeof(pHandle->ImageInfo));
    memset(&pHandle->ImageHandles, 0, sizeof(pHandle->ImageHandles));
    memset(&pHandle->FormatInfo, 0, sizeof(pHandle->FormatInfo));
    pHandle->ImageInfo.cbSize = sizeof(pHandle->ImageInfo);
    pHandle->ImageInfo.FSI = &pHandle->FormatInfo;
    pHandle->ImageInfo.Comment = NULL;
    pHandle->ImageInfo.StretchMode = PV_STRETCH_NO;
    pHandle->FormatInfo.cbSize = sizeof(pHandle->FormatInfo);
    pHandle->StretchWidth = 0;
    pHandle->StretchHeight = 0;
    pHandle->StretchMode = PV_STRETCH_NO;
    pHandle->BkColor = RGB(255, 255, 255);
    pHandle->DecodedFrameIndex = 0;
    pHandle->Decoded = FALSE;
}

static void ReleaseDecodedSurface(COpenImageBackendHandle* pHandle)
{
    if (pHandle == NULL)
        return;

    if (pHandle->Lines != NULL)
    {
        free(pHandle->Lines);
        pHandle->Lines = NULL;
    }
    if (pHandle->Pixels != NULL)
    {
        free(pHandle->Pixels);
        pHandle->Pixels = NULL;
    }
    pHandle->BufferSize = 0;
    pHandle->Decoded = FALSE;
    pHandle->ImageHandles.pLines = NULL;
    pHandle->ImageHandles.Palette = NULL;
}

static void FreeOpenImageBackendHandle(COpenImageBackendHandle* pHandle)
{
    if (pHandle == NULL)
        return;

    ReleaseDecodedSurface(pHandle);
    delete pHandle;
}

static PVCODE CopyImageInfoOut(const PVImageInfo& srcInfo, LPPVImageInfo pImgInfo, int Size)
{
    if (pImgInfo == NULL || Size <= 0)
        return PVC_OK;

    memset(pImgInfo, 0, Size);
    memcpy(pImgInfo, &srcInfo, min(Size, (int)sizeof(srcInfo)));
    return PVC_OK;
}

static DWORD GetAlignedStride(DWORD width)
{
    return ((width * 24 + 31) / 32) * 4;
}

static PVCODE HrToPVCode(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return PVC_OK;

    switch (hr)
    {
    case E_OUTOFMEMORY:
        return PVC_OUT_OF_MEMORY;

    case WINCODEC_ERR_COMPONENTNOTFOUND:
    case WINCODEC_ERR_UNKNOWNIMAGEFORMAT:
        return PVC_UNSUP_FILE_TYPE;

    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
        return PVC_CANNOT_OPEN_FILE;

    default:
        return PVC_READING_ERROR;
    }
}

static DWORD MapContainerFormat(const GUID& formatGuid, const char** pFormatName)
{
    if (pFormatName != NULL)
        *pFormatName = "WIC";

    if (IsEqualGUID(formatGuid, GUID_ContainerFormatBmp))
    {
        if (pFormatName != NULL)
            *pFormatName = "BMP";
        return PVF_BMP;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatGif))
    {
        if (pFormatName != NULL)
            *pFormatName = "GIF";
        return PVF_GIF;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatIco))
    {
        if (pFormatName != NULL)
            *pFormatName = "ICO";
        return PVF_ICO;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatJpeg))
    {
        if (pFormatName != NULL)
            *pFormatName = "JPEG";
        return PVF_JPG;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatPng))
    {
        if (pFormatName != NULL)
            *pFormatName = "PNG";
        return PVF_PNG;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatTiff))
    {
        if (pFormatName != NULL)
            *pFormatName = "TIFF";
        return PVF_TIFF;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatDds))
    {
        if (pFormatName != NULL)
            *pFormatName = "DDS";
        return PVF_DDS;
    }
    return 0;
}

static DWORD MapCompression(DWORD format)
{
    switch (format)
    {
    case PVF_JPG:
        return PVCS_JPEG_HUFFMAN;

    case PVF_PNG:
    case PVF_DDS:
        return PVCS_DEFLATE;

    case PVF_TIFF:
    case PVF_GIF:
    case PVF_BMP:
    case PVF_ICO:
    default:
        return PVCS_NO_COMPRESSION;
    }
}

static BOOL HasExifMetadata(IWICBitmapFrameDecode* pFrame, DWORD format)
{
    IWICMetadataQueryReader* pReader = NULL;
    PROPVARIANT var;
    HRESULT hr;
    BOOL hasExif = FALSE;

    if (pFrame == NULL || (format != PVF_JPG && format != PVF_TIFF))
        return FALSE;

    hr = pFrame->GetMetadataQueryReader(&pReader);
    if (FAILED(hr))
        return FALSE;

    PropVariantInit(&var);
    if (format == PVF_JPG)
        hr = pReader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &var);
    else
        hr = pReader->GetMetadataByName(L"/ifd/{ushort=274}", &var);

    if (SUCCEEDED(hr))
        hasExif = TRUE;

    PropVariantClear(&var);
    SafeRelease(pReader);
    return hasExif;
}

static PVCODE LoadFrameInfoFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    WCHAR fileNameW[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    GUID containerFormat = GUID_NULL;
    const char* formatName = "WIC";
    UINT width = 0;
    UINT height = 0;
    UINT frameCount = 0;
    double dpiX = 0.0;
    double dpiY = 0.0;
    DWORD format = 0;
    HRESULT hr;

    if (pHandle == NULL || pHandle->BitmapSource || pHandle->FileName[0] == 0)
        return PVC_INVALID_HANDLE;

    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);

    if (0 == MultiByteToWideChar(CP_ACP, 0, pHandle->FileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateDecoderFromFilename(fileNameW, NULL, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetContainerFormat(&containerFormat);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrameCount(&frameCount);
    if (SUCCEEDED(hr) && imageIndex >= frameCount)
        hr = WINCODEC_ERR_FRAMEMISSING;
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(imageIndex, &pFrame);
    if (SUCCEEDED(hr))
        hr = pFrame->GetSize(&width, &height);
    if (SUCCEEDED(hr))
        hr = pFrame->GetResolution(&dpiX, &dpiY);
    if (FAILED(hr))
    {
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return hr == WINCODEC_ERR_FRAMEMISSING ? PVC_NO_MORE_IMAGES : HrToPVCode(hr);
    }

    format = MapContainerFormat(containerFormat, &formatName);
    if (format == 0)
    {
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return PVC_UNSUP_FILE_TYPE;
    }

    ReleaseDecodedSurface(pHandle);
    ResetHandleInfo(pHandle);
    pHandle->ImageInfo.Width = width;
    pHandle->ImageInfo.Height = height;
    pHandle->ImageInfo.BytesPerLine = GetAlignedStride(width);
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.Format = format;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.NumOfImages = frameCount;
    pHandle->ImageInfo.CurrentImage = imageIndex;
    pHandle->ImageInfo.StretchedWidth = width;
    pHandle->ImageInfo.StretchedHeight = height;
    pHandle->ImageInfo.HorDPI = (DWORD)(dpiX + 0.5);
    pHandle->ImageInfo.VerDPI = (DWORD)(dpiY + 0.5);
    pHandle->ImageInfo.Compression = MapCompression(format);
    pHandle->ImageInfo.TotalBitDepth = 24;
    lstrcpynA(pHandle->ImageInfo.Info1, formatName, SizeOf(pHandle->ImageInfo.Info1));
    lstrcpynA(pHandle->ImageInfo.Info2, "Open backend (WIC)", SizeOf(pHandle->ImageInfo.Info2));

    if (HasExifMetadata(pFrame, format))
        pHandle->ImageInfo.Flags |= PVFF_EXIF;

    if (GetFileAttributesExA(pHandle->FileName, GetFileExInfoStandard, &attrData))
        pHandle->ImageInfo.FileSize = attrData.nFileSizeLow;

    pHandle->StretchWidth = (LONG)width;
    pHandle->StretchHeight = (LONG)height;
    pHandle->StretchMode = PV_STRETCH_NO;

    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);
    return PVC_OK;
}

static PVCODE AllocateDecodedSurface(COpenImageBackendHandle* pHandle, DWORD width, DWORD height)
{
    DWORD stride;
    BYTE* pPixels;
    BYTE** pLines;
    DWORD y;

    if (pHandle == NULL || width == 0 || height == 0)
        return PVC_INVALID_HANDLE;

    ReleaseDecodedSurface(pHandle);

    stride = GetAlignedStride(width);
    pPixels = (BYTE*)malloc(stride * height);
    if (pPixels == NULL)
        return PVC_OUT_OF_MEMORY;
    pLines = (BYTE**)malloc(sizeof(BYTE*) * height);
    if (pLines == NULL)
    {
        free(pPixels);
        return PVC_OUT_OF_MEMORY;
    }

    for (y = 0; y < height; y++)
        pLines[y] = pPixels + y * stride;

    pHandle->Pixels = pPixels;
    pHandle->Lines = pLines;
    pHandle->BufferSize = stride * height;
    pHandle->ImageHandles.pLines = pLines;
    pHandle->ImageHandles.Palette = NULL;
    pHandle->ImageInfo.Width = width;
    pHandle->ImageInfo.Height = height;
    pHandle->ImageInfo.BytesPerLine = stride;
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.TotalBitDepth = 24;
    if (pHandle->StretchWidth == 0 && pHandle->StretchHeight == 0)
    {
        pHandle->StretchWidth = (LONG)width;
        pHandle->StretchHeight = (LONG)height;
    }
    pHandle->Decoded = TRUE;
    return PVC_OK;
}

static PVCODE DecodeFrameFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    IWICFormatConverter* pConverter = NULL;
    WCHAR fileNameW[MAX_PATH];
    HRESULT hr;
    PVCODE ret;

    ret = LoadFrameInfoFromFile(pHandle, imageIndex);
    if (ret != PVC_OK)
        return ret;

    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);

    if (0 == MultiByteToWideChar(CP_ACP, 0, pHandle->FileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateDecoderFromFilename(fileNameW, NULL, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(imageIndex, &pFrame);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateFormatConverter(&pConverter);
    if (SUCCEEDED(hr))
        hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat24bppBGR,
                                    WICBitmapDitherTypeNone, NULL, 0.0,
                                    WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return HrToPVCode(hr);
    }

    ret = AllocateDecodedSurface(pHandle, pHandle->ImageInfo.Width, pHandle->ImageInfo.Height);
    if (ret == PVC_OK)
    {
        hr = pConverter->CopyPixels(NULL, pHandle->ImageInfo.BytesPerLine,
                                    pHandle->BufferSize, pHandle->Pixels);
        if (FAILED(hr))
        {
            ReleaseDecodedSurface(pHandle);
            ret = HrToPVCode(hr);
        }
        else
        {
            pHandle->DecodedFrameIndex = imageIndex;
            pHandle->ImageInfo.CurrentImage = imageIndex;
        }
    }

    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);
    return ret;
}

static PVCODE DecodeBitmapHandle(COpenImageBackendHandle* pHandle, HBITMAP hBitmap)
{
    BITMAP bmp;
    BITMAPINFO bmi;
    HDC hdc;
    PVCODE ret;
    int scanLines;

    if (pHandle == NULL || hBitmap == NULL)
        return PVC_INVALID_HANDLE;

    if (0 == GetObject(hBitmap, sizeof(bmp), &bmp))
        return PVC_INVALID_HANDLE;

    ResetHandleInfo(pHandle);
    pHandle->BitmapSource = TRUE;
    pHandle->ImageInfo.Format = PVF_BMP;
    pHandle->ImageInfo.Width = bmp.bmWidth;
    pHandle->ImageInfo.Height = bmp.bmHeight;
    pHandle->ImageInfo.NumOfImages = 1;
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.StretchedWidth = bmp.bmWidth;
    pHandle->ImageInfo.StretchedHeight = bmp.bmHeight;
    pHandle->ImageInfo.Compression = PVCS_NO_COMPRESSION;
    pHandle->ImageInfo.TotalBitDepth = 24;
    lstrcpynA(pHandle->ImageInfo.Info1, "BMP", SizeOf(pHandle->ImageInfo.Info1));
    lstrcpynA(pHandle->ImageInfo.Info2, "Open backend (bitmap handle)", SizeOf(pHandle->ImageInfo.Info2));
    pHandle->StretchWidth = bmp.bmWidth;
    pHandle->StretchHeight = bmp.bmHeight;

    ret = AllocateDecodedSurface(pHandle, bmp.bmWidth, bmp.bmHeight);
    if (ret != PVC_OK)
        return ret;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    hdc = GetDC(NULL);
    if (hdc == NULL)
    {
        ReleaseDecodedSurface(pHandle);
        return PVC_GDI_ERROR;
    }

    scanLines = GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pHandle->Pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    if (scanLines == 0)
    {
        ReleaseDecodedSurface(pHandle);
        return PVC_GDI_ERROR;
    }

    pHandle->DecodedFrameIndex = 0;
    pHandle->ImageInfo.CurrentImage = 0;
    return PVC_OK;
}

static COpenImageBackendHandle* GetOpenImageBackendHandle(LPPVHandle Img)
{
    return (COpenImageBackendHandle*)Img;
}

static PVCODE EnsureDecodedFrame(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    if (pHandle->Decoded && pHandle->DecodedFrameIndex == imageIndex)
        return PVC_OK;

    if (pHandle->BitmapSource)
        return imageIndex == 0 ? PVC_OK : PVC_NO_MORE_IMAGES;

    return DecodeFrameFromFile(pHandle, imageIndex);
}

static PVCODE RotateDecodedImage(COpenImageBackendHandle* pHandle, BOOL clockwise)
{
    BYTE* pPixels;
    BYTE** pLines;
    DWORD srcWidth;
    DWORD srcHeight;
    DWORD srcStride;
    DWORD dstWidth;
    DWORD dstHeight;
    DWORD dstStride;
    DWORD x;
    DWORD y;
    LONG oldStretchWidth;
    LONG oldStretchHeight;

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    if (!pHandle->Decoded || pHandle->Pixels == NULL || pHandle->Lines == NULL)
        return PVC_INVALID_HANDLE;

    srcWidth = pHandle->ImageInfo.Width;
    srcHeight = pHandle->ImageInfo.Height;
    srcStride = pHandle->ImageInfo.BytesPerLine;
    dstWidth = srcHeight;
    dstHeight = srcWidth;
    dstStride = GetAlignedStride(dstWidth);

    pPixels = (BYTE*)malloc(dstStride * dstHeight);
    if (pPixels == NULL)
        return PVC_OUT_OF_MEMORY;
    pLines = (BYTE**)malloc(sizeof(BYTE*) * dstHeight);
    if (pLines == NULL)
    {
        free(pPixels);
        return PVC_OUT_OF_MEMORY;
    }
    for (y = 0; y < dstHeight; y++)
        pLines[y] = pPixels + y * dstStride;

    memset(pPixels, 0, dstStride * dstHeight);
    for (y = 0; y < srcHeight; y++)
    {
        BYTE* pSrcLine = pHandle->Lines[y];

        for (x = 0; x < srcWidth; x++)
        {
            DWORD dstX;
            DWORD dstY;
            BYTE* pDstPixel;

            if (clockwise)
            {
                dstX = srcHeight - 1 - y;
                dstY = x;
            }
            else
            {
                dstX = y;
                dstY = srcWidth - 1 - x;
            }

            pDstPixel = pLines[dstY] + dstX * 3;
            memcpy(pDstPixel, pSrcLine + x * 3, 3);
        }
    }

    free(pHandle->Pixels);
    free(pHandle->Lines);
    oldStretchWidth = pHandle->StretchWidth;
    oldStretchHeight = pHandle->StretchHeight;
    pHandle->Pixels = pPixels;
    pHandle->Lines = pLines;
    pHandle->BufferSize = dstStride * dstHeight;
    pHandle->ImageHandles.pLines = pLines;
    pHandle->ImageInfo.Width = dstWidth;
    pHandle->ImageInfo.Height = dstHeight;
    pHandle->ImageInfo.BytesPerLine = dstStride;
    pHandle->ImageInfo.StretchedWidth = labs(oldStretchHeight);
    pHandle->ImageInfo.StretchedHeight = labs(oldStretchWidth);
    pHandle->StretchWidth = oldStretchHeight;
    pHandle->StretchHeight = oldStretchWidth;
    return PVC_OK;
}

static PVCODE WINAPI PVReadImage2Open(LPPVHandle Img, HDC PaintDC, RECT* pDRect, TProgressProc Progress, void* AppSpecific, int ImageIndex)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;

    ret = EnsureDecodedFrame(pHandle, ImageIndex < 0 ? 0 : (DWORD)ImageIndex);
    if (ret != PVC_OK)
        return ret;

    if (Progress != NULL)
        Progress(100, AppSpecific);

    if (PaintDC != NULL)
        return PVDrawImageOpen(Img, PaintDC, pDRect ? pDRect->left : 0, pDRect ? pDRect->top : 0, pDRect);

    return PVC_OK;
}

static PVCODE WINAPI PVCloseImageOpen(LPPVHandle Img)
{
    FreeOpenImageBackendHandle(GetOpenImageBackendHandle(Img));
    return PVC_OK;
}

static PVCODE WINAPI PVDrawImageOpen(LPPVHandle Img, HDC PaintDC, int X, int Y, LPRECT rect)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    BITMAPINFO bmi;
    LONG dstWidth;
    LONG dstHeight;
    int oldStretchMode;
    int savedDc = 0;
    int stretchRet;

    if (pHandle == NULL || PaintDC == NULL)
        return PVC_INVALID_HANDLE;

    if (!pHandle->Decoded || pHandle->Pixels == NULL)
        return PVC_INVALID_HANDLE;

    dstWidth = pHandle->StretchWidth != 0 ? pHandle->StretchWidth : (LONG)pHandle->ImageInfo.Width;
    dstHeight = pHandle->StretchHeight != 0 ? pHandle->StretchHeight : (LONG)pHandle->ImageInfo.Height;
    if (dstWidth == 0 || dstHeight == 0)
        return PVC_INVALID_DIMENSIONS;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = pHandle->ImageInfo.Width;
    bmi.bmiHeader.biHeight = -(LONG)pHandle->ImageInfo.Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = pHandle->ImageInfo.BytesPerLine * pHandle->ImageInfo.Height;

    if (rect != NULL)
    {
        savedDc = SaveDC(PaintDC);
        IntersectClipRect(PaintDC, rect->left, rect->top, rect->right, rect->bottom);
    }

    oldStretchMode = SetStretchBltMode(PaintDC, pHandle->StretchMode == PV_STRETCH_NO ? COLORONCOLOR : (int)pHandle->StretchMode);
    if (pHandle->StretchMode == HALFTONE)
        SetBrushOrgEx(PaintDC, 0, 0, NULL);

    stretchRet = StretchDIBits(PaintDC, X, Y, dstWidth, dstHeight, 0, 0,
                               pHandle->ImageInfo.Width, pHandle->ImageInfo.Height,
                               pHandle->Pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

    SetStretchBltMode(PaintDC, oldStretchMode);
    if (savedDc != 0)
        RestoreDC(PaintDC, savedDc);

    return stretchRet == GDI_ERROR ? PVC_GDI_ERROR : PVC_OK;
}

static const char* WINAPI PVGetErrorTextOpen(DWORD ErrorCode)
{
    switch (ErrorCode)
    {
    case PVC_OK:
        return "Open-source PictView backend completed successfully.";

    case PVC_UNSUP_FILE_TYPE:
    case PVC_UNKNOWN_FILE_STRUCT:
        return "Open-source PictView backend does not support this image format yet.";

    case PVC_CANNOT_OPEN_FILE:
        return "Open-source PictView backend could not open the image file.";

    case PVC_NO_MORE_IMAGES:
        return "Open-source PictView backend could not find the requested image frame.";

    case PVC_WRITING_ERROR:
        return "Open-source PictView backend cannot save this image yet.";

    case PVC_INVALID_HANDLE:
        return "Open-source PictView backend has no valid image handle.";

    case PVC_GDI_ERROR:
        return "Open-source PictView backend failed in a GDI drawing operation.";

    case PVC_OUT_OF_MEMORY:
        return "Open-source PictView backend ran out of memory.";

    default:
        return "Open-source PictView backend operation failed.";
    }
}

static PVCODE WINAPI PVOpenImageExOpen(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int Size)
{
    COpenImageBackendHandle* pHandle;
    PVCODE ret;

    if (Img != NULL)
        *Img = NULL;
    if (pImgInfo != NULL && Size > 0)
        memset(pImgInfo, 0, Size);
    if (Img == NULL || pOpenExInfo == NULL)
        return PVC_INVALID_HANDLE;

    pHandle = new COpenImageBackendHandle;
    if (pHandle == NULL)
        return PVC_OUT_OF_MEMORY;
    memset(pHandle, 0, sizeof(*pHandle));
    ResetHandleInfo(pHandle);

    if ((pOpenExInfo->Flags & PVOF_USERDEFINED_INPUT) != 0)
    {
        FreeOpenImageBackendHandle(pHandle);
        return PVC_UNSUP_FILE_TYPE;
    }

    if ((pOpenExInfo->Flags & PVOF_ATTACH_TO_HANDLE) != 0)
    {
        ret = DecodeBitmapHandle(pHandle, (HBITMAP)pOpenExInfo->Handle);
    }
    else
    {
        if (pOpenExInfo->FileName == NULL || *pOpenExInfo->FileName == 0)
        {
            FreeOpenImageBackendHandle(pHandle);
            return PVC_CANNOT_OPEN_FILE;
        }

        lstrcpynA(pHandle->FileName, pOpenExInfo->FileName, SizeOf(pHandle->FileName));
        ret = LoadFrameInfoFromFile(pHandle, 0);
    }

    if (ret != PVC_OK)
    {
        FreeOpenImageBackendHandle(pHandle);
        return ret;
    }

    *Img = (LPPVHandle)pHandle;
    return CopyImageInfoOut(pHandle->ImageInfo, pImgInfo, Size);
}

static PVCODE WINAPI PVSetBkHandleOpen(LPPVHandle Img, COLORREF BkColor)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    pHandle->BkColor = BkColor;
    return PVC_OK;
}

static DWORD WINAPI PVGetDLLVersionOpen(void)
{
    return MAKELONG(0, 1);
}

static PVCODE WINAPI PVSetStretchParametersOpen(LPPVHandle Img, DWORD Width, DWORD Height, DWORD Mode)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    pHandle->StretchWidth = (LONG)Width;
    pHandle->StretchHeight = (LONG)Height;
    pHandle->StretchMode = Mode;
    pHandle->ImageInfo.StretchedWidth = labs(pHandle->StretchWidth);
    pHandle->ImageInfo.StretchedHeight = labs(pHandle->StretchHeight);
    pHandle->ImageInfo.StretchMode = Mode;
    return PVC_OK;
}

static PVCODE WINAPI PVLoadFromClipboardOpen(LPPVHandle* Img, LPPVImageInfo pImgInfo, int Size)
{
    HANDLE hClipboardBitmap = NULL;
    HBITMAP hLocalBitmap = NULL;
    PVOpenImageExInfo openInfo;
    PVCODE ret;

    if (Img != NULL)
        *Img = NULL;
    if (pImgInfo != NULL && Size > 0)
        memset(pImgInfo, 0, Size);

    if (!OpenClipboard(NULL))
        return PVC_CANNOT_OPEN_FILE;

    hClipboardBitmap = GetClipboardData(CF_BITMAP);
    if (hClipboardBitmap != NULL)
        hLocalBitmap = (HBITMAP)CopyImage(hClipboardBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    CloseClipboard();

    if (hLocalBitmap == NULL)
        return PVC_UNSUP_FILE_TYPE;

    memset(&openInfo, 0, sizeof(openInfo));
    openInfo.cbSize = sizeof(openInfo);
    openInfo.Flags = PVOF_ATTACH_TO_HANDLE;
    openInfo.Handle = hLocalBitmap;
    ret = PVOpenImageExOpen(Img, &openInfo, pImgInfo, Size);
    DeleteObject(hLocalBitmap);
    return ret;
}

static PVCODE WINAPI PVGetImageInfoOpen(LPPVHandle Img, LPPVImageInfo pImgInfo, int Size, int ImageIndex)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;
    LONG stretchWidth;
    LONG stretchHeight;
    DWORD stretchMode;

    if (pHandle == NULL)
    {
        if (pImgInfo != NULL && Size > 0)
            memset(pImgInfo, 0, Size);
        return PVC_INVALID_HANDLE;
    }

    if (ImageIndex < 0)
        ImageIndex = 0;

    stretchWidth = pHandle->StretchWidth;
    stretchHeight = pHandle->StretchHeight;
    stretchMode = pHandle->StretchMode;

    if (!pHandle->BitmapSource && (DWORD)ImageIndex != pHandle->ImageInfo.CurrentImage)
    {
        ret = LoadFrameInfoFromFile(pHandle, (DWORD)ImageIndex);
        if (ret != PVC_OK)
            return ret;

        pHandle->StretchWidth = stretchWidth;
        pHandle->StretchHeight = stretchHeight;
        pHandle->StretchMode = stretchMode;
        pHandle->ImageInfo.StretchedWidth = labs(stretchWidth);
        pHandle->ImageInfo.StretchedHeight = labs(stretchHeight);
        pHandle->ImageInfo.StretchMode = stretchMode;
    }
    else
    {
        pHandle->ImageInfo.CurrentImage = ImageIndex;
    }

    return CopyImageInfoOut(pHandle->ImageInfo, pImgInfo, Size);
}

static PVCODE WINAPI PVSetParamOpen(LPPVHandle Img)
{
    return Img != NULL ? PVC_OK : PVC_INVALID_HANDLE;
}

static PVCODE WINAPI PVGetHandles2Open(LPPVHandle Img, LPPVImageHandles* pHandles)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandles != NULL)
        *pHandles = NULL;
    if (pHandle == NULL || pHandles == NULL)
        return PVC_INVALID_HANDLE;
    if (!pHandle->Decoded)
        return PVC_INVALID_HANDLE;

    *pHandles = &pHandle->ImageHandles;
    return PVC_OK;
}

static PVCODE WINAPI PVSaveImageOpen(LPPVHandle Img, const char* OutFName, LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific, int ImageIndex)
{
    return PVC_WRITING_ERROR;
}

static PVCODE WINAPI PVChangeImageOpen(LPPVHandle Img, DWORD Flags)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    ret = EnsureDecodedFrame(pHandle, pHandle->ImageInfo.CurrentImage);
    if (ret != PVC_OK)
        return ret;

    switch (Flags)
    {
    case PVCF_ROTATE90CW:
        return RotateDecodedImage(pHandle, TRUE);

    case PVCF_ROTATE90CCW:
        return RotateDecodedImage(pHandle, FALSE);

    default:
        return PVC_INVALID_HANDLE;
    }
}

static DWORD WINAPI PVIsOutCombSupportedOpen(int Fmt, int Compr, int Colors, int ColorModel)
{
    return (DWORD)-1;
}

static PVCODE WINAPI PVReadImageSequenceOpen(LPPVHandle Img, LPPVImageSequence* ppSeq)
{
    if (ppSeq != NULL)
        *ppSeq = NULL;
    return PVC_INVALID_HANDLE;
}

static PVCODE WINAPI PVCropImageOpen(LPPVHandle Img, int Left, int Top, int Width, int Height)
{
    return PVC_INVALID_HANDLE;
}
